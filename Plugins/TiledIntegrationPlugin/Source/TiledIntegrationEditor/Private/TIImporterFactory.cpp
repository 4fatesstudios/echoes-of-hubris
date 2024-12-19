// Copyright Flekz Games. All Rights Reserved.

#include "TIImporterFactory.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Editor.h"
#include "Engine/Texture2D.h"
#include "EditorFramework/AssetImportData.h"
#include "Factories/TextureFactory.h"
#include "IAssetTools.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "PackageTools.h"
#include "PaperImporterSettings.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookFactory.h"
#include "PaperJSONHelpers.h"
#include "PaperSprite.h"
#include "Serialization/JsonSerializer.h"
#include "Subsystems/ImportSubsystem.h"
#include "TICustomProperties.h"
#include "TISettings.h"
#include "TIResource.h"
#include "TITile.h"
#include "TITileInstance.h"
#include "TITileMap.h"
#include "TITileSet.h"
#include "TITileLayer.h"
#include "TileMapAssetImportData.h"

DEFINE_LOG_CATEGORY(LogTIImporter);

#include UE_INLINE_GENERATED_CPP_BY_NAME(TIImporterFactory)

#define LOCTEXT_NAMESPACE "Paper2D"
#define TILED_IMPORT_ERROR(FormatString, ...) \
	if (!bSilent) { UE_LOG(LogTIImporter, Warning, FormatString, __VA_ARGS__); }
#define TILED_IMPORT_WARNING(FormatString, ...) \
	if (!bSilent) { UE_LOG(LogTIImporter, Warning, FormatString, __VA_ARGS__); }

//////////////////////////////////////////////////////////////////////////
// FRequiredScalarField

template<typename ScalarType>
struct FRequiredScalarField
{
    ScalarType& Value;
    const FString Key;
    ScalarType MinValue;

    FRequiredScalarField(ScalarType& InValue, const FString& InKey)
    : Value(InValue)
    , Key(InKey)
    , MinValue(1.0f)
    {
    }

    FRequiredScalarField(ScalarType& InValue, const FString& InKey, ScalarType InMinValue)
    : Value(InValue)
    , Key(InKey)
    , MinValue(InMinValue)
    {
    }
};

typedef FRequiredScalarField<int32> FRequiredIntField;
typedef FRequiredScalarField<double> FRequiredDoubleField;

//////////////////////////////////////////////////////////////////////////

template <typename ScalarType>
bool ParseScalarFields(FRequiredScalarField<ScalarType>* FieldArray, int32 ArrayCount, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent)
{
    bool bSuccessfullyParsed = true;

    for (int32 ArrayIndex = 0; ArrayIndex < ArrayCount; ++ArrayIndex)
    {
        const FRequiredScalarField<ScalarType>& Field = FieldArray[ArrayIndex];

        if (!Tree->TryGetNumberField(Field.Key, Field.Value))
        {
            TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  Missing '%s' property"), *NameForErrors, *Field.Key);
            bSuccessfullyParsed = false;
            Field.Value = 0;
        }
        else
        {
            if (Field.Value < Field.MinValue)
            {
                TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  Invalid value for '%s' (%d but must be at least %d)"), *NameForErrors, *Field.Key, int(Field.Value), int(Field.MinValue));
                bSuccessfullyParsed = false;
                Field.Value = Field.MinValue;
            }
        }
    }

    return bSuccessfullyParsed;
}

bool ParseIntegerFields(FRequiredIntField* IntFieldArray, int32 ArrayCount, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent)
{
    return ParseScalarFields(IntFieldArray, ArrayCount, Tree, NameForErrors, bSilent);
}

//////////////////////////////////////////////////////////////////////////
// UTIImporterFactory

UTIImporterFactory::UTIImporterFactory(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
    bCreateNew = false;
    SupportedClass = UTITileMap::StaticClass();

    bEditorImport = true;
    bText = true;

    Formats.Add(TEXT("json;Tiled JSON file"));
}

UTITileMap* UTIImporterFactory::ImportTileMap(UTITileMapResource* Resource, UTIResourceManager* ResourceManager, UObject* InParent)
{
    UTITileMap* Result = nullptr;
    if (Resource && ResourceManager && InParent)
    {
        FString FileExtension = FPaths::GetExtension(Resource->SourcePath);
        FString AssetPath = FPaths::GetPath(InParent->GetName());
        FString AssetName = FPaths::GetBaseFilename(InParent->GetName());
        GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(this, UTITileMap::StaticClass(), InParent, *AssetName, *FileExtension);
        FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");

        bool bLoadedSuccessfully = true;
        const FString NameForErrors = AssetName;
        TSharedPtr<FJsonObject> DescriptorObject = Resource->GetJsonData();

        FTileMapFromTiled GlobalInfo;
        if (DescriptorObject.IsValid())
        {
            ParseGlobalInfoFromJSON(DescriptorObject, GlobalInfo, NameForErrors);
        }

        if (GlobalInfo.IsValid())
        {
            if (GlobalInfo.FileVersion != 1)
            {
                UE_LOG(LogTIImporter, Warning, TEXT("JSON exported from Tiled in file '%s' has an unknown version %d (expected version 1).  Parsing will continue but some things may not import correctly"), *NameForErrors, GlobalInfo.FileVersion);
            }

            // Parse the custom properties (if present)
            const TArray<TSharedPtr<FJsonValue>>* PropertiesPtr;
            if (DescriptorObject->TryGetArrayField(TEXT("properties"), PropertiesPtr))
            {
                GlobalInfo.Properties = *PropertiesPtr;
            }

            // Load the layers
            const TArray<TSharedPtr<FJsonValue>>* LayerDescriptors;
            if (DescriptorObject->TryGetArrayField(TEXT("layers"), LayerDescriptors))
            {
                for (TSharedPtr<FJsonValue> LayerDescriptor : *LayerDescriptors)
                {
                    FTileLayerFromTiled& TileLayer = *new (GlobalInfo.Layers) FTileLayerFromTiled();
                    TileLayer.ParseFromJSON(LayerDescriptor->AsObject(), NameForErrors);
                    bLoadedSuccessfully = bLoadedSuccessfully && TileLayer.IsValid();
                }
            }
            else
            {
                UE_LOG(LogTIImporter, Warning, TEXT("JSON exported from Tiled in file '%s' has no layers."), *NameForErrors);
                bLoadedSuccessfully = false;
            }

            if (bLoadedSuccessfully)
            {
                // Create the tile sets
                const auto& TileSets = Resource->GetTileSets();
                if (!TileSets.IsEmpty())
                {
                    for (auto& TileSetPair : TileSets)
                    {
                        if (TileSetPair.Value.IsValid())
                        {
                            const int32 FirstGID = TileSetPair.Key;
                            UTITileSetResource* TileSetResource = TileSetPair.Value.Get();
                            UTITileSet* TileSet = Cast<UTITileSet>(TileSetResource->GetAsset());
                            if (TileSet == nullptr)
                            {
                                FString TileSetNamingTemplate = "BP_{0}";
                                TileSetNamingTemplate.ReplaceInline(TEXT("{0}"), *TileSetResource->Name);
                                const FString TileSetAssetPath = AssetPath / TEXT("TileSets") / TileSetNamingTemplate;
                                TileSet = Cast<UTITileSet>(TileSetResource->ImportAsset(ResourceManager, TileSetAssetPath));
                            }
                            else
                            {
                                TileSetResource->ReimportAsset(ResourceManager);
                            }

                            FTileSetFromTiled& TileSetData = *new (GlobalInfo.TileSets) FTileSetFromTiled();
                            TileSetData.ParseTileSetFromJSON(TileSetResource->GetJsonData(), NameForErrors);
                            TileSetData.FirstGID = TileSetPair.Key;

                            GlobalInfo.CreatedTileSetAssets.Add(TileSet ? TileSet : nullptr);
                        }
                        else
                        {
                            UE_LOG(LogTIImporter, Warning, TEXT("JSON exported from Tiled in file '%s' has invalid tile sets."), *NameForErrors);
                            bLoadedSuccessfully = false;
                            break;
                        }
                    }
                }
                else
                {
                    UE_LOG(LogTIImporter, Warning, TEXT("JSON exported from Tiled in file '%s' has no tile sets."), *NameForErrors);
                    bLoadedSuccessfully = false;
                }

                // Create the layers
                if (bLoadedSuccessfully)
                {
                    // Create the new tile map asset and import basic/global data
                    Result = Cast<UTITileMap>(Resource->GetAsset());
                    if (Result == nullptr)
                    {
                        Result = NewObject<UTITileMap>(InParent, GetDefault<UTISettings>()->TileMapClass, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
                    }

                    Result->Modify();
                    Result->MapWidth = GlobalInfo.Width;
                    Result->MapHeight = GlobalInfo.Height;
                    Result->TileWidth = GlobalInfo.TileWidth;
                    Result->TileHeight = GlobalInfo.TileHeight;
                    Result->SeparationPerTileX = 0.0f;
                    Result->SeparationPerTileY = 0.0f;
                    Result->SeparationPerLayer = 1.0f;
                    Result->ProjectionMode = GlobalInfo.GetOrientationType();
                    Result->BackgroundColor = GlobalInfo.BackgroundColor;
                    Result->HexSideLength = GlobalInfo.HexSideLength;

                    if (GlobalInfo.Orientation == ETiledOrientation::Hexagonal)
                    {
                        Result->TileHeight += GlobalInfo.HexSideLength;
                    }

                    Result->LoadCustomProperties(GlobalInfo.Properties);

                    TArray<TObjectPtr<UPaperTileLayer>> OldTileLayers = Result->TileLayers;
                    Result->TileLayers.Empty(GlobalInfo.Layers.Num());
                    Result->Layers.Empty(GlobalInfo.Layers.Num());

                    for (int32 LayerIndex = GlobalInfo.Layers.Num() - 1; LayerIndex >= 0; --LayerIndex)
                    {
                        const FTileLayerFromTiled& LayerData = GlobalInfo.Layers[LayerIndex];
                        if (LayerData.IsValid())
                        {
                            // Try to reuse the layer if already exists (when reimporting)
                            UTITileLayer* Layer = nullptr;
                            const int32 ReversedLayerIndex = (GlobalInfo.Layers.Num() - 1) - LayerIndex;
                            if (OldTileLayers.IsValidIndex(ReversedLayerIndex))
                            {
                                Layer = Cast<UTITileLayer>(OldTileLayers[ReversedLayerIndex]);
                            }

                            if (Layer == nullptr)
                            {
                                Layer = NewObject<UTITileLayer>(Result, GetDefault<UTISettings>()->TileLayerClass);
                            }

                            Layer->SetFlags(RF_Transactional);
                            Layer->LayerName = FText::FromString(LayerData.Name);
                            Layer->SetShouldRenderInEditor(LayerData.bVisible);

                            FLinearColor LayerColor = FLinearColor::White;
                            LayerColor.A = FMath::Clamp<float>(LayerData.Opacity, 0.0f, 1.0f);
                            Layer->SetLayerColor(LayerColor);

                            Layer->LoadCustomProperties(LayerData.Properties);

                            //@TODO: No support for Objects (and thus Color, ObjectDrawOrder), OffsetX, or OffsetY

                            Layer->DestructiveAllocateMap(LayerData.Width, LayerData.Height);

                            Layer->LayerIndex = ReversedLayerIndex;
                            Layer->TileMap = Result;

                            int32 SourceIndex = 0;
                            for (int32 Y = 0; Y < LayerData.Height; ++Y)
                            {
                                for (int32 X = 0; X < LayerData.Width; ++X)
                                {
                                    const uint32 SourceTileGID = LayerData.TileIndices[SourceIndex++];
                                    const FPaperTileInfo CellContents = GlobalInfo.ConvertTileGIDToPaper2D(SourceTileGID);
                                    Layer->SetCell(X, Y, CellContents);
                                }
                            }

                            Layer->InitializeTiles(GetDefault<UTISettings>()->TileInstanceClass);

                            // Load tile instance custom properties from the map custom properties
                            const auto& TileMapProperties = GlobalInfo.Properties;
                            if (!TileMapProperties.IsEmpty())
                            {
                                Layer->ForEachOccupiedTile([&TileMapProperties](UTITileInstance* TileInstance, uint32) -> bool
                                {
                                    FString TilePropertiesName = "TileProperties[{X},{Y},{Z}]";
                                    const FIntVector& Coordinates = TileInstance->GetTileCoordinates();
                                    TilePropertiesName.ReplaceInline(TEXT("{X}"), *FString::FromInt(Coordinates.X));
                                    TilePropertiesName.ReplaceInline(TEXT("{Y}"), *FString::FromInt(Coordinates.Y));
                                    TilePropertiesName.ReplaceInline(TEXT("{Z}"), *FString::FromInt(Coordinates.Z));

                                    for (TSharedPtr<FJsonValue> PropertyPtr : TileMapProperties)
                                    {
                                        if (TSharedPtr<FJsonObject> Property = PropertyPtr->AsObject())
                                        {
                                            FString PropertyName;
                                            if (Property->TryGetStringField(TEXT("name"), PropertyName) && !PropertyName.IsEmpty())
                                            {
                                                if (PropertyName == TilePropertiesName)
                                                {
                                                    const TSharedPtr<FJsonObject>* TilePropertiesData;
                                                    if (Property->TryGetObjectField(TEXT("value"), TilePropertiesData) && TilePropertiesData && TilePropertiesData->IsValid())
                                                    {
                                                        TArray<TSharedPtr<FJsonValue>> TileProperties;
                                                        for (auto PropertyPair : TilePropertiesData->Get()->Values)
                                                        {
                                                            TSharedPtr<FJsonObject> NewProperty = MakeShared<FJsonObject>();
                                                            NewProperty->SetStringField(TEXT("name"), PropertyPair.Key);
                                                            NewProperty->SetField(TEXT("value"), PropertyPair.Value);
                                                            TileProperties.Add(MakeShared<FJsonValueObject>(NewProperty.ToSharedRef()));
                                                        }

                                                        TileInstance->LoadCustomProperties(TileProperties);
                                                    }

                                                    return false;
                                                }
                                            }
                                        }
                                    }

                                    return true;
                                });
                            }

                            Result->TileLayers.Add(Layer);
                            Result->Layers.Add(Layer);
                        }
                    }

                    // Finalize the tile map, including analyzing the tile set textures to determine a good material
                    FinalizeTileMap(GlobalInfo, Result);

                    Result->PostEditChange();
                    Result->OnImportedBroadcast();
                }
            }
        }
        else
        {
            // Failed to parse the JSON
            bLoadedSuccessfully = false;
        }

        if (Result != nullptr)
        {
            // Store the current file path and timestamp for re-import purposes
            UTileMapAssetImportData* ImportData = UTileMapAssetImportData::GetImportDataForTileMap(Result);
            ImportData->Update(CurrentFilename);
        }

        GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, Result);
    }

    return Result;
}

UTITileSet* UTIImporterFactory::ImportTileSet(UTITileSetResource* Resource, UTIResourceManager* ResourceManager, UObject* InParent)
{
    UTITileSet* Result = nullptr;
    if (Resource && ResourceManager && InParent)
    {
        FString AssetPath = FPaths::GetPath(InParent->GetName());
        FString AssetName = FPaths::GetBaseFilename(InParent->GetName());

        FString NameForErrors;
        FTileSetFromTiled TileSetData;
        TileSetData.ParseTileSetFromJSON(Resource->GetJsonData(), NameForErrors);

        if (TileSetData.IsValid())
        {
            // Import/Reimport the texture
            if (UTITextureResource* TextureResource = Resource->GetTexture())
            {
                UTexture2D* Texture = Cast<UTexture2D>(TextureResource->GetAsset());
                if (Texture == nullptr)
                {
                    FString TextureNamingTemplate = "T_{0}";
                    TextureNamingTemplate.ReplaceInline(TEXT("{0}"), *TextureResource->Name);
                    const FString TextureAssetPath = AssetPath / TEXT("Textures") / TextureNamingTemplate;
                    Texture = Cast<UTexture2D>(TextureResource->ImportAsset(ResourceManager, TextureAssetPath));
                }
                else
                {
                    TextureResource->ReimportAsset(ResourceManager);
                }

                if (Texture)
                {
                    Result = Cast<UTITileSet>(Resource->GetAsset());
                    if (Result == nullptr)
                    {
                        Result = NewObject<UTITileSet>(InParent, GetDefault<UTISettings>()->TileSetClass, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
                    }

                    Result->Modify();
                    Result->SetTileSize(FIntPoint(TileSetData.TileWidth, TileSetData.TileHeight));
                    Result->SetMargin(FIntMargin(TileSetData.Margin));
                    Result->SetPerTileSpacing(FIntPoint(TileSetData.Spacing, TileSetData.Spacing));
                    Result->SetDrawingOffset(FIntPoint(TileSetData.TileOffsetX, TileSetData.TileOffsetY));

                    Result->SetTileSheetTexture(Texture);
                    Result->LoadCustomProperties(TileSetData.Properties);

                    // Make the tile set allocate space for the individual tiles
                    FPropertyChangedEvent InteractiveRebuildTileSet(nullptr, EPropertyChangeType::Interactive);
                    Result->PostEditChangeProperty(InteractiveRebuildTileSet);
                    Result->InitializeTiles(GetDefault<UTISettings>()->TileClass);

                    // Copy across terrain information
                    const int32 MaxTerrainTypes = 0xFE;
                    const uint8 NoTerrainMembershipIndex = 0xFF;
                    if (TileSetData.TerrainTypes.Num() > MaxTerrainTypes)
                    {
                        UE_LOG(LogTIImporter, Warning, TEXT("Tile set '%s' contains more than %d terrain types, ones above this will be ignored."), *TileSetData.Name, MaxTerrainTypes);
                    }

                    const int32 NumTerrainsToCopy = FMath::Min<int32>(TileSetData.TerrainTypes.Num(), MaxTerrainTypes);
                    for (int32 TerrainIndex = 0; TerrainIndex < NumTerrainsToCopy; ++TerrainIndex)
                    {
                        const FTiledTerrain& SourceTerrain = TileSetData.TerrainTypes[TerrainIndex];

                        FPaperTileSetTerrain DestTerrain;
                        DestTerrain.TerrainName = SourceTerrain.TerrainName;
                        DestTerrain.CenterTileIndex = SourceTerrain.SolidTileLocalIndex;

                        Result->AddTerrainDescription(DestTerrain);
                    }

                    // Copy across per-tile metadata and custom properties
                    Result->ForEachTile([&TileSetData, &NumTerrainsToCopy](UTITile* Tile, uint32 TileIndex)
                    {
                        if (FPaperTileMetadata* TargetTileData = Tile->GetMutableMetadata())
                        {
                            if (const FTiledTileInfo* SourceTileData = TileSetData.PerTileData.Find(TileIndex))
                            {
                                // Convert collision geometry
                                FTiledObject::AddToSpriteGeometryCollection(FVector2D(TileSetData.TileWidth, TileSetData.TileHeight), SourceTileData->Objects, TargetTileData->CollisionData);

                                // Convert terrain memberhsip
                                for (int32 Index = 0; Index < 4; ++Index)
                                {
                                    const int32 SourceTerrainIndex = SourceTileData->TerrainIndices[Index];

                                    const uint8 DestTerrainIndex = ((SourceTerrainIndex >= 0) && (SourceTerrainIndex < NumTerrainsToCopy)) ? (uint8)SourceTerrainIndex : NoTerrainMembershipIndex;
                                    TargetTileData->TerrainMembership[Index] = DestTerrainIndex;
                                }

                                // Load custom properties
                                Tile->LoadCustomProperties(SourceTileData->Properties);

                                // Load user data from custom properties
                                FString TileUserData;
                                if (Tile->GetCustomProperties()->GetStringProperty("UserDataName", TileUserData))
                                {
                                    TargetTileData->UserDataName = *TileUserData;
                                }
                            }
                            else
                            {
                                // Reset the custom properties and the metadata
                                Tile->GetCustomProperties()->Empty();
                                *TargetTileData = FPaperTileMetadata();
                            }
                        }

                        return true;
                    });

                    // Import flipbooks
                    TArray<UTIFlipbookResource*> FlipbookResources = Resource->GetFlipbooks();
                    for (UTIFlipbookResource* FlipbookResource : FlipbookResources)
                    {
                        UPaperFlipbook* Flipbook = Cast<UPaperFlipbook>(FlipbookResource->GetAsset());
                        if (Flipbook == nullptr)
                        {
                            FString FlipbookNamingTemplate = "FB_{0}_{1}";
                            FlipbookNamingTemplate.ReplaceInline(TEXT("{0}"), *Resource->Name);
                            FlipbookNamingTemplate.ReplaceInline(TEXT("{1}"), *FlipbookResource->Name);
                            const FString FlipbookAssetPath = AssetPath / TEXT("Flipbooks") / FlipbookNamingTemplate;
                            Flipbook = Cast<UPaperFlipbook>(FlipbookResource->ImportAsset(ResourceManager, FlipbookAssetPath));
                        }
                        else
                        {
                            FlipbookResource->ReimportAsset(ResourceManager);
                        }

                        if (Flipbook)
                        {
                            const int32 TileIndex = FlipbookResource->GetTileID();
                            if (UTITile* Tile = Result->GetTile(TileIndex))
                            {
                                Tile->Flipbook = Flipbook;
                            }
                        }
                    }

                    // If the tile set was used by a tile map update the tile references
                    for (UTITileMapResource* TileMapResource : Resource->GetTileMaps())
                    {
                        if (UTITileMap* TileMap = Cast<UTITileMap>(TileMapResource->GetAsset()))
                        {
                            TileMap->ForEachLayer([this](UTITileLayer* Layer, uint32) -> bool
                            {
                                Layer->InitializeTiles(GetDefault<UTISettings>()->TileInstanceClass);
                                return true;
                            });

                            TileMap->MarkPackageDirty();
                            ULevel::LevelDirtiedEvent.Broadcast();
                            TileMap->PostEditChange();
                            TileMapResource->SaveAsset();
                        }
                    }

                    // Update anyone who might be using the tile set (in case we're reimporting)
                    FPropertyChangedEvent FinalRebuildTileSet(nullptr, EPropertyChangeType::ValueSet);
                    Result->PostEditChangeProperty(FinalRebuildTileSet);
                    Result->OnImportedBroadcast();
                }
                else
                {
                    UE_LOG(LogTIImporter, Warning, TEXT("Failed to import tile set image '%s' referenced from tile set '%s'."), *TileSetData.ImagePath, *TileSetData.Name);
                }
            }
        }
    }

    return Result;
}

UTexture2D* UTIImporterFactory::ImportTexture(UTITextureResource* Resource, UTIResourceManager* ResourceManager, UObject* InParent)
{
    UTexture2D* Result = nullptr;
    if (Resource)
    {
        FString AssetPath = FPaths::GetPath(InParent->GetName());
        FString AssetName = FPaths::GetBaseFilename(InParent->GetName());

        UTextureFactory* TextureFactory = NewObject<UTextureFactory>(UTextureFactory::StaticClass());
        TextureFactory->AddToRoot();

        if (TextureFactory->FactoryCanImport(Resource->GetAbsoluteSourcePath()))
        {
            bool bOutCancelled = false;
            TextureFactory->SuppressImportOverwriteDialog();
            Result = Cast<UTexture2D>(TextureFactory->ImportObject(UTexture2D::StaticClass(), InParent, *AssetName, RF_Public | RF_Standalone | RF_Transactional, Resource->GetAbsoluteSourcePath(), nullptr, bOutCancelled));
            if (Result)
            {
                // Change the compression settings
                GetDefault<UPaperImporterSettings>()->ApplyTextureSettings(Result);
            }
        }

        TextureFactory->RemoveFromRoot();
    }

    return Result;
}

UPaperFlipbook* UTIImporterFactory::ImportFlipbook(UTIFlipbookResource* Resource, UTIResourceManager* ResourceManager, UObject* InParent)
{
    UPaperFlipbook* Result = nullptr;
    if (Resource && ResourceManager && InParent)
    {
        FString AssetPath = FPaths::GetPath(InParent->GetName());
        FString AssetName = FPaths::GetBaseFilename(InParent->GetName());

        // Import/Reimport the sprites
        TMap<int32, UPaperSprite*> Sprites;
        UTITileSetResource* TileSetResource = Resource->GetTileSet();
        TArray<UTISpriteResource*> SpriteResources = Resource->GetSprites();
        for (UTISpriteResource* SpriteResource : SpriteResources)
        {
            UPaperSprite* Sprite = Cast<UPaperSprite>(SpriteResource->GetAsset());
            if (Sprite == nullptr)
            {
                FString SpriteNamingTemplate = "S_{0}_{1}";
                SpriteNamingTemplate.ReplaceInline(TEXT("{0}"), *TileSetResource->Name);
                SpriteNamingTemplate.ReplaceInline(TEXT("{1}"), *SpriteResource->Name);
                const FString SpriteAssetPath = AssetPath / TEXT("Sprites") / SpriteNamingTemplate;
                Sprite = Cast<UPaperSprite>(SpriteResource->ImportAsset(ResourceManager, SpriteAssetPath));
            }
            else
            {
                SpriteResource->ReimportAsset(ResourceManager);
            }

            if (Sprite)
            {
                Sprites.Add(SpriteResource->GetTileID(), Sprite);
            }
        }

        const TArray<TSharedPtr<FJsonValue>>* AnimationDatasPtr;
        if (Resource->GetJsonData()->TryGetArrayField(TEXT("animation"), AnimationDatasPtr) && AnimationDatasPtr && !AnimationDatasPtr->IsEmpty())
        {
            // Default frames per second in a flipbook
            constexpr float DefaultFramesPerSecond = 15.0f;
            constexpr float SecondsPerFrame = DefaultFramesPerSecond / 1000.0f;

            UPaperFlipbookFactory* FlipbookFactory = NewObject<UPaperFlipbookFactory>();
            FlipbookFactory->KeyFrames.Empty();

            for (TSharedPtr<FJsonValue> AnimationDataPtr : *AnimationDatasPtr)
            {
                TSharedPtr<FJsonObject> AnimationData = AnimationDataPtr->AsObject();

                int32 TileIndex = INDEX_NONE;
                AnimationData->TryGetNumberField(TEXT("tileid"), TileIndex);

                int32 FrameDuration = INDEX_NONE;
                AnimationData->TryGetNumberField(TEXT("duration"), FrameDuration);

                if (TileIndex != INDEX_NONE && FrameDuration != INDEX_NONE)
                {
                    if (auto* SpritePtr = Sprites.Find(TileIndex))
                    {
                        FPaperFlipbookKeyFrame KeyFrame;
                        KeyFrame.Sprite = *SpritePtr;
                        KeyFrame.FrameRun = (FrameDuration * SecondsPerFrame);
                        FlipbookFactory->KeyFrames.Add(KeyFrame);
                    }
                    else
                    {
                        // Something went very wrong...
                        break;
                    }
                }
            }

            if (!FlipbookFactory->KeyFrames.IsEmpty())
            {
                Result = Cast<UPaperFlipbook>(FlipbookFactory->FactoryCreateNew(UPaperFlipbook::StaticClass(), InParent, *AssetName, RF_Public | RF_Standalone | RF_Transactional, nullptr, nullptr));
            }
        }
    }

    return Result;
}

UPaperSprite* UTIImporterFactory::ImportSprite(UTISpriteResource* Resource, UTIResourceManager* ResourceManager, UObject* InParent)
{
    UPaperSprite* Result = nullptr;
    if (Resource && ResourceManager && InParent)
    {
        FString AssetPath = FPaths::GetPath(InParent->GetName());
        FString AssetName = FPaths::GetBaseFilename(InParent->GetName());

        if (UTITileSetResource* TileSetResource = Resource->GetTileSet())
        {
            if (UTITileSet* TileSet = Cast<UTITileSet>(TileSetResource->GetAsset()))
            {
                if (UTexture2D* Texture = TileSet->GetTileSheetTexture())
                {
                    const int32 TileIndex = Resource->GetTileID();
                    const FIntPoint TileSetTextureSize = Texture->GetImportedSize();
                    const FIntPoint TileSize = TileSet->GetTileSize();
                    const FIntPoint TileMargin = FIntPoint(TileSet->GetMargin().Left, TileSet->GetMargin().Top);
                    const FIntPoint TileSpacing = TileSet->GetPerTileSpacing();
                    const int32 TilesPerRow = (TileSetTextureSize.X - TileMargin.X) / (TileSize.X + TileSpacing.X);
                    const int32 TilesPerColumn = (TileSetTextureSize.Y - TileMargin.Y) / (TileSize.Y + TileSpacing.Y);

                    const int32 X = TileIndex % TilesPerRow;
                    const int32 Y = TileIndex / TilesPerRow;

                    FIntRect SpriteRect;
                    SpriteRect.Min.X = (X * (TileSize.X + TileSpacing.X)) + TileMargin.X;
                    SpriteRect.Min.Y = (Y * (TileSize.Y + TileSpacing.Y)) + TileMargin.Y;
                    SpriteRect.Max.X = (X * (TileSize.X + TileSpacing.X)) + TileMargin.X + TileSize.X;
                    SpriteRect.Max.Y = (Y * (TileSize.Y + TileSpacing.Y)) + TileMargin.Y + TileSize.Y;

                    Result = Cast<UPaperSprite>(Resource->GetAsset());
                    if (Result == nullptr)
                    {
                        Result = NewObject<UPaperSprite>(InParent, UPaperSprite::StaticClass(), *AssetName, RF_Public | RF_Standalone | RF_Transactional);
                    }

                    FSpriteAssetInitParameters SpriteInitParams;
                    SpriteInitParams.Texture = Texture;
                    SpriteInitParams.Offset = SpriteRect.Min;
                    SpriteInitParams.Dimension = FIntPoint(SpriteRect.Width(), SpriteRect.Height());

                    const UPaperImporterSettings* ImporterSettings = GetDefault<UPaperImporterSettings>();
                    bool bFoundNormalMap = false;
                    if (Texture != nullptr)
                    {
                        // Look for an associated normal map to go along with the base map
                        const FString SanitizedBasePackageName = UPackageTools::SanitizePackageName(Texture->GetOutermost()->GetName());
                        const FString PackagePath = FPackageName::GetLongPackagePath(SanitizedBasePackageName);
                        FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

                        const FString NormalMapNameNoSuffix = ImporterSettings->RemoveSuffixFromBaseMapName(Texture->GetName());

                        TArray<FString> NamesToTest;
                        ImporterSettings->GenerateNormalMapNamesToTest(NormalMapNameNoSuffix, NamesToTest);
                        ImporterSettings->GenerateNormalMapNamesToTest(Texture->GetName(), NamesToTest);

                        // Test each name for an existing asset
                        for (const FString& NameToTest : NamesToTest)
                        {
                            const FString ObjectPathToTest = PackagePath / (NameToTest + FString(TEXT(".")) + NameToTest);
                            FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(ObjectPathToTest));

                            if (AssetData.IsValid())
                            {
                                if (UTexture2D* NormalMapTexture = Cast<UTexture2D>(AssetData.GetAsset()))
                                {
                                    bFoundNormalMap = true;
                                    SpriteInitParams.AdditionalTextures.Add(NormalMapTexture);
                                    break;
                                }
                            }
                        }
                    }

                    ImporterSettings->ApplySettingsForSpriteInit(SpriteInitParams, bFoundNormalMap ? ESpriteInitMaterialLightingMode::ForceLit : ESpriteInitMaterialLightingMode::Automatic);
                    Result->InitializeSprite(SpriteInitParams);
                }
            }
        }
    }

    return Result;
}

void UTIImporterFactory::ParseGlobalInfoFromJSON(TSharedPtr<FJsonObject> Tree, FTileMapFromTiled& OutParsedInfo, const FString& NameForErrors, bool bSilent)
{
    bool bSuccessfullyParsed = true;

    // Parse all of the required integer fields
    FRequiredIntField IntFields[] = {
        FRequiredIntField(OutParsedInfo.FileVersion, TEXT("version")),
        FRequiredIntField(OutParsedInfo.Width, TEXT("width")),
        FRequiredIntField(OutParsedInfo.Height, TEXT("height")),
        FRequiredIntField(OutParsedInfo.TileWidth, TEXT("tilewidth")),
        FRequiredIntField(OutParsedInfo.TileHeight, TEXT("tileheight"))
    };
    bSuccessfullyParsed = bSuccessfullyParsed && ParseIntegerFields(IntFields, UE_ARRAY_COUNT(IntFields), Tree, NameForErrors, bSilent);

    // Parse hexsidelength if present
    FRequiredIntField OptionalIntFields[] = {
        FRequiredIntField(OutParsedInfo.HexSideLength, TEXT("hexsidelength"), 0)
    };
    ParseIntegerFields(OptionalIntFields, UE_ARRAY_COUNT(OptionalIntFields), Tree, NameForErrors, true);

    // Parse StaggerAxis if present
    const FString StaggerAxisStr = FPaperJSONHelpers::ReadString(Tree, TEXT("staggeraxis"), FString());
    if (StaggerAxisStr == TEXT("x"))
    {
        OutParsedInfo.StaggerAxis = ETiledStaggerAxis::X;
    }
    else if (StaggerAxisStr == TEXT("y"))
    {
        OutParsedInfo.StaggerAxis = ETiledStaggerAxis::Y;
    }
    else if (!StaggerAxisStr.IsEmpty())
    {
        TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  Invalid value for '%s' ('%s' but expected 'x' or 'y')"), *NameForErrors, TEXT("staggeraxis"), *StaggerAxisStr);
        bSuccessfullyParsed = false;
    }

    // Parse StaggerIndex if present
    const FString StaggerIndexStr = FPaperJSONHelpers::ReadString(Tree, TEXT("staggerindex"), FString());
    if (StaggerIndexStr == TEXT("even"))
    {
        OutParsedInfo.StaggerIndex = ETiledStaggerIndex::Even;
    }
    else if (StaggerIndexStr == TEXT("odd"))
    {
        OutParsedInfo.StaggerIndex = ETiledStaggerIndex::Odd;
    }
    else if (!StaggerIndexStr.IsEmpty())
    {
        TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  Invalid value for '%s' ('%s' but expected 'x' or 'y')"), *NameForErrors, TEXT("staggerindex"), *StaggerIndexStr);
        bSuccessfullyParsed = false;
    }

    // Parse RenderOrder if present
    const FString RenderOrderStr = FPaperJSONHelpers::ReadString(Tree, TEXT("staggerindex"), FString());
    if (RenderOrderStr == TEXT("right-down"))
    {
        OutParsedInfo.RenderOrder = ETiledRenderOrder::RightDown;
    }
    else if (RenderOrderStr == TEXT("right-up"))
    {
        OutParsedInfo.RenderOrder = ETiledRenderOrder::RightUp;
    }
    else if (RenderOrderStr == TEXT("left-down"))
    {
        OutParsedInfo.RenderOrder = ETiledRenderOrder::LeftDown;
    }
    else if (RenderOrderStr == TEXT("left-up"))
    {
        OutParsedInfo.RenderOrder = ETiledRenderOrder::LeftUp;
    }
    else if (!RenderOrderStr.IsEmpty())
    {
        TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  Invalid value for '%s' ('%s' but expected 'right-down', 'right-up', 'left-down', or 'left-up')"), *NameForErrors, TEXT("renderorder"), *RenderOrderStr);
        bSuccessfullyParsed = false;
    }

    // Parse BackgroundColor if present
    const FString ColorHexStr = FPaperJSONHelpers::ReadString(Tree, TEXT("backgroundcolor"), FString());
    if (!ColorHexStr.IsEmpty())
    {
        OutParsedInfo.BackgroundColor = FColor::FromHex(ColorHexStr);
    }

    // Parse the orientation
    const FString OrientationModeStr = FPaperJSONHelpers::ReadString(Tree, TEXT("orientation"), FString());
    if (OrientationModeStr == TEXT("orthogonal"))
    {
        OutParsedInfo.Orientation = ETiledOrientation::Orthogonal;
    }
    else if (OrientationModeStr == TEXT("isometric"))
    {
        OutParsedInfo.Orientation = ETiledOrientation::Isometric;
    }
    else if (OrientationModeStr == TEXT("staggered"))
    {
        OutParsedInfo.Orientation = ETiledOrientation::Staggered;
    }
    else if (OrientationModeStr == TEXT("hexagonal"))
    {
        OutParsedInfo.Orientation = ETiledOrientation::Hexagonal;
    }
    else
    {
        TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  Invalid value for '%s' ('%s' but expected 'orthogonal', 'isometric', 'staggered', or 'hexagonal')"), *NameForErrors, TEXT("orientation"), *OrientationModeStr);
        bSuccessfullyParsed = false;
        OutParsedInfo.Orientation = ETiledOrientation::Unknown;
    }
}

//////////////////////////////////////////////////////////////////////////
// FTileMapFromTiled

FTileMapFromTiled::FTileMapFromTiled()
    : FileVersion(0)
    , Width(0)
    , Height(0)
    , TileWidth(0)
    , TileHeight(0)
    , Orientation(ETiledOrientation::Unknown)
    , HexSideLength(0)
    , StaggerAxis(ETiledStaggerAxis::Y)
    , StaggerIndex(ETiledStaggerIndex::Odd)
    , RenderOrder(ETiledRenderOrder::RightDown)
    , BackgroundColor(55, 55, 55)
{
}

bool FTileMapFromTiled::IsValid() const
{
    return (FileVersion != 0) && (Width > 0) && (Height > 0) && (TileWidth > 0) && (TileHeight > 0) && (Orientation != ETiledOrientation::Unknown);
}

FPaperTileInfo FTileMapFromTiled::ConvertTileGIDToPaper2D(uint32 GID) const
{
    // Split the GID into flip bits and tile index
    const uint32 Flags = GID >> 29;
    const int32 TileIndex = (int32)(GID & ~(7U << 29));

    FPaperTileInfo Result;

    for (int32 SetIndex = TileSets.Num() - 1; SetIndex >= 0; SetIndex--)
    {
        const int32 RelativeIndex = TileIndex - TileSets[SetIndex].FirstGID;
        if (RelativeIndex >= 0)
        {
            // We've found the source tile set and are done searching, but only import a non-null if that tile set imported successfully
            if (UTITileSet* Set = CreatedTileSetAssets[SetIndex])
            {
                Result.TileSet = Set;
                Result.PackedTileIndex = RelativeIndex;
                Result.SetFlagValue(EPaperTileFlags::FlipHorizontal, (Flags & 0x4) != 0);
                Result.SetFlagValue(EPaperTileFlags::FlipVertical, (Flags & 0x2) != 0);
                Result.SetFlagValue(EPaperTileFlags::FlipDiagonal, (Flags & 0x1) != 0);
            }
            break;
        }
    }

    return Result;
}

ETileMapProjectionMode::Type FTileMapFromTiled::GetOrientationType() const
{
    switch (Orientation)
    {
    case ETiledOrientation::Isometric:
        return ETileMapProjectionMode::IsometricDiamond;
    case ETiledOrientation::Staggered:
        return ETileMapProjectionMode::IsometricStaggered;
    case ETiledOrientation::Hexagonal:
        return ETileMapProjectionMode::HexagonalStaggered;
    case ETiledOrientation::Orthogonal:
    default:
        return ETileMapProjectionMode::Orthogonal;
    };
}

//////////////////////////////////////////////////////////////////////////
// FTileSetFromTiled

FTileSetFromTiled::FTileSetFromTiled()
    : FirstGID(INDEX_NONE)
    , ImageWidth(0)
    , ImageHeight(0)
    , bRemoveTransparentColor(false)
    , ImageTransparentColor(FColor::Magenta)
    , TileOffsetX(0)
    , TileOffsetY(0)
    , Margin(0)
    , Spacing(0)
    , TileWidth(0)
    , TileHeight(0)
{
}

void FTileSetFromTiled::ParseTileSetFromJSON(TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent)
{
    bool bSuccessfullyParsed = true;

    // Parse all of the integer fields
    FRequiredIntField IntFields[] = {
        FRequiredIntField(ImageWidth, TEXT("imagewidth"), 1),
        FRequiredIntField(ImageHeight, TEXT("imageheight"), 1),
        FRequiredIntField(Margin, TEXT("margin"), 0),
        FRequiredIntField(Spacing, TEXT("spacing"), 0),
        FRequiredIntField(TileWidth, TEXT("tilewidth"), 1),
        FRequiredIntField(TileHeight, TEXT("tileheight"), 1)
    };

    bSuccessfullyParsed = bSuccessfullyParsed && ParseIntegerFields(IntFields, UE_ARRAY_COUNT(IntFields), Tree, NameForErrors, bSilent);

    // Parse the tile offset
    if (bSuccessfullyParsed)
    {
        if (FirstGID <= 0)
        {
            Tree->TryGetNumberField(TEXT("firstgid"), FirstGID);
        }

        FIntPoint TileOffsetTemp;

        if (Tree->HasField(TEXT("tileoffset")))
        {
            if (FPaperJSONHelpers::ReadIntPoint(Tree, TEXT("tileoffset"), TileOffsetTemp))
            {
                TileOffsetX = TileOffsetTemp.X;
                TileOffsetY = TileOffsetTemp.Y;
            }
            else
            {
                TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  Invalid or missing value for '%s'"), *NameForErrors, TEXT("tileoffset"));
                bSuccessfullyParsed = false;
            }
        }
    }

    // Parse the tile set name
    Name = FPaperJSONHelpers::ReadString(Tree, TEXT("name"), FString());
    if (Name.IsEmpty())
    {
        //TILED_IMPORT_WARNING(TEXT("Expected a non-empty name for each tile set in '%s', generating a new name"), *NameForErrors);
        Name = FString::Printf(TEXT("TileSetStartingAt%d"), FirstGID);
    }

    // Parse the image path
    if (ImagePath.IsEmpty())
    {
        ImagePath = FPaperJSONHelpers::ReadString(Tree, TEXT("image"), FString());
    }
    
    if (ImagePath.IsEmpty())
    {
        TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  Invalid value for '%s' ('%s' but expected a path to an image)"), *NameForErrors, TEXT("image"), *ImagePath);
        bSuccessfullyParsed = false;
    }

    // Parse the transparent color if present
    const FString TransparentColorStr = FPaperJSONHelpers::ReadString(Tree, TEXT("transparentcolor"), FString());
    if (!TransparentColorStr.IsEmpty())
    {
        bRemoveTransparentColor = true;
        ImageTransparentColor = FColor::FromHex(TransparentColorStr);
    }

    // Parse the custom properties (if present)
    const TArray<TSharedPtr<FJsonValue>>* PropertiesPtr;
    if (Tree->TryGetArrayField(TEXT("properties"), PropertiesPtr))
    {
        Properties = *PropertiesPtr;
    }

    // Parse the terrain types (if present)
    const TArray<TSharedPtr<FJsonValue>>* TerrainTypesArray;
    if (Tree->TryGetArrayField(TEXT("terrains"), TerrainTypesArray))
    {
        TerrainTypes.Reserve(TerrainTypesArray->Num());
        for (TSharedPtr<FJsonValue> TerrainTypeSrc : *TerrainTypesArray)
        {
            FTiledTerrain NewTerrainType;
            bSuccessfullyParsed = bSuccessfullyParsed && NewTerrainType.ParseFromJSON(TerrainTypeSrc->AsObject(), NameForErrors, bSilent);
            TerrainTypes.Add(NewTerrainType);
        }
    }

    // Parse the per-tile metadata if present (collision objects, terrain membership, etc...)
    const TArray<TSharedPtr<FJsonValue>>* TileValues;
    if (Tree->TryGetArrayField(TEXT("tiles"), TileValues))
    {
        for (TSharedPtr<FJsonValue> TileValue : *TileValues)
        {
            if (TSharedPtr<FJsonObject> TileObject = TileValue->AsObject())
            {
                int32 TileIndex;
                if (TileObject->TryGetNumberField(TEXT("id"), TileIndex))
                {
                    FTiledTileInfo& TileInfo = PerTileData.FindOrAdd(TileIndex);
                    TileInfo.ParseTileInfoFromJSON(TileIndex, TileObject, NameForErrors, bSilent);
                }
            }
        }
    }

    //@TODO: Should we parse ImageWidth and ImageHeight?
    // Are these useful if the tile map gets resized to avoid invalidating everything?
}

bool FTileSetFromTiled::IsValid() const
{
    return (TileWidth > 0) && (TileHeight > 0);
}

TSharedPtr<FJsonObject> FTileSetFromTiled::ParseJSON(const FString& FileContents, const FString& NameForErrors, bool bSilent)
{
    // Load the file up (JSON format)
    if (!FileContents.IsEmpty())
    {
        const TSharedRef< TJsonReader<> >& Reader = TJsonReaderFactory<>::Create(FileContents);

        TSharedPtr<FJsonObject> DescriptorObject;
        if (FJsonSerializer::Deserialize(Reader, DescriptorObject) && DescriptorObject.IsValid())
        {
            // File was loaded and deserialized OK!
            return DescriptorObject;
        }
        else
        {
            if (!bSilent)
            {
                //@TODO: PAPER2D: How to correctly surface import errors to the user?
                UE_LOG(LogTIImporter, Warning, TEXT("Failed to parse tile map JSON file '%s'.  Error: '%s'"), *NameForErrors, *Reader->GetErrorMessage());
            }
            return nullptr;
        }
    }
    else
    {
        if (!bSilent)
        {
            UE_LOG(LogTIImporter, Warning, TEXT("Tile map JSON file '%s' was empty.  This tile map cannot be imported."), *NameForErrors);
        }
        return nullptr;
    }
}

//////////////////////////////////////////////////////////////////////////
// FTileLayerFromTiled

FTileLayerFromTiled::FTileLayerFromTiled()
    : Width(0)
    , Height(0)
    , Color(FColor::White)
    , ObjectDrawOrder(ETiledObjectLayerDrawOrder::TopDown)
    , Opacity(1.0f)
    , bVisible(true)
    , LayerType(ETiledLayerType::TileLayer)
    , OffsetX(0)
    , OffsetY(0)
{
}

bool FTileLayerFromTiled::ParseFromJSON(TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent)
{
    bool bSuccessfullyParsed = true;

    // Parse all of the integer fields
    FRequiredIntField IntFields[] = {
        FRequiredIntField(Width, TEXT("width"), 0),
        FRequiredIntField(Height, TEXT("height"), 0),
        FRequiredIntField(OffsetX, TEXT("x"), 0),
        FRequiredIntField(OffsetY, TEXT("y"), 0)
    };

    bSuccessfullyParsed = bSuccessfullyParsed && ParseIntegerFields(IntFields, UE_ARRAY_COUNT(IntFields), Tree, NameForErrors, bSilent);

    if (!Tree->TryGetBoolField(TEXT("visible"), bVisible))
    {
        bVisible = true;
    }

    if (!FPaperJSONHelpers::ReadFloatNoDefault(Tree, TEXT("opacity"), Opacity))
    {
        Opacity = 1.0f;
    }

    if (!Tree->TryGetStringField(TEXT("name"), Name))
    {
        TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  Expected a layer name"), *NameForErrors);
        bSuccessfullyParsed = false;
    }

    // Parse the layer type
    const FString LayerTypeStr = FPaperJSONHelpers::ReadString(Tree, TEXT("type"), FString());
    if (LayerTypeStr == TEXT("tilelayer"))
    {
        if ((Width < 1) || (Height < 1))
        {
            TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  Tile layers should be at least 1x1"), *NameForErrors);
            bSuccessfullyParsed = false;
        }

        LayerType = ETiledLayerType::TileLayer;
    }
    else if (LayerTypeStr == TEXT("objectgroup"))
    {
        LayerType = ETiledLayerType::ObjectGroup;
    }
    else if (LayerTypeStr == TEXT("imagelayer"))
    {
        LayerType = ETiledLayerType::ImageLayer;
    }
    else
    {
        TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  Invalid value for '%s' ('%s' but expected 'tilelayer' or 'objectgroup')"), *NameForErrors, TEXT("type"), *LayerTypeStr);
        bSuccessfullyParsed = false;
    }

    // Parse the object draw order (if present)
    const FString ObjectDrawOrderStr = FPaperJSONHelpers::ReadString(Tree, TEXT("draworder"), FString());
    if (ObjectDrawOrderStr == TEXT("index"))
    {
        ObjectDrawOrder = ETiledObjectLayerDrawOrder::Index;
    }
    else if (ObjectDrawOrderStr == TEXT("topdown"))
    {
        ObjectDrawOrder = ETiledObjectLayerDrawOrder::TopDown;
    }
    else if (!ObjectDrawOrderStr.IsEmpty())
    {
        TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  Invalid value for '%s' ('%s' but expected 'index' or 'topdown')"), *NameForErrors, TEXT("draworder"), *ObjectDrawOrderStr);
        bSuccessfullyParsed = false;
    }

    // Parse the custom properties (if present)
    const TArray<TSharedPtr<FJsonValue>>* PropertiesPtr;
    if (Tree->TryGetArrayField(TEXT("properties"), PropertiesPtr))
    {
        Properties = *PropertiesPtr;
    }

    // Parse the data specific to this layer type
    if (LayerType == ETiledLayerType::TileLayer)
    {
        const TArray<TSharedPtr<FJsonValue>>* DataArray;
        if (Tree->TryGetArrayField(TEXT("data"), DataArray))
        {
            TileIndices.Reserve(DataArray->Num());
            for (TSharedPtr<FJsonValue> TileEntry : *DataArray)
            {
                const double TileIndexAsDouble = TileEntry->AsNumber();
                uint32 TileID = (uint32)TileIndexAsDouble;
                TileIndices.Add(TileID);
            }
        }
        else
        {
            TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  Missing tile data for layer '%s'"), *NameForErrors, *Name);
            bSuccessfullyParsed = false;
        }
    }
    else if (LayerType == ETiledLayerType::ObjectGroup)
    {
        const TArray<TSharedPtr<FJsonValue>>* ObjectArray;
        if (Tree->TryGetArrayField(TEXT("objects"), ObjectArray))
        {
            Objects.Reserve(ObjectArray->Num());
            for (TSharedPtr<FJsonValue> ObjectEntry : *ObjectArray)
            {
                FTiledObject NewObject;
                bSuccessfullyParsed = bSuccessfullyParsed && NewObject.ParseFromJSON(ObjectEntry->AsObject(), NameForErrors, bSilent);
                Objects.Add(NewObject);
            }
        }
        else
        {
            TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  Missing object data for layer '%s'"), *NameForErrors, *Name);
            bSuccessfullyParsed = false;
        }
    }
    else if (LayerType == ETiledLayerType::ImageLayer)
    {
        OverlayImagePath = FPaperJSONHelpers::ReadString(Tree, TEXT("image"), FString());
    }
    else
    {
        if (bSuccessfullyParsed)
        {
            TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  Unknown layer type for layer '%s'"), *NameForErrors, *Name);
            bSuccessfullyParsed = false;
        }
    }

    return bSuccessfullyParsed;
}

bool FTileLayerFromTiled::IsValid() const
{
    return (Width > 0) && (Height > 0) && (TileIndices.Num() == (Width * Height));
}

//////////////////////////////////////////////////////////////////////////
// FTiledTerrain

FTiledTerrain::FTiledTerrain()
    : SolidTileLocalIndex(0)
{
}

bool FTiledTerrain::ParseFromJSON(TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent)
{
    if (!Tree->TryGetStringField(TEXT("name"), TerrainName))
    {
        TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  Terrain entry is missing the 'name' field"), *NameForErrors);
        return false;
    }

    if (!Tree->TryGetNumberField(TEXT("tile"), SolidTileLocalIndex))
    {
        TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  Terrain entry is missing the 'tile' field"), *NameForErrors);
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
// FTiledTileInfo

FTiledTileInfo::FTiledTileInfo()
    : Probability(1.0f)
{
    for (int32 Index = 0; Index < 4; ++Index)
    {
        TerrainIndices[Index] = INDEX_NONE;
    }
}

bool FTiledTileInfo::ParseTileInfoFromJSON(int32 InTileIndex, TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent)
{
    bool bSuccessfullyParsed = true;

    TileIndex = InTileIndex;

    // Try reading the terrain membership array if present
    const TArray<TSharedPtr<FJsonValue>>* TerrainMembershipArray;
    if (Tree->TryGetArrayField(TEXT("terrain"), TerrainMembershipArray))
    {
        if (TerrainMembershipArray->Num() == 4)
        {
            for (int32 Index = 0; Index < 4; ++Index)
            {
                TSharedPtr<FJsonValue> MembershipIndex = (*TerrainMembershipArray)[Index];

                if (!MembershipIndex->TryGetNumber(TerrainIndices[Index]))
                {
                    TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  The 'terrain' array for tile %d should contain 4 indices into the terrain array"), *NameForErrors, TileIndex);
                    bSuccessfullyParsed = false;
                }
            }
        }
        else
        {
            TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  The 'terrain' array for tile %d should contain 4 entries but it contained %d entries"), *NameForErrors, TileIndex, TerrainMembershipArray->Num());
            bSuccessfullyParsed = false;
        }
    }

    // Try reading the probability if present
    double DoubleProbability;
    if (Tree->TryGetNumberField(TEXT("probability"), DoubleProbability))
    {
        Probability = FMath::Clamp<float>((float)DoubleProbability, 0.0f, 1.0f);
    }

    // Try reading the per-tile collision data if present
    // Note: This is really an entire fake objectgroup layer, but only the objects array matters; Tiled doesn't even provide a way to edit the rest of the data.
    const TSharedPtr<FJsonObject>* ObjectGroupSubobject;
    if (Tree->TryGetObjectField(TEXT("objectgroup"), ObjectGroupSubobject))
    {
        const TArray<TSharedPtr<FJsonValue>>* ObjectArray;
        if ((*ObjectGroupSubobject)->TryGetArrayField(TEXT("objects"), ObjectArray))
        {
            Objects.Reserve(ObjectArray->Num());
            for (TSharedPtr<FJsonValue> ObjectEntry : *ObjectArray)
            {
                FTiledObject NewObject;
                bSuccessfullyParsed = bSuccessfullyParsed && NewObject.ParseFromJSON(ObjectEntry->AsObject(), NameForErrors, bSilent);
                Objects.Add(NewObject);
            }
        }
        else
        {
            TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  Expected an 'objects' entry inside 'objectgroup' for tile %d"), *NameForErrors, TileIndex);
            bSuccessfullyParsed = false;
        }
    }

    const TArray<TSharedPtr<FJsonValue>>* PropertiesPtr;
    if (Tree->TryGetArrayField(TEXT("properties"), PropertiesPtr))
    {
        Properties = *PropertiesPtr;
    }

    return bSuccessfullyParsed;
}

//////////////////////////////////////////////////////////////////////////
// FTiledObject

FTiledObject::FTiledObject()
: TiledObjectType(ETiledObjectType::Box)
, ID(0)
, bVisible(true)
, X(0.0)
, Y(0.0)
, Width(0.0)
, Height(0.0)
, RotationDegrees(0.0)
, TileGID(0)
{
}

bool FTiledObject::ParseFromJSON(TSharedPtr<FJsonObject> Tree, const FString& NameForErrors, bool bSilent)
{
    bool bSuccessfullyParsed = true;

    // Parse all of the integer fields
    FRequiredDoubleField FloatFields[] = {
        FRequiredDoubleField(Width, TEXT("width"), 0.0),
        FRequiredDoubleField(Height, TEXT("height"), 0.0),
        FRequiredDoubleField(X, TEXT("x"), -MAX_FLT),
        FRequiredDoubleField(Y, TEXT("y"), -MAX_FLT),
        FRequiredDoubleField(RotationDegrees, TEXT("rotation"), -MAX_FLT)
    };

    bSuccessfullyParsed = bSuccessfullyParsed && ParseScalarFields(FloatFields, UE_ARRAY_COUNT(FloatFields), Tree, NameForErrors, bSilent);

    if (!Tree->TryGetBoolField(TEXT("visible"), bVisible))
    {
        bVisible = true;
    }

    if (!Tree->TryGetStringField(TEXT("name"), Name))
    {
        TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  Expected an object name"), *NameForErrors);
        bSuccessfullyParsed = false;
    }

    if (!Tree->TryGetStringField(TEXT("type"), UserType))
    {
        TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  Expected an object type"), *NameForErrors);
        bSuccessfullyParsed = false;
    }

    if (!Tree->TryGetNumberField(TEXT("id"), ID))
    {
        TILED_IMPORT_ERROR(TEXT("Failed to parse '%s'.  Expected an object ID"), *NameForErrors);
        bSuccessfullyParsed = false;
    }

    // Determine the object type
    if (Tree->TryGetNumberField(TEXT("gid"), TileGID))
    {
        TiledObjectType = ETiledObjectType::PlacedTile;
    }
    else if (Tree->HasField(TEXT("ellipse")))
    {
        TiledObjectType = ETiledObjectType::Ellipse;
    }
    else
    {
        const TArray<TSharedPtr<FJsonValue>>* PointsArray;
        if (Tree->TryGetArrayField(TEXT("polygon"), PointsArray))
        {
            TiledObjectType = ETiledObjectType::Polygon;
            bSuccessfullyParsed = bSuccessfullyParsed && ParsePointArray(Points, *PointsArray, NameForErrors, bSilent);
        }
        else if (Tree->TryGetArrayField(TEXT("polyline"), PointsArray))
        {
            TiledObjectType = ETiledObjectType::Polyline;
            bSuccessfullyParsed = bSuccessfullyParsed && ParsePointArray(Points, *PointsArray, NameForErrors, bSilent);
        }
        else
        {
            TiledObjectType = ETiledObjectType::Box;
        }
    }


    return bSuccessfullyParsed;
}

bool FTiledObject::ParsePointArray(TArray<FVector2D>& OutPoints, const TArray<TSharedPtr<FJsonValue>>& InArray, const FString& NameForErrors, bool bSilent)
{
    bool bSuccessfullyParsed = true;

    OutPoints.Reserve(InArray.Num());
    for (TSharedPtr<FJsonValue> ArrayElement : InArray)
    {
        double X = 0.0;
        double Y = 0.0;

        FRequiredDoubleField FloatFields[] = {
            FRequiredDoubleField(X, TEXT("x"), -MAX_FLT),
            FRequiredDoubleField(Y, TEXT("y"), -MAX_FLT)
        };

        bSuccessfullyParsed = bSuccessfullyParsed && ParseScalarFields(FloatFields, UE_ARRAY_COUNT(FloatFields), ArrayElement->AsObject(), NameForErrors, bSilent);

        OutPoints.Add(FVector2D(X, Y));
    }

    return bSuccessfullyParsed;
}

void FTiledObject::AddToSpriteGeometryCollection(const FVector2D& TileSize, const TArray<FTiledObject>& InObjects, FSpriteGeometryCollection& InOutShapes)
{
    InOutShapes.Shapes.Empty();
    InOutShapes.GeometryType = ESpritePolygonMode::FullyCustom;

    if (!InObjects.IsEmpty())
    {
        for (const FTiledObject& SourceObject : InObjects)
        {
            const FVector2D SourcePos = FVector2D(SourceObject.X, SourceObject.Y);

            bool bCreatedShape = false;
            switch (SourceObject.TiledObjectType)
            {
                case ETiledObjectType::Box:
                {
                    FVector2D Offset;
                    Offset.X = (TileSize.X * -0.5f) + (SourceObject.Width * 0.5f);
                    Offset.Y = (TileSize.Y * -0.5f) + (SourceObject.Height * 0.5f);
                    InOutShapes.AddRectangleShape(SourcePos + Offset, FVector2D(SourceObject.Width, SourceObject.Height));
                    bCreatedShape = true;
                    break;
                }

                case ETiledObjectType::Ellipse:
                {
                    FVector2D Offset;
                    Offset.X = (TileSize.X * -0.5f) + (SourceObject.Width * 0.5f);
                    Offset.Y = (TileSize.Y * -0.5f) + (SourceObject.Height * 0.5f);
                    InOutShapes.AddCircleShape(SourcePos + Offset, FVector2D(SourceObject.Width, SourceObject.Height));
                    bCreatedShape = true;
                    break;
                }

                case ETiledObjectType::Polygon:
                {
                    const FVector2D Offset = TileSize * -0.5f;
                    FSpriteGeometryShape NewShape;
                    NewShape.ShapeType = ESpriteShapeType::Polygon;
                    NewShape.BoxPosition = SourcePos + Offset;
                    NewShape.Vertices = SourceObject.Points;
                    InOutShapes.Shapes.Add(NewShape);
                    bCreatedShape = true;
                    break;
                }
            
                case ETiledObjectType::PlacedTile:
                {
                    UE_LOG(LogTIImporter, Warning, TEXT("Ignoring Tiled Object of type PlacedTile"));
                    break;
                }

                case ETiledObjectType::Polyline:
                {
                    UE_LOG(LogTIImporter, Warning, TEXT("Ignoring Tiled Object of type Polyline"));
                    break;
                }

                default:
                {
                    ensureMsgf(false, TEXT("Unknown enumerant in ETiledObjectType"));
                    break;
                }
            }

            if (bCreatedShape)
            {
                const float RotationUnwound = FMath::Fmod(-SourceObject.RotationDegrees, 360.0);
                InOutShapes.Shapes.Last().Rotation = RotationUnwound;
            }
        }

        InOutShapes.ConditionGeometry();
    }
}

void UTIImporterFactory::FinalizeTileMap(FTileMapFromTiled& GlobalInfo, UTITileMap* TileMap)
{
    const UPaperImporterSettings* ImporterSettings = GetDefault<UPaperImporterSettings>();

    // Bind our selected tile set to the first tile set that was imported so something is already picked
    UTITileSet* DefaultTileSet = (GlobalInfo.CreatedTileSetAssets.Num() > 0) ? GlobalInfo.CreatedTileSetAssets[0] : nullptr;
    TileMap->SelectedTileSet = DefaultTileSet;

    // Initialize some configuration settings
    double PixelsPerUnrealUnit;
    if (!TileMap->GetCustomProperties()->GetNumberProperty("PixelsPerUnrealUnit", PixelsPerUnrealUnit))
    {
        PixelsPerUnrealUnit = ImporterSettings->GetDefaultPixelsPerUnrealUnit();
    }

    TileMap->PixelsPerUnrealUnit = (float)PixelsPerUnrealUnit;

    double SeparationPerLayer;
    if (!TileMap->GetCustomProperties()->GetNumberProperty("SeparationPerLayer", SeparationPerLayer))
    {
        SeparationPerLayer = 1.0f;
    }

    TileMap->SeparationPerLayer = (float)SeparationPerLayer;

    // Analyze the tile set textures (anything with translucent wins; failing that use masked)
    ESpriteInitMaterialType BestMaterial = ESpriteInitMaterialType::Masked;
    if (ImporterSettings->ShouldPickBestMaterialWhenCreatingTileMaps())
    {
        BestMaterial = ESpriteInitMaterialType::Automatic;
        for (UTITileSet* TileSet : GlobalInfo.CreatedTileSetAssets)
        {
            if ((TileSet != nullptr) && (TileSet->GetTileSheetTexture() != nullptr))
            {
                ESpriteInitMaterialType TileSheetMaterial = ImporterSettings->AnalyzeTextureForDesiredMaterialType(TileSet->GetTileSheetTexture(), FIntPoint::ZeroValue, TileSet->GetTileSheetAuthoredSize());

                switch (TileSheetMaterial)
                {
                case ESpriteInitMaterialType::Opaque:
                case ESpriteInitMaterialType::Masked:
                    BestMaterial = ((BestMaterial == ESpriteInitMaterialType::Automatic) || (BestMaterial == ESpriteInitMaterialType::Opaque)) ? TileSheetMaterial : BestMaterial;
                    break;
                case ESpriteInitMaterialType::Translucent:
                    BestMaterial = TileSheetMaterial;
                    break;
                }
            }
        }
    }

    if (BestMaterial == ESpriteInitMaterialType::Automatic)
    {
        // Fall back to masked if we wanted automatic and couldn't analyze things
        BestMaterial = ESpriteInitMaterialType::Masked;
    }

    if (BestMaterial != ESpriteInitMaterialType::LeaveAsIs)
    {
        const bool bUseLitMaterial = false;
        TileMap->Material = ImporterSettings->GetDefaultMaterial(BestMaterial, bUseLitMaterial);
    }
}

//////////////////////////////////////////////////////////////////////////

#undef TILED_IMPORT_ERROR
#undef LOCTEXT_NAMESPACE