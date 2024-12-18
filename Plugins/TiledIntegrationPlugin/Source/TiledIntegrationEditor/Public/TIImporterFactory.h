// Copyright Flekz Games. All Rights Reserved.

#pragma once
#include "EditorReimportHandler.h"
#include "Factories/Factory.h"
#include "TIImporterFactory.generated.h"

namespace ETileMapProjectionMode { enum Type : int; }

class UPaperFlipbook;
class UPaperSprite;
class UTITileMap;
class UTIResourceManager;
class UTITileMapResource;
class UTITileSetResource;
class UTITextureResource;
class UTIFlipbookResource;
class UTISpriteResource;
class UTITileSet;
class UTexture2D;

struct FPaperTileInfo;
struct FSpriteGeometryCollection;
struct FTileSetFromTiled;
struct FTiledTileInfo;

enum class ETiledLayerType : uint8
{
    TileLayer,
    ObjectGroup,
    ImageLayer
};

enum class ETiledOrientation : uint8
{
    Unknown,
    Orthogonal,
    Isometric,
    Staggered,
    Hexagonal
};

enum class ETiledObjectLayerDrawOrder : uint8
{
    TopDown,
    Index
};

enum class ETiledStaggerAxis : uint8
{
    X,
    Y
};

enum class ETiledStaggerIndex : uint8
{
    Odd,
    Even
};

enum class ETiledRenderOrder : uint8
{
    RightDown,
    RightUp,
    LeftDown,
    LeftUp
};

enum class ETiledObjectType : uint8
{
    Box,
    Ellipse,
    Polygon,
    Polyline,
    PlacedTile
};

DECLARE_LOG_CATEGORY_EXTERN(LogTIImporter, Verbose, All);

// Imports a tile map (and associated textures & tile sets) exported from Tiled (http://www.mapeditor.org/)
UCLASS()
class TILEDINTEGRATIONEDITOR_API UTIImporterFactory : public UFactory
{
    GENERATED_UCLASS_BODY()

public:
    UTITileMap* ImportTileMap(UTITileMapResource* Resource, UTIResourceManager* ResourceManager, UObject* InParent);
    UTITileSet* ImportTileSet(UTITileSetResource* Resource, UTIResourceManager* ResourceManager, UObject* InParent);
    UTexture2D* ImportTexture(UTITextureResource* Resource, UTIResourceManager* ResourceManager, UObject* InParent);
    UPaperFlipbook* ImportFlipbook(UTIFlipbookResource* Resource, UTIResourceManager* ResourceManager, UObject* InParent);
    UPaperSprite* ImportSprite(UTISpriteResource* Resource, UTIResourceManager* ResourceManager, UObject* InParent);

protected:
    void ParseGlobalInfoFromJSON(TSharedPtr<FJsonObject> Tree, struct FTileMapFromTiled& OutParsedInfo, const FString& NameForErrors, bool bSilent = false);
    void FinalizeTileMap(FTileMapFromTiled& GlobalInfo, UTITileMap* TileMap);
};

//////////////////////////////////////////////////////////////////////////
// FTiledObject - A single object/shape placed inside a layer or tile

// See https://github.com/bjorn/tiled/wiki/TMX-Map-Format#object for more information

struct FTiledObject
{
    // The type of the object
    ETiledObjectType TiledObjectType;

    // ID of the shape (always 0 for per-tile collision shapes)
    int32 ID;

    // Arbitrary user-specified name
    FString Name;

    // Arbitrary user-specified type
    FString UserType;

    // Is the shape currently visible?
    bool bVisible;

    // Position of the shape:
    //   Ellipse or Box: The center
    //   PlacedTile: Bottom left
    //   Polygon or Polyline: Position of first vertex (not their center, the Tiled editor always bakes down rotations for them!)
    double X;
    double Y;

    // Local-space width/height of the shape (Ellipse, Box)
    // Note: Set to (0,0) for Polygon, Polyline, PlacedTile
    double Width;
    double Height;

    // Rotation (in degrees)
    // Note: Always 0 for Polyline and Polygon
    // Note: Includes winding (can be < 0 or > 360)
    double RotationDegrees;

    // Arbitrary user specified Key-Value pairs
    //TArray<FTiledStringPair> Properties;

    // Points for polygon/polyline shapes, relative to (X,Y), which seems to always be the first point in the polygon
    // Note: Only used when TiledObjectType is Polygon or Polyline
    TArray<FVector2D> Points;

    // The tile GID for placed tiles
    // Note: Only used when TiledObjectType is PlacedTile
    uint32 TileGID;

public:
    FTiledObject();

    bool ParseFromJSON(TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent = false);

    static bool ParsePointArray(TArray<FVector2D>& OutPoints, const TArray<TSharedPtr<FJsonValue>>& InArray, const FString& NameForErrors, bool bSilent);

    static void AddToSpriteGeometryCollection(const FVector2D& TileSize, const TArray<FTiledObject>& InObjects, FSpriteGeometryCollection& InOutShapes);
};

//////////////////////////////////////////////////////////////////////////
// FTileLayerFromTiled - A layer, containing either tiles or objects

// See https://github.com/bjorn/tiled/wiki/TMX-Map-Format#layer for more information

struct FTileLayerFromTiled
{
    // Name of the layer
    FString Name;

    // Array of tiles (only used when LayerType is TileLayer)
    TArray<uint32> TileIndices;

    // Width and height in tiles
    int32 Width;
    int32 Height;

    // Color of the layer (only set for object layers, to help distinguish them)
    FColor Color;

    // Object draw order (only used for object layers)
    ETiledObjectLayerDrawOrder ObjectDrawOrder;

    // Saved layer opacity (only RGB are used, A is ignored)
    float Opacity;

    // Is the layer currently visible?
    bool bVisible;

    // Type of the layer
    ETiledLayerType LayerType;

    // Offset
    int32 OffsetX;
    int32 OffsetY;

    // Placed objects (only used when LayerType is ObjectGroup)
    TArray<FTiledObject> Objects;

    // Overlay image (only used when LayerType is ImageLayer)
    FString OverlayImagePath;

    // Custom properties
    TArray<TSharedPtr<FJsonValue>> Properties;

public:
    FTileLayerFromTiled();

    bool ParseFromJSON(TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent = false);

    bool IsValid() const;
};

//////////////////////////////////////////////////////////////////////////
// FTiledTerrain

// See https://github.com/bjorn/tiled/wiki/TMX-Map-Format#terrain for more information

struct FTiledTerrain
{
    // The name of this terrain type
    FString TerrainName;

    // The index of the solid tile for this terrain (local  index to the tile set, not a GID!)
    uint32 SolidTileLocalIndex;

public:
    FTiledTerrain();

    bool ParseFromJSON(TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent = false);
};

//////////////////////////////////////////////////////////////////////////
// FTiledTileInfo

// See https://github.com/bjorn/tiled/wiki/TMX-Map-Format#tile for more information

struct FTiledTileInfo
{
    int32 TileIndex;

    // The terrain indices (into the TerrainTypes array of the containing tile set, INDEX_NONE if invalid)
    // Order is top-left, top-right, bottom-left, bottom-right
    int32 TerrainIndices[4];

    // The probability of placement of this tile (0..1)
    // Note: Tiled doesn't allow editing of this value right now (at least as of 0.11.0), so it's of limited value/trustworthiness
    float Probability;

    //@TODO: image?

    // Collision shapes
    TArray<FTiledObject> Objects;

    // Custom properties
    TArray<TSharedPtr<FJsonValue>> Properties;

public:
    FTiledTileInfo();

    bool ParseTileInfoFromJSON(int32 InTileIndex, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent = false);
};

//////////////////////////////////////////////////////////////////////////
// FTileSetFromTiled

// See https://github.com/bjorn/tiled/wiki/TMX-Map-Format#tileset for more information

struct FTileSetFromTiled
{
    int32 FirstGID;
    FString Name;

    // Source image path
    FString ImagePath;

    // Source image dimensions
    int32 ImageWidth;
    int32 ImageHeight;

    // Should we chroma-key remove certain pixels, making them transparent?
    bool bRemoveTransparentColor;

    // The color to remove if bRemoveTransparentColor is true
    FColor ImageTransparentColor;

    // Custom properties
    TArray<TSharedPtr<FJsonValue>> Properties;

    // Terrain types
    TArray<FTiledTerrain> TerrainTypes;

    // Per-tile info (terrain membership, collision objects, properties, etc...)
    TMap<int32, FTiledTileInfo> PerTileData;

    // Offset used when drawing tiles from this tile set
    int32 TileOffsetX;
    int32 TileOffsetY;

    // The spacing to ignore around the outer edge of the source image (in pixels)
    int32 Margin;

    // The spacing between each tile in the source image (in pixels)
    int32 Spacing;

    int32 TileWidth;
    int32 TileHeight;

public:
    FTileSetFromTiled();

    void ParseTileSetFromJSON(TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent = false);

    bool IsValid() const;

private:
    TSharedPtr<FJsonObject> ParseJSON(const FString& FileContents, const FString& NameForErrors, bool bSilent = false);
};

//////////////////////////////////////////////////////////////////////////
// FTileMapFromTiled

// See https://github.com/bjorn/tiled/wiki/TMX-Map-Format#map for more information

struct FTileMapFromTiled
{
    // JSON export file version as defined by Tiled (1 is the only known version)
    int32 FileVersion;

    // Dimensions of the tile map (in tiles)
    int32 Width;
    int32 Height;

    // Dimensions of a tile (in pixels)
    int32 TileWidth;
    int32 TileHeight;

    // Projection mode of the tile map
    ETiledOrientation Orientation;

    // Side length (only used in Hexagonal projection mode)
    int32 HexSideLength;

    // Stagger axis (only used in Staggered and Hexagonal modes)
    ETiledStaggerAxis StaggerAxis;

    // Stagger index (only used in Staggered and Hexagonal modes)
    ETiledStaggerIndex StaggerIndex;

    // Render order
    ETiledRenderOrder RenderOrder;

    // Background color
    FColor BackgroundColor;

    // Set of source tile sets imported from Tiled
    TArray<FTileSetFromTiled> TileSets;

    // Set of destination tile set assets created by this import
    TArray<UTITileSet*> CreatedTileSetAssets;

    // Layers
    TArray<FTileLayerFromTiled> Layers;

    // Custom properties
    TArray<TSharedPtr<FJsonValue>> Properties;

public:
    FPaperTileInfo ConvertTileGIDToPaper2D(uint32 GID) const;
    ETileMapProjectionMode::Type GetOrientationType() const;

    FTileMapFromTiled();
    bool IsValid() const;
};

#if UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_2
#include "CoreMinimal.h"
#include "PaperTileLayer.h"
#include "PaperTileMap.h"
#include "UObject/ObjectMacros.h"
#endif
