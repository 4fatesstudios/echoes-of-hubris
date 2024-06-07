#include "SwingActorAroundActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "TimerManager.h"

void USwingActorAroundActor::SwingActorAroundAnother(AActor* ActorA, AActor* ActorB, float SwingSpeed)
{
    if (!ActorA || !ActorB || SwingSpeed <= 0.0f)
    {
        return;
    }

    FVector StartLocation = ActorA->GetActorLocation();
    FVector ActorBLocation = ActorB->GetActorLocation();

    // Calculate the target position opposite ActorB
    FVector ToA = StartLocation - ActorBLocation;
    FVector EndLocation = ActorBLocation - ToA;

    // Calculate the swing duration based on the swing speed
    float SwingDuration = (StartLocation - EndLocation).Size() / SwingSpeed;

    FTimerHandle TimerHandle;

    // Start the swing process
    PerformSwing(ActorA, StartLocation, EndLocation, SwingSpeed, 0.0f, SwingDuration, TimerHandle);
}

void USwingActorAroundActor::PerformSwing(AActor* ActorA, FVector StartLocation, FVector EndLocation, float SwingSpeed, float ElapsedTime, float SwingDuration, FTimerHandle& TimerHandle)
{
    if (!ActorA) return;

    UWorld* World = ActorA->GetWorld();
    if (!World) return;

    float DeltaTime = World->GetDeltaSeconds();
    ElapsedTime += DeltaTime;

    // Calculate the alpha value for interpolation
    float Alpha = FMath::Clamp(ElapsedTime / SwingDuration, 0.0f, 1.0f);

    // Calculate the current desired location
    FVector CurrentLocation = FMath::Lerp(StartLocation, EndLocation, Alpha);

    // Calculate velocity needed to reach CurrentLocation
    FVector Velocity = CalculateVelocity(ActorA->GetActorLocation(), CurrentLocation, SwingSpeed, DeltaTime);

    // Use velocity to simulate smooth movement
    FVector NewLocation = ActorA->GetActorLocation() + Velocity * DeltaTime;

    // Apply new location using physics or other movement logic
    if (UPrimitiveComponent* RootComponent = Cast<UPrimitiveComponent>(ActorA->GetRootComponent()))
    {
        RootComponent->MoveComponent(NewLocation - ActorA->GetActorLocation(), ActorA->GetActorRotation(), true);
    }

    // Check if the swing is complete
    if (Alpha >= 1.0f)
    {
        // Swing complete; clear the timer
        World->GetTimerManager().ClearTimer(TimerHandle);
    }
    else
    {
        // Continue swinging in the next frame
        World->GetTimerManager().SetTimer(TimerHandle, [ActorA, StartLocation, EndLocation, SwingSpeed, ElapsedTime, SwingDuration, &TimerHandle]() {
            PerformSwing(ActorA, StartLocation, EndLocation, SwingSpeed, ElapsedTime, SwingDuration, TimerHandle);
            }, DeltaTime, false);
    }
}

FVector USwingActorAroundActor::CalculateVelocity(FVector CurrentLocation, FVector TargetLocation, float SwingSpeed, float DeltaTime)
{
    FVector Direction = (TargetLocation - CurrentLocation).GetSafeNormal();
    return Direction * SwingSpeed;
}
