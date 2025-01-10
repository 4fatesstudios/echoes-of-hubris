// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "OrnamentManager.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class ECHOESOFHUBRIS_API UOrnamentManager : public UGameInstance
{
	GENERATED_BODY()

	public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, category = OrnamentManager)
	TSubclassOf<UOrnament> Ornament;
};
