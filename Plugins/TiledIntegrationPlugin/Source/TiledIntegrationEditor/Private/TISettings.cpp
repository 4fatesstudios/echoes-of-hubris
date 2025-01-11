// Copyright Flekz Games. All Rights Reserved.

#include "TISettings.h"

#include "TITile.h"
#include "TITileInstance.h"
#include "TITileMap.h"
#include "TITileMapActor.h"
#include "TITileSet.h"
#include "TITileLayer.h"

UTISettings::UTISettings(const FObjectInitializer& initializer)
: UDeveloperSettings(initializer)
{
    SaveFilePath = TEXT("TiledIngetration.json");
    TileClass = UTITile::StaticClass();
    TileInstanceClass = UTITileInstance::StaticClass();
    TileMapClass = UTITileMap::StaticClass();
    TileMapActorClass = ATITileMapActor::StaticClass();
    TileSetClass = UTITileSet::StaticClass();
    TileLayerClass = UTITileLayer::StaticClass();
}