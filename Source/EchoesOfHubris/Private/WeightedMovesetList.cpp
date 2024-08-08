#include "WeightedMovesetList.h"
#include "Math/UnrealMathUtility.h"

void UWeightedMovesetList::AddMove(const FString& Move, int32 Weight)
{
    MovesetMap.Add(Move, Weight);
}

void UWeightedMovesetList::RemoveMove(const FString& Move)
{
    MovesetMap.Remove(Move);
}

void UWeightedMovesetList::UpdateMoveWeight(const FString& Move, int32 NewWeight)
{
    if (MovesetMap.Contains(Move))
    {
        MovesetMap[Move] = NewWeight;
    }
}

FString UWeightedMovesetList::GetRandomMove() const
{
    if (MovesetMap.Num() == 0)
    {
        return FString(); // Return an empty string if there are no moves
    }

    // Calculate the total weight excluding moves with a weight of 0
    int32 TotalWeight = 0;
    for (const auto& Elem : MovesetMap)
    {
        if (Elem.Value > 0)
        {
            TotalWeight += Elem.Value;
        }
    }

    if (TotalWeight == 0)
    {
        return FString(); // No valid moves with a weight greater than 0
    }

    int32 RandomWeight = FMath::RandRange(1, TotalWeight);
    int32 CurrentWeight = 0;

    for (const auto& Elem : MovesetMap)
    {
        if (Elem.Value > 0)
        {
            CurrentWeight += Elem.Value;
            if (RandomWeight <= CurrentWeight)
            {
                return Elem.Key;
            }
        }
    }

    return FString(); // Fallback, should not reach here if map is correctly populated
}

void UWeightedMovesetList::PopulateMovesetFromArrays(const TArray<FString>& Moves, const TArray<int32>& Weights, bool bReplaceExisting)
{
    if (Moves.Num() != Weights.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("Moves and Weights arrays must be of the same length."));
        return;
    }

    if (bReplaceExisting)
    {
        MovesetMap.Empty(); // Clear existing map if replacing
    }

    for (int32 i = 0; i < Moves.Num(); i++)
    {
        MovesetMap.Add(Moves[i], Weights[i]);
    }
}
