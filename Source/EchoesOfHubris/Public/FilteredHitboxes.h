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
     * Filters the given array of Primitive Components, removing duplicates based on their owners.
     *
     * @param InComponents The array of Primitive Components to be filtered.
     * @return The filtered array of UPrimitiveComponent*.
     */
    UFUNCTION(BlueprintCallable, Category = "Hitbox Filter", meta = (ToolTip = "Filters the given array of Primitive Components, removing duplicates based on their owners."))
    TArray<UPrimitiveComponent*> FilterComponents(const TArray<UPrimitiveComponent*>& InComponents);

    /**
     * Clears the set of seen actors.
     */
    UFUNCTION(BlueprintCallable, Category = "Hitbox Filter", meta = (ToolTip = "Clears the set of seen actors."))
    void ClearSeenActors();

    /**
     * The set of actors that have been seen.
     */
    UPROPERTY(BlueprintReadWrite, Category = "Hitbox Filter", meta = (ToolTip = "The set of actors that have been seen."))
    TSet<AActor*> SeenActors;
};