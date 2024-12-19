// Copyright Flekz Games. All Rights Reserved.

#pragma once
#include "PaperTileMapActor.h"
#include "TITileMapActor.generated.h"

class UTITileMapComponent;

UCLASS(ComponentWrapperClass)
class TILEDINTEGRATION_API ATITileMapActor : public APaperTileMapActor
{
    GENERATED_UCLASS_BODY()

protected:
    void CreateTileMapComponent(TSubclassOf<UTITileMapComponent> TileMapComponentClass, FName TileMapComponentName);
};