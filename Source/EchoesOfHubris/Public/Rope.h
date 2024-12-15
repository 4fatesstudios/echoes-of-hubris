// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rope.generated.h"

struct FRopeSegment
{
public:
	FRopeSegment(FVector pos);

	FVector posNow;
	FVector posOld;
};

UCLASS()
class ECHOESOFHUBRIS_API ARope : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARope();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
	FVector constrainedYPlane;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
	FVector2D ropeStartPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
	FVector2D ropeEndPoint;


private:
	TArray<FRopeSegment> ropeSegments;
	float ropeSegmentLength;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
