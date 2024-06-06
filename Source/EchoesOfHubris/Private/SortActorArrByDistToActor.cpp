#include "SortActorArrByDistToActor.h"
#include "GameFramework/Actor.h"

TArray<AActor*> USortActorArrByDistToActor::SortArrayByActorDistance(const TArray<AActor*>& ActorArray, AActor* ReferenceActor)
{
	if (!ReferenceActor)
	{
		// If the reference actor is null, return the original array
		return ActorArray;
	}

	TArray<AActor*> SortedArray = ActorArray;

	SortedArray.Sort([ReferenceActor](const AActor& A, const AActor& B) {
		float DistanceA = FVector::Dist(ReferenceActor->GetActorLocation(), A.GetActorLocation());
		float DistanceB = FVector::Dist(ReferenceActor->GetActorLocation(), B.GetActorLocation());
		return DistanceA < DistanceB;
		});

	return SortedArray;
}
