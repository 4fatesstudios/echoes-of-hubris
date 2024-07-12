#include "FilteredHitboxes.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"

UFilteredHitboxes::UFilteredHitboxes()
{
    SeenActors = TSet<AActor*>();
}

TArray<UBoxComponent*> UFilteredHitboxes::FilterComponents(const TArray<UPrimitiveComponent*>& InComponents)
{
    TArray<UBoxComponent*> FilteredComponents;

    // Set to track seen owners
    TSet<AActor*> SeenOwners;

    for (UPrimitiveComponent* PrimitiveComp : InComponents)
    {
        // Check if the component is a UBoxComponent and matches other criteria
        UBoxComponent* BoxComp = Cast<UBoxComponent>(PrimitiveComp);
        if (BoxComp && !SeenOwners.Contains(BoxComp->GetOwner()) && BoxComp->GetName().Contains("Hitbox"))
        {
            // Add to seen owners and filtered list
            SeenOwners.Add(BoxComp->GetOwner());
            FilteredComponents.Add(BoxComp);
        }
    }

    return FilteredComponents;
}

void UFilteredHitboxes::ClearSeenActors()
{
    SeenActors.Empty();
}
