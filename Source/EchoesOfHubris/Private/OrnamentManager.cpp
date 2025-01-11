// Fill out your copyright notice in the Description page of Project Settings.


#include "OrnamentManager.h"

void UOrnamentManager::AddOrnament(const TSubclassOf<UObject> Ornament)
{
	if (!Ornament)
	{
		UE_LOG(LogTemp, Error, TEXT("Ornament is null"));
		return;
	}

	// Checks if Ornament is unique
	if (ObtainedOrnaments.Contains(Ornament))
	{
		UE_LOG(LogTemp, Error, TEXT("Ornament already exists"));
		return;
	}

	ObtainedOrnaments.Add(Ornament);
	UnequippedOrnaments.Add(Ornament);
	UE_LOG(LogTemp, Display, TEXT("Ornament added"));
}

bool UOrnamentManager::EquipOrnament(const TSubclassOf<UObject> Ornament)
{
	return false;
}
