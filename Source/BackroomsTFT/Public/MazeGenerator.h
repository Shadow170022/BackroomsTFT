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
    // Tus propiedades editables
    UPROPERTY(EditAnywhere, Category = "Maze Config")
    int32 Width = 10;

    UPROPERTY(EditAnywhere, Category = "Maze Config")
    int32 Height = 10;

    UPROPERTY(EditAnywhere, Category = "Maze Config")
    float RoomXSize = 600.f;

    UPROPERTY(EditAnywhere, Category = "Maze Config")
    float RoomZSize = 600.f;

    UPROPERTY(EditAnywhere, Category = "Maze Config")
    TArray<TSubclassOf<AActor>> RoomPrefabs;

    // Estructuras internas (no expuestas)
    TArray<bool> Visited;
    TMap<FIntPoint, AActor*> Rooms;

    // Tus funciones
    void GenerateMaze();
    TArray<FMazeCell> GetUnvisitedNeighbors(const FMazeCell& Cell) const;
    void CreateRoom(const FMazeCell& Cell);
};
