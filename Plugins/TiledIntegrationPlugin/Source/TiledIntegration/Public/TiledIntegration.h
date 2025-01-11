// Copyright Flekz Games. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FTIModule : public IModuleInterface
{
public:
	void StartupModule() override;
	void ShutdownModule() override;
};
