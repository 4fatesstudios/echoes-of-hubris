// Copyright Flekz Games. All Rights Reserved.

#include "TITileSet.h"

#include "Dom/JsonValue.h"
#include "TICustomProperties.h"
#include "TITile.h"

UTITileSet::UTITileSet(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
    CustomProperties = ObjectInitializer.CreateDefaultSubobject<UTICustomProperties>(this, TEXT("CustomProperties"));
}

UTITile* UTITileSet::GetTile(int32 TileIndex) const
{
    return Tiles.IsValidIndex(TileIndex) ? Tiles[TileIndex] : nullptr;
}

void UTITileSet::ForEachTile(TFunction<bool(UTITile*, uint32)> Callback, bool bReversed /*= false*/) const
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

void UTITileSet::InitializeTiles(TSubclassOf<UTITile> TileClass)
{
    const int32 NumTiles = GetTileCount();
    Tiles.Empty(NumTiles);

    for (int32 TileIndex = 0; TileIndex < NumTiles; TileIndex++)
    {
        UTITile* Tile = NewObject<UTITile>(this, TileClass);
        Tile->TileSet = this;
        Tile->TileIndex = TileIndex;
        Tiles.Add(Tile);
    }
}

void UTITileSet::LoadCustomProperties(const TArray<TSharedPtr<FJsonValue>>& Properties)
{
    CustomProperties->Load(Properties);
    OnCustomPropertiesLoaded(CustomProperties);
}

void UTITileSet::OnCustomPropertiesLoaded_Implementation(UTICustomProperties* Properties)
{

}

void UTITileSet::OnImportedBroadcast()
{
    for (auto* Tile : Tiles)
    {
        Tile->OnImported();
    }

    OnImported();
}

void UTITileSet::OnImported_Implementation()
{

}
