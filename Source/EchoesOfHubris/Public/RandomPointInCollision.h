#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "RandomPointInCollision.generated.h"

UCLASS()
class ECHOESOFHUBRIS_API ARandomPointInCollision : public AActor
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Random")
    static FVector GetRandomPointInBox(UBoxComponent* BoxComponent);

    UFUNCTION(BlueprintCallable, Category = "Random")
    static FVector GetRandomPointInSphere(USphereComponent* SphereComponent);

    UFUNCTION(BlueprintCallable, Category = "Random")
    static FVector GetRandomPointInCapsule(UCapsuleComponent* CapsuleComponent);
};
