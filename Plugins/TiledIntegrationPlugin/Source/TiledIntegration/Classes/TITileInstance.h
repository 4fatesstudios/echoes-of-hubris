// Copyright Flekz Games. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TITileInstance.generated.h"

class ATITileMapActor;
class FJsonValue;
class UPaperFlipbook;
class UPaperSprite;
class UTICustomProperties;
class UTITile;
class UTITileLayer;
class UTITileMap;
class UTITileSet;

// A instanced UTITile inside a UTITileLayer
UCLASS(BlueprintType)
class TILEDINTEGRATION_API UTITileInstance : public UObject
{
    GENERATED_BODY()

    friend class FTITileMapRenderSceneProxy;
    friend class UTITileMapComponent;
    friend class UTITileLayer;
    friend class UTIImporterFactory;

public:
    UTITileInstance(const class FObjectInitializer& ObjectInitializer);

    virtual void Tick(float DeltaTime);

    UPaperSprite* GetAnimatedTileSprite();

private:
    void LoadCustomProperties(const TArray<TSharedPtr<FJsonValue>>& Properties);

    // Called when the tile custom properties have been loaded
    virtual void OnCustomPropertiesLoaded_Implementation(UTICustomProperties* Properties);

    // Called when the tile has finished being imported
    virtual void OnImported_Implementation();

public:
    // Get the properties of the tile set tile
    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    UTICustomProperties* GetTileCustomProperties();

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    UTICustomProperties* GetCustomProperties() const { return CustomProperties; }

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    UTITileSet* GetTileSet();

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    int32 GetTileIndex();

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    UTITile* GetTile();

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    UTITileLayer* GetLayer() const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    UTITileMap* GetTileMap() const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    const FIntVector& GetTileCoordinates() const;

    // Returns the top left tile position within the tile map
    // Specify the owner actor to convert the position to world position
    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    FVector GetTilePosition(ATITileMapActor* OwnerActor = nullptr) const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    bool IsValid();

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    bool IsAnimated();

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    UPaperFlipbook* GetFlipbook();

protected:
    // Called when the tile set custom properties have been loaded
    UFUNCTION(BlueprintNativeEvent, Category = "TiledIntegration")
    void OnCustomPropertiesLoaded(UTICustomProperties* Properties);

    // Called when the tile has finished being imported
    UFUNCTION(BlueprintNativeEvent, Category = "TiledIntegration")
    void OnImported();

private:
    UPROPERTY()
    UTICustomProperties* CustomProperties;

    UPROPERTY()
    TWeakObjectPtr<UTITileLayer> TileLayer;

    // If it has negative values it means it's invalid
    UPROPERTY()
    FIntVector Coordinates;

    UPROPERTY(Transient)
    TObjectPtr<UTITile> Tile;

    UPROPERTY()
    TSoftObjectPtr<UTITile> TileSoftPtr;

private:
    float AnimatedTileTime;
    uint32 AnimatedTileLastKeyFrame;
    bool bRenderDataDirty;
};