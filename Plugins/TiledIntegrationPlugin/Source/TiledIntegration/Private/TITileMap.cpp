// Copyright Flekz Games. All Rights Reserved.

#include "TITileMap.h"

#include "Dom/JsonValue.h"
#include "PaperTileMapActor.h"
#include "PaperTileMapComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "TICustomProperties.h"
#include "TITileLayer.h"
#include "TITileSet.h"

UTITileMap::UTITileMap(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
    CustomProperties = ObjectInitializer.CreateDefaultSubobject<UTICustomProperties>(this, TEXT("CustomProperties"));
}

UTITileLayer* UTITileMap::GetLayer(int32 LayerIndex) const
{
    return Layers.IsValidIndex(LayerIndex) && Layers[LayerIndex].IsValid() ? Layers[LayerIndex].Get() : nullptr;
}

TArray<UTITileLayer*> UTITileMap::FindLayersByName(FString LayerName) const
{
    TArray<UTITileLayer*> LayersFound;
    const FText LayerNameText = FText::FromString(LayerName);
    ForEachLayer([&LayersFound, &LayerNameText](UTITileLayer* Layer, uint32) -> bool
    {
        if (Layer->LayerName.EqualTo(LayerNameText))
        {
            LayersFound.Add(Layer);
        }

        return true;
    });

    return LayersFound;
}

int32 UTITileMap::GetLayersAmount() const
{
    return Layers.Num();
}

void UTITileMap::LoadCustomProperties(const TArray<TSharedPtr<FJsonValue>>& Properties)
{
    CustomProperties->Load(Properties);
    OnCustomPropertiesLoaded(CustomProperties);
}

void UTITileMap::OnCustomPropertiesLoaded_Implementation(UTICustomProperties* Properties)
{

}

void UTITileMap::OnImportedBroadcast()
{
    for (auto Layer : Layers)
    {
        Layer->OnImportedBroadcast();
    }

    OnImported();
}

void UTITileMap::OnImported_Implementation()
{
    
}

void UTITileMap::ForEachLayer(TFunction<bool(UTITileLayer*, uint32)> Callback, bool bReversed) const
{
    if (bReversed)
    {
        for (int32 LayerIndex = Layers.Num() - 1; LayerIndex >= 0; LayerIndex--)
        {
            if (Layers[LayerIndex].IsValid())
            {
                if (!Callback(Layers[LayerIndex].Get(), LayerIndex))
                {
                    break;
                }
            }
        }
    }
    else
    {
        for (int32 LayerIndex = 0; LayerIndex < Layers.Num(); LayerIndex++)
        {
            if (Layers[LayerIndex].IsValid())
            {
                if (!Callback(Layers[LayerIndex].Get(), LayerIndex))
                {
                    break;
                }
            }
        }
    }
}

void UTITileMap::ForEachOccupiedTile(TFunction<bool(UTITileInstance*, uint32)> Callback, bool bReversed) const
{
    for (auto Layer : Layers)
    {
        Layer->ForEachOccupiedTile(Callback, bReversed);
    }
}

void UTITileMap::ForEachTile(TFunction<bool(UTITileInstance*, uint32)> Callback, bool bReversed) const
{
    for (auto Layer : Layers)
    {
        Layer->ForEachTile(Callback, bReversed);
    }
}

#if WITH_EDITOR
void UTITileMap::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    bool bTileMapChanged = false;

    // Check if a layer was added using the editor and remove it if so
    if (Layers.Num() < TileLayers.Num())
    {
        for (int32 LayerIndex = TileLayers.Num() - 1; LayerIndex >= 0; --LayerIndex)
        {
            auto TileLayer = TileLayers[LayerIndex];

            bool bFound = false;
            for (auto Layer : Layers)
            {
                if (TileLayer == Layer)
                {
                    bFound = true;
                    break;
                }
            }

            if (!bFound)
            {
                bTileMapChanged = true;
                TileLayers.RemoveAt(LayerIndex);
            }
        }
    }

    // Check if a layer was removed using the editor and add it again if so
    if (Layers.Num() > TileLayers.Num())
    {
        for (int32 LayerIndex = 0; LayerIndex < Layers.Num(); ++LayerIndex)
        {
            if (UTITileLayer* Layer = GetLayer(LayerIndex))
            {
                bool bFound = false;
                for (auto TileLayer : TileLayers)
                {
                    if (TileLayer == Layer)
                    {
                        bFound = true;
                        break;
                    }
                }

                if (!bFound)
                {
                    TileLayers.Insert(Layer, LayerIndex);
                    bTileMapChanged = true;
                }
            }
        }
    }

    // Check if a layer has changed sorting and revert it if so
    for (int32 LayerIndex = 0; LayerIndex < Layers.Num(); ++LayerIndex)
    {
        if (UTITileLayer* Layer = GetLayer(LayerIndex))
        {
            const int32 CurrentLayerIndex = TileLayers.Find(Layer);
            const int32 DesiredLayerIndex = Layer->GetLayerIndex();
            if (CurrentLayerIndex != DesiredLayerIndex)
            {
                TileLayers.Swap(CurrentLayerIndex, DesiredLayerIndex);
                bTileMapChanged = true;
                break;
            }
        }
    }

    // Check if the number of occupied tiles in a layer has changed
    for (int32 LayerIndex = 0; LayerIndex < Layers.Num(); ++LayerIndex)
    {
        UTITileLayer* Layer = GetLayer(LayerIndex);
        if (Layer && Layer->GetNumOccupiedCells() != Layer->GetNumOccupiedTiles())
        {
            Layer->PostEditChangeProperty(PropertyChangedEvent);
            bTileMapChanged = true;
        }
    }

    if (bTileMapChanged)
    {
        SetFlags(RF_Transactional);
        Modify();
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

UTITileMap* UTITileMapLibrary::GetTileMapFromActor(const APaperTileMapActor* TileMapActor)
{
    UTITileMap* TileMap = nullptr;
    if (TileMapActor)
    {
        TileMap = Cast<UTITileMap>(TileMapActor->GetRenderComponent()->TileMap.Get());
    }

    return TileMap;
}

UTITileLayer* UTITileMapLibrary::GetTileLayerFromActor(const APaperTileMapActor* TileMapActor, int32 LayerIndex)
{
    UTITileLayer* Layer = nullptr;
    if (UTITileMap* TileMap = GetTileMapFromActor(TileMapActor))
    {
        Layer = TileMap->GetLayer(LayerIndex);
    }

    return Layer;
}