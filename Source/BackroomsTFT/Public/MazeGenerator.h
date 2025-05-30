#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MazeGenerator.generated.h"

// Agrega BlueprintType y UPROPERTY para que UHT procese bien la struct
USTRUCT(BlueprintType)
struct FMazeCell
{
    GENERATED_BODY()

    FMazeCell() : X(0), Y(0) {}
    FMazeCell(int32 InX, int32 InY) : X(InX), Y(InY) {}

    UPROPERTY()
    int32 X;

    UPROPERTY()
    int32 Y;
};

UCLASS()
class BACKROOMSTFT_API AMazeGenerator : public AActor
{
    GENERATED_BODY()

public:
    AMazeGenerator();

protected:
    virtual void BeginPlay() override;

private:
    // propiedades editables
    UPROPERTY(EditAnywhere, Category = "Maze Config")
    int32 Width = 10;

    UPROPERTY(EditAnywhere, Category = "Maze Config")
    int32 Height = 10;

    UPROPERTY(EditAnywhere, Category = "Maze Config")
    float RoomXSize = 750.f;

    UPROPERTY(EditAnywhere, Category = "Maze Config")
    float RoomZSize = 750.f;

    UPROPERTY(EditAnywhere, Category = "Maze Prefabs")
    TArray<TSubclassOf<AActor>> RoomEntryPrefabs;

    UPROPERTY(EditAnywhere, Category = "Maze Prefabs")
    TArray<TSubclassOf<AActor>> RoomExitPrefabs;

    UPROPERTY(EditAnywhere, Category = "Maze Prefabs")
    TArray<TSubclassOf<AActor>> RoomCornerPrefabs;

    UPROPERTY(EditAnywhere, Category = "Maze Prefabs")
    TArray<TSubclassOf<AActor>> RoomBorderPrefabs;

    UPROPERTY(EditAnywhere, Category = "Maze Prefabs")
    TArray<TSubclassOf<AActor>> RoomInteriorPrefabs;

    UPROPERTY(EditAnywhere, Category = "Maze Config")
    TArray<TSubclassOf<AActor>> RoomPrefabs;

    // Internos
    TArray<bool> Visited;
    TMap<FIntPoint, AActor*> Rooms;

    // Entrada / Salida
    FMazeCell EntryCell;
    FMazeCell ExitCell;

    // Helpers
    FMazeCell PickRandomBorderCell() const;

    // Flujo de generación
    void GenerateMaze();
    TArray<FMazeCell> GetUnvisitedNeighbors(const FMazeCell& Cell) const;

    // Instanciación de salas
    void CreateRoom(const FMazeCell& Cell, bool bIsEntryOrNormal = false, bool bIsExit = false);
    void MarkExit(const FMazeCell& Cell);
};
