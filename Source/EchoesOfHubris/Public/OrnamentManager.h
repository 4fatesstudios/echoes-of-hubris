// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OrnamentManager.generated.h"

/**
 * 
 */

enum ESpiritualBeadTier { Lesser=1, Fair=2, Greater=3, Sanctified=4 };

UCLASS(Blueprintable)
class ECHOESOFHUBRIS_API UOrnamentManager : public UObject
{
	GENERATED_BODY()

	
	
	public:
	// Track obtained Ornaments
	UPROPERTY(EditAnywhere, BlueprintReadWrite, category = OrnamentManager)
	TArray<TSubclassOf<UObject>> ObtainedOrnaments;
	
	// Track unequipped Ornaments
	UPROPERTY(EditAnywhere, BlueprintReadWrite, category = OrnamentManager)
	TArray<TSubclassOf<UObject>> UnequippedOrnaments;

	// TODO: Create Spiritual Bead tracker

	// Adds unique Ornament to UnequippedOrnaments array
	UFUNCTION(BlueprintCallable, category = OrnamentManager)
	void AddOrnament(const TSubclassOf<UObject> Ornament);

	// Equip Ornament to a specific SpiritualBead if there is enough space
	UFUNCTION(BlueprintCallable, category = OrnamentManager)
	bool EquipOrnament(const TSubclassOf<UObject> Ornament);
};

struct FSpiritualBead
{
	FSpiritualBead() : BeadTier(Lesser) {}
	
	explicit FSpiritualBead(const ESpiritualBeadTier BeadTier) : BeadTier(BeadTier)
	{
		
	}

	const ESpiritualBeadTier BeadTier;
};

