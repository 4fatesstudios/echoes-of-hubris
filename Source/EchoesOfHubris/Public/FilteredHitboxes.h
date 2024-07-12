#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Components/PrimitiveComponent.h"
#include "FilteredHitboxes.generated.h"

UCLASS(Blueprintable)
class ECHOESOFHUBRIS_API UFilteredHitboxes : public UObject
{
    GENERATED_BODY()

public:
    UFilteredHitboxes();

    /**
     * Filters the given array of Primitive Components, removing duplicates based on their owners,
     * removing any components that do not contain "Hitbox" in their name,
     * and filtering out components that are not UBoxComponents.
     *
     * @param InComponents The array of Primitive Components to be filtered.
     * @return The filtered array of UBoxComponent*.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom", meta = (ToolTip = "Filters the given array of Primitive Components, removing duplicates based on their owners, removing any components that do not contain \"Hitbox\" in their name, and filtering out components that are not UBoxComponents."))
    TArray<UBoxComponent*> FilterComponents(const TArray<UPrimitiveComponent*>& InComponents);

    /**
     * Clears the set of seen actors.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom", meta = (ToolTip = "Clears the set of seen actors."))
    void ClearSeenActors();

    /**
     * The set of actors that have been seen.
     */
    UPROPERTY(BlueprintReadWrite, Category = "Custom", meta = (ToolTip = "The set of actors that have been seen."))
    TSet<AActor*> SeenActors;
};
