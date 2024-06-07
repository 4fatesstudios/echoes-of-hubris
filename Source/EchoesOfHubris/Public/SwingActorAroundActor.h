#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SwingActorAroundActor.generated.h"

UCLASS()
class ECHOESOFHUBRIS_API USwingActorAroundActor : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Movement")
    static void SwingActorAroundAnother(AActor* ActorA, AActor* ActorB, float SwingSpeed);

private:
    static void PerformSwing(AActor* ActorA, FVector StartLocation, FVector EndLocation, float SwingSpeed, float ElapsedTime, float SwingDuration, FTimerHandle& TimerHandle);

    static FVector CalculateVelocity(FVector CurrentLocation, FVector TargetLocation, float SwingSpeed, float DeltaTime);
};
