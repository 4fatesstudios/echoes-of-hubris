#include "RandomPointInCollision.h"

FVector ARandomPointInCollision::GetRandomPointInBox(UBoxComponent* BoxComponent)
{
    if (!BoxComponent) return FVector::ZeroVector;

    // Get the extent of the box (half-size)
    FVector Extent = BoxComponent->GetScaledBoxExtent();

    // Get a random point in local space
    FVector RandomPoint = UKismetMathLibrary::RandomPointInBoundingBox(FVector::ZeroVector, Extent);

    // Transform the random point from local space to world space
    FVector WorldLocation = BoxComponent->GetComponentLocation();
    FRotator WorldRotation = BoxComponent->GetComponentRotation();
    FVector WorldPoint = WorldLocation + WorldRotation.RotateVector(RandomPoint);

    return WorldPoint;
}

FVector ARandomPointInCollision::GetRandomPointInSphere(USphereComponent* SphereComponent)
{
    if (!SphereComponent) return FVector::ZeroVector;

    // Get the radius of the sphere, accounting for scale
    float Radius = SphereComponent->GetScaledSphereRadius();

    // Get a random point inside the sphere in local space
    FVector RandomPoint = UKismetMathLibrary::RandomUnitVector() * FMath::RandRange(0.0f, Radius);

    // Transform the random point from local space to world space
    FVector WorldLocation = SphereComponent->GetComponentLocation();
    FVector WorldPoint = WorldLocation + RandomPoint;

    return WorldPoint;
}

FVector ARandomPointInCollision::GetRandomPointInCapsule(UCapsuleComponent* CapsuleComponent)
{
    if (!CapsuleComponent) return FVector::ZeroVector;

    // Get the half height and radius of the capsule, accounting for scale
    float HalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();
    float Radius = CapsuleComponent->GetScaledCapsuleRadius();

    // Get a random point inside a unit cylinder in local space
    FVector RandomPoint = UKismetMathLibrary::RandomPointInBoundingBox(FVector::ZeroVector, FVector(Radius, Radius, HalfHeight));

    // Adjust for the spherical caps of the capsule
    if (FMath::FRand() < Radius / (Radius + HalfHeight))
    {
        RandomPoint.Z = FMath::RandBool() ? FMath::RandRange(HalfHeight, HalfHeight + Radius) : FMath::RandRange(-HalfHeight - Radius, -HalfHeight);
    }

    // Transform the random point from local space to world space
    FVector WorldLocation = CapsuleComponent->GetComponentLocation();
    FRotator WorldRotation = CapsuleComponent->GetComponentRotation();
    FVector WorldPoint = WorldLocation + WorldRotation.RotateVector(RandomPoint);

    return WorldPoint;
}
