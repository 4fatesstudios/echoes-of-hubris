#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Components/BoxComponent.h"
#include "FilteredHitboxes.generated.h"

UCLASS(Blueprintable)
class ECHOESOFHUBRIS_API UFilteredHitboxes : public UObject
{
    GENERATED_BODY()

public:
    UFilteredHitboxes();

    /**
     * Filters the given array of Box Components, removing duplicates based on their owners
     * and removing any components that do not contain "Hitbox" in their name.
     *
     * @param BoxComponents The array of Box Components to be filtered.
     */
    UFUNCTION(BlueprintCallable, Category = "Filter Hitboxes", meta = (ToolTip = "Filters the given array of Box Components, removing duplicates based on their owners and removing any components that do not contain \"Hitbox\" in their name."))
    void FilterBoxCollisions(TArray<UBoxComponent*>& BoxComponents);

    /**
     * Clears the set of seen actors.
     */
    UFUNCTION(BlueprintCallable, Category = "Filter Hitboxes", meta = (ToolTip = "Clears the set of seen actors."))
    void ClearSeenActors();

    /**
     * The set of actors that have been seen.
     */
    UPROPERTY(BlueprintReadWrite, Category = "Filter Hitboxes", meta = (ToolTip = "The set of actors that have been seen."))
    TSet<AActor*> SeenActors;
};
