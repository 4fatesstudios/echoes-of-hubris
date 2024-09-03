#include "FilteredHitboxes.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"

UFilteredHitboxes::UFilteredHitboxes()
{
    SeenActors = TSet<AActor*>();
}

TArray<UPrimitiveComponent*> UFilteredHitboxes::FilterComponents(const TArray<UPrimitiveComponent*>& InComponents)
{
    TArray<UPrimitiveComponent*> FilteredComponents;

    // Set to track seen owners
    TSet<AActor*> SeenOwners;

    for (UPrimitiveComponent* PrimitiveComp : InComponents)
    {
        // Check if the component's owner hasn't been seen yet
        if (PrimitiveComp && !SeenOwners.Contains(PrimitiveComp->GetOwner()))
        {
            // Add to seen owners and filtered list
            SeenOwners.Add(PrimitiveComp->GetOwner());
            FilteredComponents.Add(PrimitiveComp);
        }
    }

    return FilteredComponents;
}

void UFilteredHitboxes::ClearSeenActors()
{
    SeenActors.Empty();
}