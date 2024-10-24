#include "SortActorArrByDistToActor.h"
#include "GameFramework/Actor.h"

TArray<AActor*> USortActorArrByDistToActor::SortArrayByActorDistance(const TArray<AActor*>& ActorArray, AActor* ReferenceActor)
{
    if (!ReferenceActor)
    {
        return ActorArray; // If no reference actor is provided, return the array as is
    }

    TArray<AActor*> SortedArray = ActorArray;
    SortedArray.Sort([ReferenceActor](const AActor& A, const AActor& B) {
        return ReferenceActor->GetDistanceTo(&A) < ReferenceActor->GetDistanceTo(&B);
        });

    return SortedArray;
}

TArray<AActor*> USortActorArrByDistToActor::SortArrayByLocationDistance(const TArray<AActor*>& ActorArray, const FVector& ReferenceLocation)
{
    TArray<AActor*> SortedArray = ActorArray;
    SortedArray.Sort([ReferenceLocation](const AActor& A, const AActor& B) {
        return FVector::Dist(A.GetActorLocation(), ReferenceLocation) < FVector::Dist(B.GetActorLocation(), ReferenceLocation);
        });

    return SortedArray;
}