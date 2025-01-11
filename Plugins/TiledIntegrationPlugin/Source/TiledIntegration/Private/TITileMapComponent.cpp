// Copyright Flekz Games. All Rights Reserved.

#include "TITileMapComponent.h"

#include "PaperFlipbook.h"
#include "PaperFlipbookActor.h"
#include "PaperFlipbookComponent.h"
#include "TITile.h"
#include "TITileInstance.h"
#include "TITileMap.h"
#include "TITileLayer.h"
#include "TITileSet.h"
#include "TITileMapRenderSceneProxy.h"

DECLARE_CYCLE_STAT(TEXT("Rebuild Tile Map"), STAT_PaperRender_TileMapRebuild, STATGROUP_Paper2D);

UTITileMapComponent::UTITileMapComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_DuringPhysics;
    bTickInEditor = true;
}

UTITileMap* UTITileMapComponent::GetTileMap() const
{
    return Cast<UTITileMap>(TileMap);
}

UTITileInstance* UTITileMapComponent::GetTileInstance(const FIntVector& Coordinates) const
{
    UTITileMap* TITileMap = GetTileMap();
    UTITileLayer* TITileLayer = TITileMap ? TITileMap->GetLayer(Coordinates.Z) : nullptr;
    return TITileLayer ? TITileLayer->GetTile(Coordinates.X, Coordinates.Y) : nullptr;
}

#if WITH_EDITOR
void UTITileMapComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    CacheAnimatedTiles();
}
#endif

FPrimitiveSceneProxy* UTITileMapComponent::CreateSceneProxy()
{
    SCOPE_CYCLE_COUNTER(STAT_PaperRender_TileMapRebuild);
    CacheAnimatedTiles();
    FTITileMapRenderSceneProxy* Proxy = new FTITileMapRenderSceneProxy(this);

    ENQUEUE_RENDER_COMMAND(FSendPaperRenderComponentDynamicData)(
    [Proxy](FRHICommandListImmediate& RHICmdList)
    {
        Proxy->BuildRenderData_RenderThread();
    });

    return Proxy;
}

void UTITileMapComponent::TickComponent(float DeltaTime, enum ELevelTick, FActorComponentTickFunction*)
{
    // Don't tick if not on screen
    if (WasRecentlyRendered())
    {
        bool bRenderUpdateNeeded = false;
        for (const auto& AnimatedTile : AnimatedTiles)
        {
            if (AnimatedTile.IsValid())
            {
                AnimatedTile->Tick(DeltaTime);
                if (AnimatedTile->bRenderDataDirty)
                {
                    bRenderUpdateNeeded = true;
                }
            }
        }

        if (bRenderUpdateNeeded)
        {
            MarkRenderDynamicDataDirty();
        }
    }
}

void UTITileMapComponent::CacheAnimatedTiles()
{
    AnimatedTiles.Empty();

    UTITileMap* TITileMap = Cast<UTITileMap>(TileMap);
    if (TITileMap)
    {
        TITileMap->ForEachOccupiedTile([this](UTITileInstance* TileInstance, uint32) -> bool
        {
            if (TileInstance->IsAnimated())
            {
                AnimatedTiles.Add(TileInstance);
            }

            return true;
        });
    }
}

void UTITileMapComponent::SendRenderDynamicData_Concurrent()
{
    if (SceneProxy != nullptr)
    {
        FTITileMapRenderSceneProxy* InSceneProxy = (FTITileMapRenderSceneProxy*)SceneProxy;
        ENQUEUE_RENDER_COMMAND(FSendPaperRenderComponentDynamicData)(
        [InSceneProxy](FRHICommandListImmediate& RHICmdList)
        {
            InSceneProxy->UpdateRenderData_RenderThread();
        });
    }
}
