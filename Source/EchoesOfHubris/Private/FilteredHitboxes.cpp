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

    // Set to track seen owners during this filter pass
    TSet<AActor*> SeenOwners;

    for (UPrimitiveComponent* PrimitiveComp : InComponents)
    {
        AActor* Owner = PrimitiveComp ? PrimitiveComp->GetOwner() : nullptr;

        // Check if the component's owner exists, hasn't been seen in this filter pass, and isn't in SeenActors
        if (Owner && !SeenOwners.Contains(Owner) && !SeenActors.Contains(Owner))
        {
            // Add to seen owners and filtered list
            SeenOwners.Add(Owner);
            FilteredComponents.Add(PrimitiveComp);

            // Also add the actor to SeenActors to keep track of all processed actors
            SeenActors.Add(Owner);
        }
    }

    return FilteredComponents;
}

void UFilteredHitboxes::ClearSeenActors()
{
    SeenActors.Empty();
}
