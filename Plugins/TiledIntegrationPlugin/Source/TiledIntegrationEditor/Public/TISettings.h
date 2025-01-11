// Copyright Flekz Games. All Rights Reserved.

#pragma once
#include "Engine/DeveloperSettings.h"
#include "TISettings.generated.h"

class ATITileMapActor;
class UTITile;
class UTITileInstance;
class UTITileMap;
class UTITileMapComponent;
class UTITileLayer;
class UTITileSet;

UCLASS(config = EditorPerProjectUserSettings, meta = (DisplayName = "Tiled Integration"))
class TILEDINTEGRATIONEDITOR_API UTISettings : public UDeveloperSettings
{
    GENERATED_UCLASS_BODY()

public:
    UPROPERTY(config, EditAnywhere, Category = "TiledIntegration", meta = (ConfigRestartRequired = true, ToolTip = "Relative file path (from the root of the project directory) where the plugin save file will be located. If changed after importing files please manually move the save file to the new location."))
    FString SaveFilePath;

    UPROPERTY(config, EditAnywhere, Category = "TiledIntegration", meta = (ConfigRestartRequired = true))
    TSubclassOf<UTITileMap> TileMapClass;

    UPROPERTY(config, EditAnywhere, Category = "TiledIntegration", meta = (ConfigRestartRequired = true))
    TSubclassOf<ATITileMapActor> TileMapActorClass;

    UPROPERTY(config, EditAnywhere, Category = "TiledIntegration", meta = (ConfigRestartRequired = true))
    TSubclassOf<UTITileSet> TileSetClass;

    UPROPERTY(config, EditAnywhere, Category = "TiledIntegration", meta = (ConfigRestartRequired = true))
    TSubclassOf<UTITileLayer> TileLayerClass;

    UPROPERTY(config, EditAnywhere, Category = "TiledIntegration", meta = (ConfigRestartRequired = true))
    TSubclassOf<UTITileInstance> TileInstanceClass;

    UPROPERTY(config, EditAnywhere, Category = "TiledIntegration", meta = (ConfigRestartRequired = true))
    TSubclassOf<UTITile> TileClass;
};