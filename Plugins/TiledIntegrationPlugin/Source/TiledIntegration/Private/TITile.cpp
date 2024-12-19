// Copyright Flekz Games. All Rights Reserved.

#include "TITile.h"

#include "Dom/JsonValue.h"
#include "TICustomProperties.h"
#include "TITileSet.h"

UTITile::UTITile(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, TileSet(nullptr)
, TileIndex(INDEX_NONE)
, Flipbook(nullptr)
{
    CustomProperties = ObjectInitializer.CreateDefaultSubobject<UTICustomProperties>(this, TEXT("CustomProperties"));
}

FName UTITile::GetUserData() const
{
    if (const FPaperTileMetadata* Metadata = GetMetadata())
    {
        return Metadata->UserDataName;
    }
    else
    {
        return NAME_None;
    }
}

UTITileSet* UTITile::GetTileSet() const
{
    return TileSet.IsValid() ? TileSet.Get() : nullptr;
}

int32 UTITile::GetTileIndex() const
{
    return TileIndex;
}

FIntPoint UTITile::GetSize() const
{
    return TileSet.IsValid() ? TileSet->GetTileSize() : FIntPoint();
}

bool UTITile::IsValid() const
{
    return TileSet.IsValid() && TileIndex != INDEX_NONE;
}

FPaperTileMetadata* UTITile::GetMutableMetadata()
{
    return TileSet.IsValid() ? TileSet->GetMutableTileMetadata(TileIndex) : nullptr;
}

const FPaperTileMetadata* UTITile::GetMetadata() const
{
    return TileSet.IsValid() ? TileSet->GetTileMetadata(TileIndex) : nullptr;
}

void UTITile::LoadCustomProperties(const TArray<TSharedPtr<FJsonValue>>& Properties)
{
    CustomProperties->Load(Properties);
    OnCustomPropertiesLoaded(CustomProperties);
}

void UTITile::OnCustomPropertiesLoaded_Implementation(UTICustomProperties* Properties)
{

}

void UTITile::OnImported_Implementation()
{

}