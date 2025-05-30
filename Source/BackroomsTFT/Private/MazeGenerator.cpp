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
	FMazeCell Current(0, 0);

	// Marcar y crear la primera sala
	Visited[0 + 0 * Width] = true;
	CreateRoom(Current);
	Stack.Push(Current);

	// DFS con backtracking
	while (Stack.Num() > 0)
	{
		Current = Stack.Last();
		TArray<FMazeCell> Neighbors = GetUnvisitedNeighbors(Current);

		if (Neighbors.Num() > 0)
		{
			// Escoger uno al azar
			int32 Idx = UKismetMathLibrary::RandomIntegerInRange(0, Neighbors.Num() - 1);
			FMazeCell Next = Neighbors[Idx];

			// Marcar y crear
			Visited[Next.X + Next.Y * Width] = true;
			CreateRoom(Next);

			Stack.Push(Next);
		}
		else
		{
			// Retroceder
			Stack.Pop();
		}
	}
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

void AMazeGenerator::CreateRoom(const FMazeCell& Cell)
{
	FIntPoint Key(Cell.X, Cell.Y);
	if (Rooms.Contains(Key))
	{
		UE_LOG(LogTemp, Warning, TEXT("Room %d,%d already exists"), Cell.X, Cell.Y);
		return;
	}

	// Elegir prefab aleatorio
	int32 PrefabIdx = UKismetMathLibrary::RandomIntegerInRange(0, RoomPrefabs.Num() - 1);
	TSubclassOf<AActor> RoomClass = RoomPrefabs[PrefabIdx];
	if (!RoomClass) return;

	// Calcular posición
	FVector Location(
		Cell.X * RoomXSize,
		Cell.Y * RoomZSize,
		0.f
	);
	FActorSpawnParameters Params;
	Params.Owner = this;

	// Instanciar actor
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
