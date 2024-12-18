// Copyright Flekz Games. All Rights Reserved.

#include "TITileLayer.h"

#include "Dom/JsonValue.h"
#include "Paper2DModule.h"
#include "PhysicsEngine/BodySetup.h"
#include "TICustomProperties.h"
#include "TITile.h"
#include "TITileInstance.h"
#include "TITileMap.h"
#include "TITileSet.h"

UTITileLayer::UTITileLayer(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, TileMap(nullptr)
, LayerIndex(INDEX_NONE)
{
    CustomProperties = ObjectInitializer.CreateDefaultSubobject<UTICustomProperties>(this, TEXT("CustomProperties"));
}

UTITileInstance* UTITileLayer::GetTile(int32 X, int32 Y) const
{
    return InBounds(X, Y) ? Tiles[X + (Y * GetLayerWidth())] : nullptr;
}

UTITileMap* UTITileLayer::GetTileMap() const
{
    return TileMap.IsValid() ? TileMap.Get() : nullptr;
}

int32 UTITileLayer::GetLayerIndex() const
{
    return LayerIndex;
}

bool UTITileLayer::IsValid() const
{
    return TileMap.IsValid() && LayerIndex != INDEX_NONE;
}

void UTITileLayer::ForEachOccupiedTile(TFunction<bool(UTITileInstance*, uint32)> Callback, bool bReversed) const
{
    ForEachTile([&Callback](UTITileInstance* TileInstance, uint32 TileIndex) -> bool
    {
        if (TileInstance)
        {
            if (!Callback(TileInstance, TileIndex))
            {
                return false;
            }
        }

        return true;
    }, bReversed);
}

void UTITileLayer::ForEachTile(TFunction<bool(UTITileInstance*, uint32)> Callback, bool bReversed) const
{
    if (bReversed)
    {
        for (int32 TileIndex = Tiles.Num() - 1; TileIndex >= 0; TileIndex--)
        {
            if (!Callback(Tiles[TileIndex], TileIndex))
            {
                break;
            }
        }
    }
    else
    {
        for (int32 TileIndex = 0; TileIndex < Tiles.Num(); TileIndex++)
        {
            if (!Callback(Tiles[TileIndex], TileIndex))
            {
                break;
            }
        }
    }
}

int32 UTITileLayer::GetNumOccupiedTiles() const
{
    int32 NumOccupiedTiles = 0;
    for (UTITileInstance* Tile : Tiles)
    {
        if (Tile && Tile->IsValid())
        {
            ++NumOccupiedTiles;
        }
    }

    return NumOccupiedTiles;
}

#if WITH_EDITOR
void UTITileLayer::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    bool bTileLayerChanged = false;

    // Check if a new tile has been set using the editor and remove it if so OR
    // Check if a tile has been removed using the editor and re-add it if so
    if (GetNumOccupiedCells() != GetNumOccupiedTiles())
    {
        for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
        {
            const int32 X = TileIndex % GetLayerWidth();
            const int32 Y = TileIndex / GetLayerWidth();

            UTITileInstance* Tile = GetTile(X, Y);
            FPaperTileInfo TileInfo = GetCell(X, Y);

            const bool bIsOccupied = TileInfo.IsValid();
            const bool bShouldBeOccupied = Tile != nullptr;

            if (bIsOccupied && !bShouldBeOccupied)
            {
                TileInfo.TileSet = nullptr;
                TileInfo.PackedTileIndex = INDEX_NONE;
                SetCell(X, Y, TileInfo);
                bTileLayerChanged = true;
            }
            else if (!bIsOccupied && bShouldBeOccupied)
            {
                TileInfo.TileSet = Tile->GetTileSet();
                TileInfo.PackedTileIndex = Tile->GetTileIndex();
                SetCell(X, Y, TileInfo);
                bTileLayerChanged = true;
            }
        }
    }

    if (bTileLayerChanged)
    {
        SetFlags(RF_Transactional);
        Modify();
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UTITileLayer::LoadCustomProperties(const TArray<TSharedPtr<FJsonValue>>& Properties)
{
    CustomProperties->Load(Properties);
    OnCustomPropertiesLoaded(CustomProperties);
}

void UTITileLayer::OnCustomPropertiesLoaded_Implementation(UTICustomProperties* Properties)
{

}

void UTITileLayer::OnImportedBroadcast()
{
    ForEachOccupiedTile([](UTITileInstance* Tile, uint32)-> bool
    {
        Tile->OnImported();
        return true;
    });

    OnImported();
}

void UTITileLayer::OnImported_Implementation()
{

}

void UTITileLayer::InitializeTiles(TSubclassOf<UTITileInstance> TileInstanceClass)
{
    const int32 iLayerWidth = GetLayerWidth();
    const int32 NumTiles = iLayerWidth * GetLayerHeight();
    Tiles.Empty(NumTiles);

    for (int32 TileIndex = 0; TileIndex < NumTiles; ++TileIndex)
    {
        UTITileInstance* TileInstance = nullptr;
        const int32 X = TileIndex % iLayerWidth;
        const int32 Y = TileIndex / iLayerWidth;

        // Only instantiate a tile if it has content
        const auto& TileInfo = GetCell(X, Y);
        if (TileInfo.IsValid())
        {
            if (UTITileSet* TileSet = Cast<UTITileSet>(TileInfo.TileSet))
            {
                TileInstance = NewObject<UTITileInstance>(this, TileInstanceClass);
                TileInstance->TileLayer = this;
                TileInstance->Coordinates = FIntVector(X, Y, LayerIndex);
                TileInstance->TileSoftPtr = TileSet->GetTile(TileInfo.GetTileIndex());
            }
        }

        Tiles.Add(TileInstance);
    }
}

bool UTITileLayer::DoesLayerCollide() const
{
    return GetBoolPropertyValue("bLayerCollides", true);
}

bool UTITileLayer::GetBoolPropertyValue(const FString& PropertyName, bool DefaultValue) const
{
    FProperty* Property = FindFProperty<FProperty>(UPaperTileLayer::StaticClass(), *PropertyName);
    if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
    {
        return BoolProperty->GetPropertyValue_InContainer(this);
    }

    return DefaultValue;
}

float UTITileLayer::GetFloatPropertyValue(const FString& PropertyName, float DefaultValue) const
{
    FProperty* Property = FindFProperty<FProperty>(UPaperTileLayer::StaticClass(), *PropertyName);
    if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
    {
        return FloatProperty->GetPropertyValue_InContainer(this);
    }

    return DefaultValue;
}
