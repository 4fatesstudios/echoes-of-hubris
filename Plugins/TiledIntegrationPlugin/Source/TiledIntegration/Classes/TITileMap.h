// Copyright Flekz Games. All Rights Reserved.

#pragma once
#include "PaperTileMap.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TITileMap.generated.h"

class APaperTileMapActor;
class FJsonValue;
class UTICustomProperties;
class UTITileInstance;
class UTITileLayer;
class UTITileSet;

// An extension of the UPaperTileMap class that supports more parameters from Tiled
UCLASS(BlueprintType)
class TILEDINTEGRATION_API UTITileMap : public UPaperTileMap
{
    GENERATED_BODY()

    friend class UTIImporterFactory;

public:
    UTITileMap(const class FObjectInitializer& ObjectInitializer);

    // From highest to lowest layer
    void ForEachLayer(TFunction<bool(UTITileLayer*, uint32)> Callback, bool bReversed = false) const;
    void ForEachOccupiedTile(TFunction<bool(UTITileInstance*, uint32)> Callback, bool bReversed = false) const;
    void ForEachTile(TFunction<bool(UTITileInstance*, uint32)> Callback, bool bReversed = false) const;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    void LoadCustomProperties(const TArray<TSharedPtr<FJsonValue>>& Properties);

    // Called when the tile set custom properties have been loaded
    virtual void OnCustomPropertiesLoaded_Implementation(UTICustomProperties* Properties);

    void OnImportedBroadcast();

    // Called when the tile map has finished being imported
    virtual void OnImported_Implementation();

public:
    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    UTICustomProperties* GetCustomProperties() const { return CustomProperties; }

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    UTITileLayer* GetLayer(int32 LayerIndex) const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    TArray<UTITileLayer*> FindLayersByName(FString LayerName) const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    int32 GetLayersAmount() const;

protected:
    // Called when the tile set custom properties have been loaded
    UFUNCTION(BlueprintNativeEvent, Category = "TiledIntegration")
    void OnCustomPropertiesLoaded(UTICustomProperties* Properties);

    // Called when the tile map has finished being imported
    UFUNCTION(BlueprintNativeEvent, Category = "TiledIntegration")
    void OnImported();

private:
    UPROPERTY()
    UTICustomProperties* CustomProperties;

    UPROPERTY()
    TArray<TSoftObjectPtr<UTITileLayer>> Layers;
};

UCLASS()
class TILEDINTEGRATION_API UTITileMapLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    static UTITileMap* GetTileMapFromActor(const APaperTileMapActor* TileMapActor);

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    static UTITileLayer* GetTileLayerFromActor(const APaperTileMapActor* TileMapActor, int32 LayerIndex);
};