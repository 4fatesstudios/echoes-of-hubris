// Copyright Flekz Games. All Rights Reserved.

#include "TITileInstance.h"

#include "PaperFlipbook.h"
#include "TICustomProperties.h"
#include "TITile.h"
#include "TITileLayer.h"
#include "TITileMap.h"
#include "TITileMapActor.h"
#include "TITileSet.h"

UTITileInstance::UTITileInstance(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, TileLayer(nullptr)
, Coordinates(INDEX_NONE)
, Tile(nullptr)
, TileSoftPtr(nullptr)
, AnimatedTileTime(0.0f)
, AnimatedTileLastKeyFrame(0)
, bRenderDataDirty(false)
{
    CustomProperties = ObjectInitializer.CreateDefaultSubobject<UTICustomProperties>(this, TEXT("CustomProperties"));
}

UTICustomProperties* UTITileInstance::GetTileCustomProperties()
{
    return GetTile() ? Tile->GetCustomProperties() : nullptr;
}

UTITileSet* UTITileInstance::GetTileSet()
{
    return GetTile() ? Tile->GetTileSet() : nullptr;
}

int32 UTITileInstance::GetTileIndex()
{
    return GetTile() ? Tile->GetTileIndex() : INDEX_NONE;
}

UTITile* UTITileInstance::GetTile()
{
    if (Tile == nullptr)
    {
        Tile = TileSoftPtr.Get();
    }

    return Tile;
}

UTITileLayer* UTITileInstance::GetLayer() const
{
    return TileLayer.IsValid() ? TileLayer.Get() : nullptr;
}

UTITileMap* UTITileInstance::GetTileMap() const
{
    UTITileLayer* Layer = GetLayer();
    return Layer ? Layer->GetTileMap() : nullptr;
}

const FIntVector& UTITileInstance::GetTileCoordinates() const
{
    return Coordinates;
}

FVector UTITileInstance::GetTilePosition(ATITileMapActor* OwnerActor) const
{
    FVector Result;
    if (UTITileLayer* Layer = GetLayer())
    {
        if (UTITileMap* TileMap = Layer->GetTileMap())
        {
            Result = TileMap->GetTilePositionInLocalSpace(Coordinates.X, Coordinates.Y, Coordinates.Z);

            if (OwnerActor)
            {
                Result = OwnerActor->GetActorTransform().TransformPosition(Result);
            }
        }
    }

    return Result;
}

bool UTITileInstance::IsValid()
{
    return TileLayer.IsValid() && Coordinates.X != INDEX_NONE && GetTile();
}

bool UTITileInstance::IsAnimated()
{
    return GetTile() ? Tile->IsAnimated() : false;
}

UPaperFlipbook* UTITileInstance::GetFlipbook()
{
    return GetTile() ? Tile->GetFlipbook() : nullptr;
}

void UTITileInstance::Tick(float DeltaTime)
{
    // Update tile animation
    if (UPaperFlipbook* Flipbook = GetFlipbook())
    {
        const float FlipbookDuration = Flipbook->GetTotalDuration();
        AnimatedTileTime = AnimatedTileTime + DeltaTime <= FlipbookDuration ? AnimatedTileTime + DeltaTime : 0;

        const uint32 CurrentKeyFrame = Flipbook->GetKeyFrameIndexAtTime(AnimatedTileTime);
        if (CurrentKeyFrame != AnimatedTileLastKeyFrame)
        {
            bRenderDataDirty = true;
            AnimatedTileLastKeyFrame = CurrentKeyFrame;
        }
    }

    // TO DO: No need to tick if tile is outside camera viewport
    // ...
}

UPaperSprite* UTITileInstance::GetAnimatedTileSprite()
{
    UPaperSprite* Sprite = nullptr;
    if (UPaperFlipbook* Flipbook = GetFlipbook())
    {
        Sprite = Flipbook->GetSpriteAtTime(AnimatedTileTime, true);
    }

    return Sprite;
}

void UTITileInstance::LoadCustomProperties(const TArray<TSharedPtr<FJsonValue>>& Properties)
{
    CustomProperties->Load(Properties);
    OnCustomPropertiesLoaded(CustomProperties);
}

void UTITileInstance::OnCustomPropertiesLoaded_Implementation(UTICustomProperties* Properties)
{

}

void UTITileInstance::OnImported_Implementation()
{

}
