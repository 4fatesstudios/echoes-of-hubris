// Copyright Flekz Games. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TITile.generated.h"

class FJsonValue;
class UPaperFlipbook;
class UTICustomProperties;
class UTITileSet;

struct FPaperTileMetadata;

// A single tile inside a UTITileSet
UCLASS(BlueprintType)
class TILEDINTEGRATION_API UTITile : public UObject
{
    GENERATED_BODY()

    friend class UTITileSet;
    friend class UTIImporterFactory;

public:
    UTITile(const class FObjectInitializer& ObjectInitializer);

    FPaperTileMetadata* GetMutableMetadata();
    const FPaperTileMetadata* GetMetadata() const;

private:
    void LoadCustomProperties(const TArray<TSharedPtr<FJsonValue>>& Properties);

    // Called when the tile custom properties have been loaded
    virtual void OnCustomPropertiesLoaded_Implementation(UTICustomProperties* Properties);
    
    // Called when the tile has finished being imported
    virtual void OnImported_Implementation();

public:
    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    UTICustomProperties* GetCustomProperties() const { return CustomProperties; }

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    FName GetUserData() const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    virtual UTITileSet* GetTileSet() const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    int32 GetTileIndex() const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    FIntPoint GetSize() const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    bool IsValid() const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    bool IsAnimated() const { return Flipbook != nullptr; }

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    UPaperFlipbook* GetFlipbook() const { return Flipbook; }

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
    TWeakObjectPtr<UTITileSet> TileSet;
    
    UPROPERTY()
    int32 TileIndex;

    UPROPERTY()
    TObjectPtr<UPaperFlipbook> Flipbook;
};