#include "MazeGenerator.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"

AMazeGenerator::AMazeGenerator()
{
	PrimaryActorTick.bCanEverTick = false; // no necesitamos Tick
}

void AMazeGenerator::BeginPlay()
{
	Super::BeginPlay();

	// Validaciones básicas
	if (Width <= 0 || Height <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Maze dimensions must be > 0"));
		return;
	}
	if (RoomPrefabs.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("No RoomPrefabs assigned"));
		return;
	}

	EntryCell = PickRandomBorderCell();
	do {
		ExitCell = PickRandomBorderCell();
	} while (ExitCell.X == EntryCell.X && ExitCell.Y == EntryCell.Y);

	// Inicializar matriz de visitas
	Visited.Init(false, Width * Height);

	// Generar y (opcional) reconstruir NavMesh al final
	GenerateMaze();

	// Reconstruir NavMesh si tienes un NavMeshBoundsVolume en escena
	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		NavSys->Build();
	}
}

void AMazeGenerator::GenerateMaze()
{
	TArray<FMazeCell> Stack;
	FMazeCell Current = EntryCell;

	// Fase 0: Iniciar en EntryCell
	Visited[Current.X + Current.Y * Width] = true;
	CreateRoom(Current, /*bIsEntryOrNormal=*/true, /*bIsExit=*/false);
	Stack.Push(Current);

	// Fase 1: DFS dirigido hasta ExitCell
	bool bReachedExit = false;
	while (!bReachedExit && Stack.Num() > 0)
	{
		Current = Stack.Last();
		TArray<FMazeCell> Neighbors = GetUnvisitedNeighbors(Current);
		if (Neighbors.Num() > 0)
		{
			FMazeCell Next = Neighbors[UKismetMathLibrary::RandomIntegerInRange(0, Neighbors.Num() - 1)];
			Visited[Next.X + Next.Y * Width] = true;
			bool bIsExit = (Next.X == ExitCell.X && Next.Y == ExitCell.Y);
			CreateRoom(Next, /*bIsEntryOrNormal=*/false, bIsExit);
			Stack.Push(Next);

			if (bIsExit)
			{
				bReachedExit = true;
				MarkExit(Next);
			}
		}
		else
		{
			Stack.Pop();
		}
	}

	// Fase 2: Rellenar el resto con backtracking clásico
	while (Stack.Num() > 0)
	{
		Current = Stack.Last();
		TArray<FMazeCell> Neighbors = GetUnvisitedNeighbors(Current);
		if (Neighbors.Num() > 0)
		{
			FMazeCell Next = Neighbors[UKismetMathLibrary::RandomIntegerInRange(0, Neighbors.Num() - 1)];
			Visited[Next.X + Next.Y * Width] = true;
			CreateRoom(Next, /*bIsEntryOrNormal=*/false, /*bIsExit=*/false);
			Stack.Push(Next);
		}
		else
		{
			Stack.Pop();
		}
	}
}

FMazeCell AMazeGenerator::PickRandomBorderCell() const
{
	// 0 = izquierda, 1 = derecha, 2 = abajo, 3 = arriba
	int side = UKismetMathLibrary::RandomIntegerInRange(0, 3);
	int x = 0, y = 0;
	switch (side)
	{
	case 0: // izquierda
		x = 0;
		y = UKismetMathLibrary::RandomIntegerInRange(0, Height - 1);
		break;
	case 1: // derecha
		x = Width - 1;
		y = UKismetMathLibrary::RandomIntegerInRange(0, Height - 1);
		break;
	case 2: // abajo
		x = UKismetMathLibrary::RandomIntegerInRange(0, Width - 1);
		y = 0;
		break;
	case 3: // arriba
		x = UKismetMathLibrary::RandomIntegerInRange(0, Width - 1);
		y = Height - 1;
		break;
	}
	return FMazeCell(x, y);
}


TArray<FMazeCell> AMazeGenerator::GetUnvisitedNeighbors(const FMazeCell& Cell) const
{
	TArray<FMazeCell> Result;
	static const FMazeCell Dirs[4] = {
		FMazeCell(1, 0),
		FMazeCell(-1, 0),
		FMazeCell(0, 1),
		FMazeCell(0, -1)
	};

	for (const FMazeCell& Dir : Dirs)
	{
		int32 NX = Cell.X + Dir.X;
		int32 NY = Cell.Y + Dir.Y;

		// Dentro de rango y no visitado
		if (NX >= 0 && NX < Width && NY >= 0 && NY < Height)
		{
			if (!Visited[NX + NY * Width])
			{
				Result.Emplace(NX, NY);
			}
		}
	}
	return Result;
}

void AMazeGenerator::CreateRoom(const FMazeCell& Cell, bool bIsEntryOrNormal, bool bIsExit)
{
	FIntPoint Key(Cell.X, Cell.Y);
	if (Rooms.Contains(Key))
	{
		return;
	}

	// Determinar posición relativa al borde
	bool bIsLeft = (Cell.X == 0);
	bool bIsRight = (Cell.X == Width - 1);
	bool bIsBottom = (Cell.Y == 0);
	bool bIsTop = (Cell.Y == Height - 1);

	int BorderCount = (int)bIsLeft + (int)bIsRight + (int)bIsBottom + (int)bIsTop;
	bool bIsCorner = (BorderCount == 2);
	bool bIsBorder = (BorderCount == 1);
	bool bIsInterior = (BorderCount == 0);

	// Escoger el array apropiado
	TArray<TSubclassOf<AActor>>* PrefabArray = nullptr;

	// Entrada (EntryCell) y Salida (ExitCell)
	if (Cell.X == EntryCell.X && Cell.Y == EntryCell.Y)
	{
		PrefabArray = &RoomEntryPrefabs;
	}
	else if (bIsExit)
	{
		PrefabArray = &RoomExitPrefabs;
	}
	// Esquina
	else if (bIsCorner)
	{
		PrefabArray = &RoomCornerPrefabs;
	}
	// Borde
	else if (bIsBorder)
	{
		PrefabArray = &RoomBorderPrefabs;
	}
	// Interior
	else if (bIsInterior)
	{
		PrefabArray = &RoomInteriorPrefabs;
	}

	// Si no hay prefabs definidos para esta categoría, abortar
	if (!PrefabArray || PrefabArray->Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No prefabs assigned for this room type at %d,%d"), Cell.X, Cell.Y);
		return;
	}

	// Seleccionar uno al azar
	int32 Idx = FMath::RandRange(0, PrefabArray->Num() - 1);
	TSubclassOf<AActor> RoomClass = (*PrefabArray)[Idx];
	if (!RoomClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Null prefab in array at index %d"), Idx);
		return;
	}

	// Instanciar en el mundo
	FVector Location(Cell.X * RoomXSize, Cell.Y * RoomZSize, 0.f);
	FActorSpawnParameters Params;
	Params.Owner = this;

	AActor* RoomActor = GetWorld()->SpawnActor<AActor>(RoomClass, Location, FRotator::ZeroRotator, Params);
	if (RoomActor)
	{
		Rooms.Add(Key, RoomActor);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn room at %d,%d"), Cell.X, Cell.Y);
	}
}


void AMazeGenerator::MarkExit(const FMazeCell& Cell)
{
	// Aquí puedes, por ejemplo, instanciar un actor de puerta o efecto de salida:
	FVector Location(
		Cell.X * RoomXSize,
		Cell.Y * RoomZSize,
		0.f
	);
	// GetWorld()->SpawnActor<AYourExitActor>(ExitActorClass, Location, FRotator::ZeroRotator);
	UE_LOG(LogTemp, Log, TEXT("Exit placed at %d,%d"), Cell.X, Cell.Y);
}