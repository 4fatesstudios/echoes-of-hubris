#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "WeightedMovesetList.generated.h"

UCLASS(Blueprintable)
class ECHOESOFHUBRIS_API UWeightedMovesetList : public UObject
{
    GENERATED_BODY()

private:
    TMap<FString, int32> MovesetMap;

public:
    // Adds a move with a specific weight
    UFUNCTION(BlueprintCallable, Category = "Weighted Moveset")
    void AddMove(const FString& Move, int32 Weight);

    // Removes a move from the moveset
    UFUNCTION(BlueprintCallable, Category = "Weighted Moveset")
    void RemoveMove(const FString& Move);

    // Updates the weight of an existing move
    UFUNCTION(BlueprintCallable, Category = "Weighted Moveset")
    void UpdateMoveWeight(const FString& Move, int32 NewWeight);

    // Returns a random move based on their weights
    UFUNCTION(BlueprintCallable, Category = "Weighted Moveset")
    FString GetRandomMove() const;

    // Populates the map from arrays of moves and weights
    UFUNCTION(BlueprintCallable, Category = "Weighted Moveset")
    void PopulateMovesetFromArrays(const TArray<FString>& Moves, const TArray<int32>& Weights, bool bReplaceExisting = true);
};
