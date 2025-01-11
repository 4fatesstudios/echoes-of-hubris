// Copyright Flekz Games. All Rights Reserved.

#pragma once
#include "PaperTileLayer.h"
#include "TITileLayer.generated.h"

class FJsonValue;
class UTICustomProperties;
class UBodySetup;
class UTITileInstance;
class UTITileMap;
class UTITileSet;

// An extension of the UPaperTileLayer class that supports more parameters from Tiled
UCLASS(BlueprintType)
class TILEDINTEGRATION_API UTITileLayer : public UPaperTileLayer
{
    GENERATED_BODY()

    friend class UTIImporterFactory;
    friend class UTITileMap;

public:
    UTITileLayer(const class FObjectInitializer& ObjectInitializer);

    void ForEachOccupiedTile(TFunction<bool(UTITileInstance*, uint32)> Callback, bool bReversed = false) const;
    void ForEachTile(TFunction<bool(UTITileInstance*, uint32)> Callback, bool bReversed = false) const;

    int32 GetNumOccupiedTiles() const;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    void InitializeTiles(TSubclassOf<UTITileInstance> TileInstanceClass);

    void LoadCustomProperties(const TArray<TSharedPtr<FJsonValue>>& Properties);

    // Called when the tile set custom properties have been loaded
    virtual void OnCustomPropertiesLoaded_Implementation(UTICustomProperties* Properties);

    void OnImportedBroadcast();

    // Called when the tile layer has finished being imported
    virtual void OnImported_Implementation();

    bool DoesLayerCollide() const;

    bool GetBoolPropertyValue(const FString& PropertyName, bool DefaultValue = false) const;
    float GetFloatPropertyValue(const FString& PropertyName, float DefaultValue = 0.0f) const;

public:
    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    UTICustomProperties* GetCustomProperties() const { return CustomProperties; }

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    UTITileInstance* GetTile(int32 X, int32 Y) const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    UTITileMap* GetTileMap() const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    int32 GetLayerIndex() const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    bool IsValid() const;

protected:
    // Called when the tile set custom properties have been loaded
    UFUNCTION(BlueprintNativeEvent, Category = "TiledIntegration")
    void OnCustomPropertiesLoaded(UTICustomProperties* Properties);

    // Called when the tile layer has finished being imported
    UFUNCTION(BlueprintNativeEvent, Category = "TiledIntegration")
    void OnImported();

private:
    UPROPERTY()
    UTICustomProperties* CustomProperties;

    UPROPERTY(Instanced)
    TArray<UTITileInstance*> Tiles;

    UPROPERTY()
    TWeakObjectPtr<UTITileMap> TileMap;

    UPROPERTY()
    int32 LayerIndex;
};