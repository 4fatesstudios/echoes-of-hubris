// Copyright Flekz Games. All Rights Reserved.

#pragma once
#include "PaperTileSet.h"
#include "TITileSet.generated.h"

class FJsonValue;
class UTICustomProperties;
class UTITile;

// An extension of the UPaperTileSet class that supports more parameters from Tiled
UCLASS(BlueprintType)
class TILEDINTEGRATION_API UTITileSet : public UPaperTileSet
{
    GENERATED_BODY()

    friend class UTIImporterFactory;

public:
    UTITileSet(const class FObjectInitializer& ObjectInitializer);

    void ForEachTile(TFunction<bool(UTITile*, uint32)> Callback, bool bReversed = false) const;

private:
    void InitializeTiles(TSubclassOf<UTITile> TileClass);

    void LoadCustomProperties(const TArray<TSharedPtr<FJsonValue>>& Properties);

    // Called when the tile set custom properties have been loaded
    virtual void OnCustomPropertiesLoaded_Implementation(UTICustomProperties* Properties);

    void OnImportedBroadcast();

    // Called when the tile set has finished being imported
    virtual void OnImported_Implementation();

public:
    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    UTITile* GetTile(int32 TileIndex) const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    UTICustomProperties* GetCustomProperties() const { return CustomProperties; }

protected:
    // Called when the tile set custom properties have been loaded
    UFUNCTION(BlueprintNativeEvent, Category = "TiledIntegration")
    void OnCustomPropertiesLoaded(UTICustomProperties * Properties);

    // Called when the tile set has finished being imported
    UFUNCTION(BlueprintNativeEvent, Category = "TiledIntegration")
    void OnImported();

private:
    UPROPERTY()
    UTICustomProperties* CustomProperties;

    UPROPERTY(VisibleAnywhere, Instanced, Category = "TiledIntegration")
    TArray<UTITile*> Tiles;
};