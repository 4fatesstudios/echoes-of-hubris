// Copyright Epic Games, Inc. All Rights Reserved.

#include "TITileMapRenderSceneProxy.h"

#include "Engine/Engine.h"
#include "Materials/Material.h"
#include "Materials/MaterialRenderProxy.h"
#include "MaterialDomain.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "PhysicsEngine/BodySetup.h"
#include "SceneInterface.h"
#include "SceneManagement.h"
#include "TITile.h"
#include "TITileInstance.h"
#include "TITileLayer.h"
#include "TITileMap.h"
#include "TITileMapComponent.h"
#include "TITileSet.h"

DECLARE_CYCLE_STAT(TEXT("Tile Map Proxy"), STAT_TileMap_GetDynamicMeshElements, STATGROUP_Paper2D);
DECLARE_CYCLE_STAT(TEXT("Tile Map Editor Grid"), STAT_TileMap_EditorWireDrawing, STATGROUP_Paper2D);

FTITileMapRenderSceneProxy::FTITileMapRenderSceneProxy(const UTITileMapComponent* InComponent)
: FPaperRenderSceneProxy(InComponent)
#if WITH_EDITOR
, bShowPerTileGridWhenSelected(false)
, bShowPerTileGridWhenUnselected(false)
, bShowPerLayerGridWhenSelected(false)
, bShowPerLayerGridWhenUnselected(false)
, bShowOutlineWhenUnselected(false)
#endif
, TileMap(nullptr)
, TileMapComponent(InComponent)
, OnlyLayerIndex(/*InComponent->bUseSingleLayer ? InComponent->UseSingleLayerIndex :*/ INDEX_NONE)
, WireDepthBias(0.0001f)
{
    check(InComponent);

    SetWireframeColor(InComponent->GetWireframeColor());
    TileMap = Cast<UTITileMap>(InComponent->TileMap);
    MaterialRelevance = InComponent->GetMaterialRelevance(GetScene().GetFeatureLevel());

#if WITH_EDITORONLY_DATA
    bShowPerTileGridWhenSelected = InComponent->bShowPerTileGridWhenSelected;
    bShowPerTileGridWhenUnselected = InComponent->bShowPerTileGridWhenUnselected;
    bShowPerLayerGridWhenSelected = InComponent->bShowPerLayerGridWhenSelected;
    bShowPerLayerGridWhenUnselected = InComponent->bShowPerLayerGridWhenUnselected;
    bShowOutlineWhenUnselected = InComponent->bShowOutlineWhenUnselected;
#endif
}

SIZE_T FTITileMapRenderSceneProxy::GetTypeHash() const
{
    static size_t UniquePointer;
    return reinterpret_cast<size_t>(&UniquePointer);
}

void FTITileMapRenderSceneProxy::DrawBoundsForLayer(FPrimitiveDrawInterface* PDI, const FLinearColor& Color, int32 LayerIndex) const
{
    const FMatrix& LocalToWorldMat = GetLocalToWorld();
    const FVector TL(LocalToWorldMat.TransformPosition(TileMap->GetTilePositionInLocalSpace(0, 0, LayerIndex)));
    const FVector TR(LocalToWorldMat.TransformPosition(TileMap->GetTilePositionInLocalSpace(TileMap->MapWidth, 0, LayerIndex)));
    const FVector BL(LocalToWorldMat.TransformPosition(TileMap->GetTilePositionInLocalSpace(0, TileMap->MapHeight, LayerIndex)));
    const FVector BR(LocalToWorldMat.TransformPosition(TileMap->GetTilePositionInLocalSpace(TileMap->MapWidth, TileMap->MapHeight, LayerIndex)));

    PDI->DrawLine(TL, TR, Color, SDPG_Foreground, 0.0f, WireDepthBias);
    PDI->DrawLine(TR, BR, Color, SDPG_Foreground, 0.0f, WireDepthBias);
    PDI->DrawLine(BR, BL, Color, SDPG_Foreground, 0.0f, WireDepthBias);
    PDI->DrawLine(BL, TL, Color, SDPG_Foreground, 0.0f, WireDepthBias);
}

void FTITileMapRenderSceneProxy::DrawNormalGridLines(FPrimitiveDrawInterface* PDI, const FLinearColor& PerTileColor, const FLinearColor& MultiTileColor, int32 MultiTileGridWidth, int32 MultiTileGridHeight, int32 MultiTileGridOffsetX, int32 MultiTileGridOffsetY, int32 LayerIndex) const
{
    FLinearColor Color = PerTileColor;
    const FMatrix& LocalToWorldMat = GetLocalToWorld();
    const uint8 DPG = SDPG_Foreground;//GetDepthPriorityGroup(View);

    // Draw horizontal lines on the selection
    for (int32 Y = 0; Y <= TileMap->MapHeight; ++Y)
    {
        int32 X = 0;
        const FVector Start(TileMap->GetTilePositionInLocalSpace(X, Y, LayerIndex));

        X = TileMap->MapWidth;
        const FVector End(TileMap->GetTilePositionInLocalSpace(X, Y, LayerIndex));

        if (MultiTileGridHeight > 0 && int32(Y - MultiTileGridOffsetY) % int32(MultiTileGridHeight) == 0)
        {
            Color = MultiTileColor;
        }
        else
        {
            Color = PerTileColor;
        }

        PDI->DrawLine(LocalToWorldMat.TransformPosition(Start), LocalToWorldMat.TransformPosition(End), Color, DPG, 0.0f, WireDepthBias);
    }

    // Draw vertical lines
    for (int32 X = 0; X <= TileMap->MapWidth; ++X)
    {
        int32 Y = 0;
        const FVector Start(TileMap->GetTilePositionInLocalSpace(X, Y, LayerIndex));

        Y = TileMap->MapHeight;
        const FVector End(TileMap->GetTilePositionInLocalSpace(X, Y, LayerIndex));

        if (MultiTileGridWidth > 0 && int32(X - MultiTileGridOffsetX) % int32(MultiTileGridWidth) == 0)
        {
            Color = MultiTileColor;
        }
        else
        {
            Color = PerTileColor;
        }

        PDI->DrawLine(LocalToWorldMat.TransformPosition(Start), LocalToWorldMat.TransformPosition(End), Color, DPG, 0.0f, WireDepthBias);
    }
}

void FTITileMapRenderSceneProxy::DrawStaggeredGridLines(FPrimitiveDrawInterface* PDI, const FLinearColor& PerTileColor, const FLinearColor& MultiTileColor, int32 MultiTileGridWidth, int32 MultiTileGridHeight, int32 MultiTileGridOffsetX, int32 MultiTileGridOffsetY, int32 LayerIndex) const
{
    FLinearColor Color = PerTileColor;
    TArray<FVector> Poly;
    Poly.Empty(4);

    const FMatrix& LocalToWorldMat = GetLocalToWorld();
    const uint8 DPG = SDPG_Foreground;//GetDepthPriorityGroup(View);

    FVector CornerPosition;
    FVector OffsetYFactor;
    FVector StepX;
    FVector StepY;

    TileMap->GetTileToLocalParameters(/*out*/ CornerPosition, /*out*/ StepX, /*out*/ StepY, /*out*/ OffsetYFactor);

    const FVector PartialZ = (TileMap->SeparationPerLayer * LayerIndex) * PaperAxisZ;
    const FVector TotalOffset = CornerPosition + PartialZ;

    const bool bStaggerEven = false;

    const FVector TopCenterStart = TotalOffset + StepX * 0.5f + (StepX * -0.5f) + StepY;
    for (int32 X = 0 - ((TileMap->MapHeight + 1) / 2); X < TileMap->MapWidth; ++X)
    {
        int32 XTop = FMath::Max(X, 0);
        int32 YTop = FMath::Max(-2 * X, 0);

        if (X < 0)
        {
            XTop--;
            YTop--;
        }

        // A is top of center top row cell
        Poly.Reset();
        TileMap->GetTilePolygon(XTop, YTop, LayerIndex, Poly);
        const FVector LSA = Poly[0];

        // Determine the bottom row cell
        int32 YBottom = TileMap->MapHeight - 1;
        int32 XBottom = X + (TileMap->MapHeight + 1) / 2;
        const int32 XExcess = FMath::Max(XBottom - TileMap->MapWidth, 0);
        XBottom -= XExcess;
        YBottom -= XExcess * 2;

        if (XBottom == TileMap->MapWidth)
        {
            YBottom -= ((TileMap->MapHeight & 1) != 0) ? 0 : 1;
        }

        // Bottom center
        Poly.Reset();
        TileMap->GetTilePolygon(XBottom, YBottom, LayerIndex, Poly);
        const FVector LSB = Poly[2];

        if (MultiTileGridHeight > 0 && int32(X - MultiTileGridOffsetY) % int32(MultiTileGridHeight) == 0)
        {
            Color = MultiTileColor;
        }
        else
        {
            Color = PerTileColor;
        }

        PDI->DrawLine(LocalToWorldMat.TransformPosition(LSA), LocalToWorldMat.TransformPosition(LSB), Color, DPG, 0.0f, WireDepthBias);
    }

    for (int32 X = 0; X < TileMap->MapWidth + ((TileMap->MapHeight + 1) / 2) + 1; ++X)
    {
        const int32 XTop = FMath::Min(X, TileMap->MapWidth);
        const int32 YTop = FMath::Max(2 * (X - TileMap->MapWidth), 0);

        // A is top center of top row cell
        Poly.Reset();
        TileMap->GetTilePolygon(XTop, YTop, LayerIndex, Poly);
        const FVector LSA = Poly[0];

        // Determine the bottom row cell
        int32 YBottom = TileMap->MapHeight;
        int32 XBottom = X - ((TileMap->MapHeight + 1) / 2);
        const int32 XExcess = FMath::Max(-XBottom, 0);
        XBottom += XExcess;
        YBottom -= XExcess * 2;

        if (XExcess > 0)
        {
            YBottom += (TileMap->MapHeight & 1);
        }

        // Bottom left
        Poly.Reset();
        TileMap->GetTilePolygon(XBottom, YBottom, LayerIndex, Poly);
        const FVector LSB = Poly[3];

        if (MultiTileGridWidth > 0 && int32(X - MultiTileGridOffsetX) % int32(MultiTileGridWidth) == 0)
        {
            Color = MultiTileColor;
        }
        else
        {
            Color = PerTileColor;
        }

        PDI->DrawLine(LocalToWorldMat.TransformPosition(LSA), LocalToWorldMat.TransformPosition(LSB), Color, DPG, 0.0f, WireDepthBias);
    }
}

void FTITileMapRenderSceneProxy::DrawHexagonalGridLines(FPrimitiveDrawInterface* PDI, const FLinearColor& PerTileColor, const FLinearColor& MultiTileColor, int32 MultiTileGridWidth, int32 MultiTileGridHeight, int32 MultiTileGridOffsetX, int32 MultiTileGridOffsetY, int32 LayerIndex) const
{
    FLinearColor Color = FLinearColor::White;
    //@TODO: This isn't very efficient
    const FMatrix& LocalToWorldMat = GetLocalToWorld();
    const uint8 DPG = SDPG_Foreground;//GetDepthPriorityGroup(View);

    TArray<FVector> Poly;
    Poly.Empty(6);
    for (int32 Y = 0; Y < TileMap->MapHeight; ++Y)
    {
        for (int32 X = 0; X < TileMap->MapWidth; ++X)
        {
            Poly.Reset();
            TileMap->GetTilePolygon(X, Y, LayerIndex, Poly);

            FVector LastVertexWS = LocalToWorldMat.TransformPosition(Poly[5]);
            for (int32 VI = 0; VI < Poly.Num(); ++VI)
            {
                FVector ThisVertexWS = LocalToWorldMat.TransformPosition(Poly[VI]);
                PDI->DrawLine(LastVertexWS, ThisVertexWS, Color, DPG, 0.0f, WireDepthBias);
                LastVertexWS = ThisVertexWS;
            }
        }
    }
}

void FTITileMapRenderSceneProxy::BuildRenderData_RenderThread()
{
    if (TileMap == nullptr || TileMapComponent == nullptr)
    {
        return;
    }

    // Handles the rotation and flipping of UV coordinates in a tile
    // 0123 = BL BR TR TL
    const static uint8 PermutationTable[8][4] =
    {
        {0, 1, 2, 3}, // 000 - normal
        {2, 1, 0, 3}, // 001 - diagonal
        {3, 2, 1, 0}, // 010 - flip Y
        {3, 0, 1, 2}, // 011 - diagonal then flip Y
        {1, 0, 3, 2}, // 100 - flip X
        {1, 2, 3, 0}, // 101 - diagonal then flip X
        {2, 3, 0, 1}, // 110 - flip X and flip Y
        {0, 3, 2, 1}  // 111 - diagonal then flip X and Y
    };

    FVector CornerOffset;
    FVector OffsetYFactor;
    FVector StepPerTileX;
    FVector StepPerTileY;
    TileMap->GetTileToLocalParameters(CornerOffset, StepPerTileX, StepPerTileY, OffsetYFactor);

    UTexture2D* LastSourceTexture = nullptr;
    FVector TileSetOffset = FVector::ZeroVector;
    FVector2f InverseTextureSize(1.0f, 1.0f);
    FVector2f SourceDimensionsUV(1.0f, 1.0f);
    FVector2f TileSizeXY(0.0f, 0.0f);

    const float UnrealUnitsPerPixel = TileMap->GetUnrealUnitsPerPixel();

    // TO DO: Get this properties from the tilemap component
    const bool bUseSingleLayerCheck = false;
    const uint32 SingleLayerIndex = INDEX_NONE;

    // Run thru the layers and estimate how big of an allocation we will need
    int32 EstimatedNumVerts = 0;

    for (int32 Z = TileMap->TileLayers.Num() - 1; Z >= 0; --Z)
    {
        UPaperTileLayer* Layer = TileMap->TileLayers[Z];

        if ((Layer != nullptr) && (!bUseSingleLayerCheck || (Z == SingleLayerIndex)))
        {
            const int32 NumOccupiedCells = Layer->GetNumOccupiedCells();
            EstimatedNumVerts += 6 * NumOccupiedCells;
        }
    }

    TileVertexIndexRelation.Empty(EstimatedNumVerts / 6);
    Vertices.Empty(EstimatedNumVerts);
    BatchedSections.Empty();

    UMaterialInterface* TileMapMaterial = TileMapComponent->GetMaterial(0);
    if (TileMapMaterial == nullptr)
    {
        TileMapMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
    }

    // Actual pass
    for (int32 Z = TileMap->TileLayers.Num() - 1; Z >= 0; --Z)
    {
        UTITileLayer* Layer = TileMap->GetLayer(Z);
        if (Layer == nullptr)
        {
            continue;
        }

        if (bUseSingleLayerCheck)
        {
            if (Z != SingleLayerIndex)
            {
                continue;
            }
        }

        const uint32 TileLayerIndex = Layer->GetLayerIndex() * Layer->GetLayerWidth() * Layer->GetLayerHeight();
        const FLinearColor DrawColorLinear = TileMapComponent->GetTileMapColor() * Layer->GetLayerColor();
        const FColor DrawColor(DrawColorLinear.ToFColor(/*bSRGB=*/ false));

#if WITH_EDITORONLY_DATA
        if (!Layer->ShouldRenderInEditor())
        {
            continue;
        }
#endif

        bool bNewBatch = true;
        FSpriteRenderSection* CurrentBatch = nullptr;
        FVector CurrentDestinationOrigin;
        const FPaperTileInfo* CurrentCellPtr = Layer->PRIVATE_GetAllocatedCells();
        check(Layer->GetLayerWidth() == TileMap->MapWidth);
        check(Layer->GetLayerHeight() == TileMap->MapHeight);

        for (int32 Y = 0; Y < TileMap->MapHeight; ++Y)
        {
            // In pixels
            FVector EffectiveTopLeftCorner;

            switch (TileMap->ProjectionMode)
            {
            case ETileMapProjectionMode::Orthogonal:
            default:
                EffectiveTopLeftCorner = CornerOffset;
                break;
            case ETileMapProjectionMode::IsometricDiamond:
                EffectiveTopLeftCorner = CornerOffset - 0.5f * StepPerTileX + 0.5f * StepPerTileY;
                break;
            case ETileMapProjectionMode::IsometricStaggered:
            case ETileMapProjectionMode::HexagonalStaggered:
                EffectiveTopLeftCorner = CornerOffset + (Y & 1) * OffsetYFactor;
                break;
            }

            for (int32 X = 0; X < TileMap->MapWidth; ++X)
            {
                const FPaperTileInfo& TileInfo = *CurrentCellPtr++;

                const float TotalSeparation = (TileMap->SeparationPerLayer * Z) + (TileMap->SeparationPerTileX * X) + (TileMap->SeparationPerTileY * Y);
                FVector TopLeftCornerOfTile = (StepPerTileX * X) + (StepPerTileY * Y) + EffectiveTopLeftCorner;
                TopLeftCornerOfTile += TotalSeparation * PaperAxisZ;

                const int32 TileWidth = TileMap->TileWidth;
                const int32 TileHeight = TileMap->TileHeight;

                UTITileInstance* TileInstance = Layer->GetTile(X, Y);
                UTITile* Tile = TileInstance ? TileInstance->GetTile() : nullptr;
                UTITileSet* TileSet = Tile ? Tile->GetTileSet() : nullptr;

                if (Tile && TileInstance && TileSet)
                {
                    UPaperSprite* Sprite = nullptr;
                    UTexture2D* SourceTexture = nullptr;
                    if (Tile->IsAnimated())
                    {
                        Sprite = TileInstance->GetAnimatedTileSprite();
                        if (Sprite)
                        {
                            SourceTexture = Sprite->GetBakedTexture();
                        }
                    }
                    else
                    {
                        SourceTexture = TileSet->GetTileSheetTexture();
                    }

                    if (SourceTexture == nullptr)
                    {
                        continue;
                    }

                    if ((SourceTexture != LastSourceTexture) || (CurrentBatch == nullptr))
                    {
                        CurrentBatch = new (BatchedSections) FSpriteRenderSection();
                        CurrentBatch->BaseTexture = SourceTexture;
                        CurrentBatch->AdditionalTextures = TileSet->GetAdditionalTextures();
                        CurrentBatch->Material = TileMapMaterial;
                        CurrentBatch->VertexOffset = Vertices.Num();
                        CurrentDestinationOrigin = TopLeftCornerOfTile.ProjectOnTo(PaperAxisZ);
                        bNewBatch = true;
                    }

                    const uint32 TileIndex = X + (Y * Layer->GetLayerWidth()) + TileLayerIndex;
                    if (Tile->IsAnimated())
                    {
                        if (Sprite)
                        {
                            LastSourceTexture = SourceTexture;
                            FSpriteDrawCallRecord DrawCall; 
                            DrawCall.BuildFromSprite(Sprite);
                            DrawCall.Color = DrawColor;
                            DrawCall.Destination = TileInstance->GetTilePosition();

                            const FVector4& BottomRight = DrawCall.RenderVerts[3];
                            const FVector4& TopRight = DrawCall.RenderVerts[0];
                            const FVector4& BottomLeft = DrawCall.RenderVerts[2];
                            const FVector4& TopLeft = DrawCall.RenderVerts[1];

                            CurrentBatch->AddVertex(BottomRight.X, BottomRight.Y, BottomRight.Z, BottomRight.W, DrawCall.Destination, DrawColor, Vertices);
                            CurrentBatch->AddVertex(TopRight.X, TopRight.Y, TopRight.Z, TopRight.W, DrawCall.Destination, DrawColor, Vertices);
                            CurrentBatch->AddVertex(BottomLeft.X, BottomLeft.Y, BottomLeft.Z, BottomLeft.W, DrawCall.Destination, DrawColor, Vertices);

                            CurrentBatch->AddVertex(TopRight.X, TopRight.Y, TopRight.Z, TopRight.W, DrawCall.Destination, DrawColor, Vertices);
                            CurrentBatch->AddVertex(TopLeft.X, TopLeft.Y, TopLeft.Z, TopLeft.W, DrawCall.Destination, DrawColor, Vertices);
                            CurrentBatch->AddVertex(BottomLeft.X, BottomLeft.Y, BottomLeft.Z, BottomLeft.W, DrawCall.Destination, DrawColor, Vertices);
                            
                            TileVertexIndexRelation.Add(TileIndex, Vertices.Num() - 6);
                        }
                    }
                    else
                    {
                        FVector2D SourceUVDbl = FVector2D::ZeroVector;
                        if (!TileSet->GetTileUV(Tile->GetTileIndex(), SourceUVDbl))
                        {
                            continue;
                        }

                        if (SourceTexture != LastSourceTexture || bNewBatch)
                        {
                            const FVector2f TextureSize(SourceTexture->GetImportedSize());
                            InverseTextureSize = FVector2f(1.0f / TextureSize.X, 1.0f / TextureSize.Y);

                            if (TileSet != nullptr)
                            {
                                const FIntPoint TileSetTileSize(TileSet->GetTileSize());

                                SourceDimensionsUV = FVector2f(TileSetTileSize.X * InverseTextureSize.X, TileSetTileSize.Y * InverseTextureSize.Y);
                                TileSizeXY = FVector2f(UnrealUnitsPerPixel * TileSetTileSize.X, UnrealUnitsPerPixel * TileSetTileSize.Y);

                                const FIntPoint TileSetDrawingOffset = TileSet->GetDrawingOffset();
                                const float HorizontalCellOffset = TileSetDrawingOffset.X * UnrealUnitsPerPixel;
                                const float VerticalCellOffset = (-TileSetDrawingOffset.Y - TileHeight + TileSetTileSize.Y) * UnrealUnitsPerPixel;
                                TileSetOffset = (HorizontalCellOffset * PaperAxisX) + (VerticalCellOffset * PaperAxisY);
                            }
                            else
                            {
                                SourceDimensionsUV = FVector2f(TileWidth * InverseTextureSize.X, TileHeight * InverseTextureSize.Y);
                                TileSizeXY = FVector2f(UnrealUnitsPerPixel * TileWidth, UnrealUnitsPerPixel * TileHeight);
                                TileSetOffset = FVector::ZeroVector;
                            }

                            bNewBatch = false;
                            LastSourceTexture = SourceTexture;
                        }

                        TopLeftCornerOfTile += TileSetOffset;

                        FVector2f SourceUV = static_cast<FVector2f>(SourceUVDbl);
                        SourceUV.X *= InverseTextureSize.X;
                        SourceUV.Y *= InverseTextureSize.Y;

                        const float WX0 = FVector::DotProduct(TopLeftCornerOfTile, PaperAxisX);
                        const float WY0 = FVector::DotProduct(TopLeftCornerOfTile, PaperAxisY);

                        const int32 Flags = TileInfo.GetFlagsAsIndex();

                        const FVector2f TileSizeWithFlip = TileInfo.HasFlag(EPaperTileFlags::FlipDiagonal) ? FVector2f(TileSizeXY.Y, TileSizeXY.X) : TileSizeXY;
                        const float UValues[4] = { SourceUV.X, SourceUV.X + SourceDimensionsUV.X, SourceUV.X + SourceDimensionsUV.X, SourceUV.X };
                        const float VValues[4] = { SourceUV.Y + SourceDimensionsUV.Y, SourceUV.Y + SourceDimensionsUV.Y, SourceUV.Y, SourceUV.Y };

                        const uint8 UVIndex0 = PermutationTable[Flags][0];
                        const uint8 UVIndex1 = PermutationTable[Flags][1];
                        const uint8 UVIndex2 = PermutationTable[Flags][2];
                        const uint8 UVIndex3 = PermutationTable[Flags][3];

                        const FVector4f BottomLeft(WX0, WY0 - TileSizeWithFlip.Y, UValues[UVIndex0], VValues[UVIndex0]);
                        const FVector4f BottomRight(WX0 + TileSizeWithFlip.X, WY0 - TileSizeWithFlip.Y, UValues[UVIndex1], VValues[UVIndex1]);
                        const FVector4f TopRight(WX0 + TileSizeWithFlip.X, WY0, UValues[UVIndex2], VValues[UVIndex2]);
                        const FVector4f TopLeft(WX0, WY0, UValues[UVIndex3], VValues[UVIndex3]);

                        CurrentBatch->AddVertex(BottomRight.X, BottomRight.Y, BottomRight.Z, BottomRight.W, CurrentDestinationOrigin, DrawColor, Vertices);
                        CurrentBatch->AddVertex(TopRight.X, TopRight.Y, TopRight.Z, TopRight.W, CurrentDestinationOrigin, DrawColor, Vertices);
                        CurrentBatch->AddVertex(BottomLeft.X, BottomLeft.Y, BottomLeft.Z, BottomLeft.W, CurrentDestinationOrigin, DrawColor, Vertices);

                        CurrentBatch->AddVertex(TopRight.X, TopRight.Y, TopRight.Z, TopRight.W, CurrentDestinationOrigin, DrawColor, Vertices);
                        CurrentBatch->AddVertex(TopLeft.X, TopLeft.Y, TopLeft.Z, TopLeft.W, CurrentDestinationOrigin, DrawColor, Vertices);
                        CurrentBatch->AddVertex(BottomLeft.X, BottomLeft.Y, BottomLeft.Z, BottomLeft.W, CurrentDestinationOrigin, DrawColor, Vertices);
                        
                        TileVertexIndexRelation.Add(TileIndex, Vertices.Num() - 6);
                    }
                }
            }
        }
    }

    RecreateCachedRenderData(FRHICommandListImmediate::Get());
}

void FTITileMapRenderSceneProxy::UpdateRenderData_RenderThread()
{
    // TO DO: We could probably optimize this by caching the animated tiles (or maybe do we want to re render other kind of tiles too?)
    bool bRenderDataUpdated = false;
    for (int32 Z = TileMap->TileLayers.Num() - 1; Z >= 0; --Z)
    {
        UTITileLayer* Layer = TileMap->GetLayer(Z);
        const uint32 TileLayerIndex = Layer->GetLayerIndex() * Layer->GetLayerWidth() * Layer->GetLayerHeight();
        const FLinearColor DrawColorLinear = TileMapComponent->GetTileMapColor() * Layer->GetLayerColor();
        const FColor DrawColor(DrawColorLinear.ToFColor(false));

        for (int32 Y = 0; Y < TileMap->MapHeight; ++Y)
        {
            for (int32 X = 0; X < TileMap->MapWidth; ++X)
            {
                UTITileInstance* TileInstance = Layer->GetTile(X, Y);
                if (TileInstance && TileInstance->bRenderDataDirty)
                {
                    if (TileInstance->IsAnimated())
                    {
                        if (UPaperSprite* Sprite = TileInstance->GetAnimatedTileSprite())
                        {
                            const uint32 TileIndex = X + (Y * Layer->GetLayerWidth()) + TileLayerIndex;
                            if (const auto* TileVertexIndex = TileVertexIndexRelation.Find(TileIndex))
                            {
                                FSpriteDrawCallRecord DrawCall;
                                DrawCall.BuildFromSprite(Sprite);
                                DrawCall.Destination = TileInstance->GetTilePosition();

                                const FVector4& TopRight = DrawCall.RenderVerts[0];
                                const FVector4& TopLeft = DrawCall.RenderVerts[1];
                                const FVector4& BottomLeft = DrawCall.RenderVerts[2];
                                const FVector4& BottomRight = DrawCall.RenderVerts[3];

                                const uint32 VertexStartIndex = *TileVertexIndex;
                                Vertices[VertexStartIndex + 0] = FDynamicMeshVertex(FVector3f((PaperAxisX * BottomRight.X) + (PaperAxisY * BottomRight.Y) + DrawCall.Destination), FPaperSpriteTangents::PackedNormalX.ToFVector3f(), FPaperSpriteTangents::PackedNormalZ.ToFVector3f(), FVector2f(BottomRight.Z, BottomRight.W), DrawColor);
                                Vertices[VertexStartIndex + 1] = FDynamicMeshVertex(FVector3f((PaperAxisX * TopRight.X) + (PaperAxisY * TopRight.Y) + DrawCall.Destination), FPaperSpriteTangents::PackedNormalX.ToFVector3f(), FPaperSpriteTangents::PackedNormalZ.ToFVector3f(), FVector2f(TopRight.Z, TopRight.W), DrawColor);
                                Vertices[VertexStartIndex + 2] = FDynamicMeshVertex(FVector3f((PaperAxisX * BottomLeft.X) + (PaperAxisY * BottomLeft.Y) + DrawCall.Destination), FPaperSpriteTangents::PackedNormalX.ToFVector3f(), FPaperSpriteTangents::PackedNormalZ.ToFVector3f(), FVector2f(BottomLeft.Z, BottomLeft.W), DrawColor);

                                Vertices[VertexStartIndex + 3] = Vertices[VertexStartIndex + 1];
                                Vertices[VertexStartIndex + 4] = FDynamicMeshVertex(FVector3f((PaperAxisX * TopLeft.X) + (PaperAxisY * TopLeft.Y) + DrawCall.Destination), FPaperSpriteTangents::PackedNormalX.ToFVector3f(), FPaperSpriteTangents::PackedNormalZ.ToFVector3f(), FVector2f(TopLeft.Z, TopLeft.W), DrawColor);
                                Vertices[VertexStartIndex + 5] = Vertices[VertexStartIndex + 2];

                                TileInstance->bRenderDataDirty = false;
                                bRenderDataUpdated = true;
                            }
                        }
                    }
                }
            }
        }
    }

    if (bRenderDataUpdated)
    {
        RecreateCachedRenderData(FRHICommandListImmediate::Get());
    }
}

void FTITileMapRenderSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
    SCOPE_CYCLE_COUNTER(STAT_TileMap_GetDynamicMeshElements);

    for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
    {
        SCOPE_CYCLE_COUNTER(STAT_TileMap_EditorWireDrawing);

        if (VisibilityMap & (1 << ViewIndex))
        {
            const FSceneView* View = Views[ViewIndex];
            FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

            // Draw the tile maps
            //@TODO: RenderThread race condition
            if (TileMap != nullptr)
            {
                if ((View->Family->EngineShowFlags.Collision /*@TODO: && bIsCollisionEnabled*/) && AllowDebugViewmodes())
                {
                    if (UBodySetup* BodySetup = TileMap->BodySetup)
                    {
                        if (FMath::Abs(GetLocalToWorld().Determinant()) < SMALL_NUMBER)
                        {
                            // Catch this here or otherwise GeomTransform below will assert
                            // This spams so commented out
                            //UE_LOG(LogStaticMesh, Log, TEXT("Zero scaling not supported (%s)"), *StaticMesh->GetPathName());
                        }
                        else
                        {
                            // Make a material for drawing solid collision stuff
                            const UMaterial* LevelColorationMaterial = View->Family->EngineShowFlags.Lighting
                                ? GEngine->ShadedLevelColorationLitMaterial : GEngine->ShadedLevelColorationUnlitMaterial;

                            auto CollisionMaterialInstance = new FColoredMaterialRenderProxy(
                                LevelColorationMaterial->GetRenderProxy(),
                                GetWireframeColor()
                            );

                            // Draw the static mesh's body setup.

                            // Get transform without scaling.
                            FTransform GeomTransform(GetLocalToWorld());

                            // In old wireframe collision mode, always draw the wireframe highlighted (selected or not).
                            bool bDrawWireSelected = IsSelected();
                            if (View->Family->EngineShowFlags.Collision)
                            {
                                bDrawWireSelected = true;
                            }

                            // Differentiate the color based on bBlockNonZeroExtent.  Helps greatly with skimming a level for optimization opportunities.
                            FColor CollisionColor = FColor(157, 149, 223, 255);

                            const bool bPerHullColor = false;
                            const bool bDrawSimpleSolid = false;
                            BodySetup->AggGeom.GetAggGeom(GeomTransform, GetSelectionColor(CollisionColor, bDrawWireSelected, IsHovered()).ToFColor(true), CollisionMaterialInstance, bPerHullColor, bDrawSimpleSolid, AlwaysHasVelocity(), ViewIndex, Collector);
                        }
                    }
                }

                // Draw the bounds
                RenderBounds(PDI, View->Family->EngineShowFlags, GetBounds(), IsSelected());

#if WITH_EDITOR
                const bool bShowAsSelected = IsSelected();
                const bool bEffectivelySelected = bShowAsSelected || IsHovered();

                const uint8 DPG = SDPG_Foreground;//GetDepthPriorityGroup(View);


                // Draw the debug outline
                if (bEffectivelySelected)
                {
                    const int32 SelectedLayerIndex = (OnlyLayerIndex != INDEX_NONE) ? OnlyLayerIndex : TileMap->SelectedLayerIndex;

                    // Draw separation wires if selected
                    const FLinearColor OverrideColor = GetSelectionColor(FLinearColor::White, bShowAsSelected, IsHovered(), /*bUseOverlayIntensity=*/ false);

                    if (bShowPerLayerGridWhenSelected)
                    {
                        if (OnlyLayerIndex == INDEX_NONE)
                        {
                            // Draw a bound for every layer but the selected one (and even that one if the per-tile grid is off)
                            for (int32 LayerIndex = 0; LayerIndex < TileMap->TileLayers.Num(); ++LayerIndex)
                            {
                                if ((LayerIndex != SelectedLayerIndex) || !bShowPerTileGridWhenSelected)
                                {
                                    DrawBoundsForLayer(PDI, OverrideColor, LayerIndex);
                                }
                            }
                        }
                        else if (!bShowPerTileGridWhenSelected)
                        {
                            DrawBoundsForLayer(PDI, OverrideColor, OnlyLayerIndex);
                        }
                    }

                    if (bShowPerTileGridWhenSelected && (SelectedLayerIndex != INDEX_NONE))
                    {
                        switch (TileMap->ProjectionMode)
                        {
                        default:
                        case ETileMapProjectionMode::Orthogonal:
                        case ETileMapProjectionMode::IsometricDiamond:
                            DrawNormalGridLines(PDI, OverrideColor, OverrideColor, 0, 0, 0, 0, SelectedLayerIndex);
                            break;
                        case ETileMapProjectionMode::IsometricStaggered:
                            DrawStaggeredGridLines(PDI, OverrideColor, OverrideColor, 0, 0, 0, 0, SelectedLayerIndex);
                            break;
                        case ETileMapProjectionMode::HexagonalStaggered:
                            DrawHexagonalGridLines(PDI, OverrideColor, OverrideColor, 0, 0, 0, 0, SelectedLayerIndex);
                            break;
                        }
                    }
                }
                else if (View->Family->EngineShowFlags.Grid)
                {
                    const int32 SelectedLayerIndex = (OnlyLayerIndex != INDEX_NONE) ? OnlyLayerIndex : TileMap->SelectedLayerIndex;

                    if (bShowOutlineWhenUnselected)
                    {
                        // Draw a layer rectangle even when not selected, so you can see where the tile map is in the editor
                        DrawBoundsForLayer(PDI, GetWireframeColor(), /*LayerIndex=*/ (OnlyLayerIndex != INDEX_NONE) ? OnlyLayerIndex : 0);
                    }
                    if (bShowPerLayerGridWhenUnselected)
                    {
                        const FLinearColor LayerGridColor = TileMap->LayerGridColor;

                        if (OnlyLayerIndex == INDEX_NONE)
                        {
                            // Draw a bound for every layer but the selected one (and even that one if the per-tile grid is off)
                            for (int32 LayerIndex = 0; LayerIndex < TileMap->TileLayers.Num(); ++LayerIndex)
                            {
                                if ((LayerIndex != SelectedLayerIndex) || !bShowPerTileGridWhenUnselected)
                                {
                                    DrawBoundsForLayer(PDI, LayerGridColor, LayerIndex);
                                }
                            }
                        }
                        else if (!bShowPerTileGridWhenUnselected)
                        {
                            DrawBoundsForLayer(PDI, LayerGridColor, OnlyLayerIndex);
                        }
                    }
                    if (bShowPerTileGridWhenUnselected)
                    {
                        const FLinearColor TileGridColor = TileMap->TileGridColor;
                        const FLinearColor MultiTileGridColor = TileMap->MultiTileGridColor;
                        int32 MultiTileGridWidth = TileMap->MultiTileGridWidth;
                        int32 MultiTileGridHeight = TileMap->MultiTileGridHeight;
                        int32 MultiTileGridOffsetX = TileMap->MultiTileGridOffsetX;
                        int32 MultiTileGridOffsetY = TileMap->MultiTileGridOffsetY;

                        switch (TileMap->ProjectionMode)
                        {
                        default:
                        case ETileMapProjectionMode::Orthogonal:
                        case ETileMapProjectionMode::IsometricDiamond:
                            DrawNormalGridLines(PDI, TileGridColor, MultiTileGridColor, MultiTileGridWidth, MultiTileGridHeight, MultiTileGridOffsetX, MultiTileGridOffsetY, SelectedLayerIndex);
                            break;
                        case ETileMapProjectionMode::IsometricStaggered:
                            DrawStaggeredGridLines(PDI, TileGridColor, MultiTileGridColor, MultiTileGridWidth, MultiTileGridHeight, MultiTileGridOffsetX, MultiTileGridOffsetY, SelectedLayerIndex);
                            break;
                        case ETileMapProjectionMode::HexagonalStaggered:
                            DrawHexagonalGridLines(PDI, TileGridColor, MultiTileGridColor, MultiTileGridWidth, MultiTileGridHeight, MultiTileGridOffsetX, MultiTileGridOffsetY, SelectedLayerIndex);
                            break;
                        }
                    }
                }
#endif
            }
        }
    }

    // Draw all of the queued up sprites
    FPaperRenderSceneProxy::GetDynamicMeshElements(Views, ViewFamily, VisibilityMap, Collector);
}