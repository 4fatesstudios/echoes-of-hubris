// Copyright Flekz Games. All Rights Reserved.

#pragma once
#include "PaperRenderSceneProxy.h"

class FMeshElementCollector;
class FPrimitiveDrawInterface;
class UTITileMap;
class UTITileMapComponent;

//////////////////////////////////////////////////////////////////////////
// FPaperTileMapRenderSceneProxy

class FTITileMapRenderSceneProxy final : public FPaperRenderSceneProxy
{
public:
    FTITileMapRenderSceneProxy(const UTITileMapComponent* InComponent);

    SIZE_T GetTypeHash() const override;

    // FPrimitiveSceneProxy interface.
    virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
    // End of FPrimitiveSceneProxy interface.

    void DrawBoundsForLayer(FPrimitiveDrawInterface* PDI, const FLinearColor& Color, int32 LayerIndex) const;
    void DrawNormalGridLines(FPrimitiveDrawInterface* PDI, const FLinearColor& PerTileColor, const FLinearColor& MultiTileColor, int32 MultiTileGridWidth, int32 MultiTileGridHeight, int32 MultiTileGridOffsetX, int32 MultiTileGridOffsetY, int32 LayerIndex) const;
    void DrawStaggeredGridLines(FPrimitiveDrawInterface* PDI, const FLinearColor& PerTileColor, const FLinearColor& MultiTileColor, int32 MultiTileGridWidth, int32 MultiTileGridHeight, int32 MultiTileGridOffsetX, int32 MultiTileGridOffsetY, int32 LayerIndex) const;
    void DrawHexagonalGridLines(FPrimitiveDrawInterface* PDI, const FLinearColor& PerTileColor, const FLinearColor& MultiTileColor, int32 MultiTileGridWidth, int32 MultiTileGridHeight, int32 MultiTileGridOffsetX, int32 MultiTileGridOffsetY, int32 LayerIndex) const;

    void BuildRenderData_RenderThread();
    void UpdateRenderData_RenderThread();

protected:
#if WITH_EDITOR
    bool bShowPerTileGridWhenSelected;
    bool bShowPerTileGridWhenUnselected;
    bool bShowPerLayerGridWhenSelected;
    bool bShowPerLayerGridWhenUnselected;
    bool bShowOutlineWhenUnselected;
#endif

    const UTITileMap* TileMap;
    const UTITileMapComponent* TileMapComponent;

    // The only layer to draw, or INDEX_NONE if the filter is unset
    const int32 OnlyLayerIndex;

    // Slight depth bias so that the wireframe grid overlay doesn't z-fight with the tiles themselves
    const float WireDepthBias;

    TMap<uint32, uint32> TileVertexIndexRelation;
};
