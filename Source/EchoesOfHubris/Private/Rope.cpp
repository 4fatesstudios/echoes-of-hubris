// Fill out your copyright notice in the Description page of Project Settings.


#include "Rope.h"

FRopeSegment::FRopeSegment(FVector pos)
{
	this->posNow = pos;
	this->posOld = pos;
}

// Sets default values
ARope::ARope()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	this->ropeSegments = TArray<FRopeSegment>();
	this->ropeSegmentLength = 5.0f;
	this->segmentLength = 50;
}

// Called when the game starts or when spawned
void ARope::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ARope::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
