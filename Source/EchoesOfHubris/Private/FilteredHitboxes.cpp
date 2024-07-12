#include "FilteredHitboxes.h"
#include "GameFramework/Actor.h"

UFilteredHitboxes::UFilteredHitboxes()
{
    SeenActors = TSet<AActor*>();
}

void UFilteredHitboxes::FilterBoxCollisions(TArray<UBoxComponent*>& BoxComponents)
{
    TArray<UBoxComponent*> FilteredComponents;

    for (UBoxComponent* BoxComp : BoxComponents)
    {
        if (BoxComp)
        {
            // Check if the box name contains "Hitbox"
            if (BoxComp->GetName().Contains("Hitbox"))
            {
                AActor* Owner = BoxComp->GetOwner();

                if (Owner && !SeenActors.Contains(Owner))
                {
                    // Add to the set of seen owners and the filtered list
                    SeenActors.Add(Owner);
                    FilteredComponents.Add(BoxComp);
                }
            }
        }
    }

    // Update the input array with the filtered components
    BoxComponents = FilteredComponents;
}

void UFilteredHitboxes::ClearSeenActors()
{
    SeenActors.Empty();
}
