#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SortActorArrByDistToActor.generated.h"

/**
 *
 */
UCLASS()
class ECHOESOFHUBRIS_API USortActorArrByDistToActor : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Sort Actor Array by Distance to Actor")
    static TArray<AActor*> SortArrayByActorDistance(const TArray<AActor*>& ActorArray, AActor* ReferenceActor);
};
