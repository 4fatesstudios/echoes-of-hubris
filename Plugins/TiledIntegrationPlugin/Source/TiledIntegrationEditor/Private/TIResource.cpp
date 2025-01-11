// Copyright Flekz Games. All Rights Reserved.

#include "TIResource.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Dom/JsonObject.h"
#include "FileHelpers.h"
#include "HAL/FileManagerGeneric.h"
#include "PaperTileMap.h"
#include "PaperTileSet.h"
#include "ObjectTools.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "Serialization/JsonSerializer.h"
#include "TIImporterFactory.h"
#include "TISettings.h"
#include "TITileMap.h"
#include "TITileSet.h"

UTIResource::UTIResource(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, ID(INDEX_NONE)
, Type(ETIResourceType::Invalid)
, bShouldAutoReimport(false)
, bManuallyImported(false)
, LastImportTime(0)
, bCanAutoReimport(true)
{

}

UObject* UTIResource::ImportAsset(UTIResourceManager* ResourceManager, const FString& TargetPath)
{
    UObject* ImportedAsset = nullptr;

    // Prevent importing if asset already imported
    if (GetAsset() == nullptr)
    {
        // Generate a valid package path
        FString DestinationFilePath = FPaths::GetPath(TargetPath) / FPaths::GetBaseFilename(TargetPath);
        if (!TargetPath.StartsWith("/Game"))
        {
            FPaths::MakePathRelativeTo(DestinationFilePath, *FPaths::ProjectContentDir());
            DestinationFilePath = TEXT("/Game") / DestinationFilePath;
        }

        FString AssetName, PackageName;
        FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
        AssetToolsModule.Get().CreateUniqueAssetName(DestinationFilePath, TEXT(""), PackageName, AssetName);

        UPackage* Package = CreatePackage(*PackageName);

        UTIImporterFactory* ImporterFactory = NewObject<UTIImporterFactory>();
        ImporterFactory->AddToRoot();

        // Store the imported package name as the asset path
        AssetPath = Package->GetName();

        if (UObject* Asset = ImportAssetInternal(ResourceManager, Package, ImporterFactory))
        {
            if (IsValid())
            {
                // Mark as dirty to be saved
                ImportedAsset = Asset;
                FAssetRegistryModule::AssetCreated(ImportedAsset);
                ImportedAsset->MarkPackageDirty();
                ULevel::LevelDirtiedEvent.Broadcast();
                ImportedAsset->PostEditChange();
                SaveAsset();
            }
        }

        ImporterFactory->RemoveFromRoot();
    }

    return ImportedAsset;
}

bool UTIResource::ReimportAsset(UTIResourceManager* ResourceManager)
{
    bool Success = false;
    if (IsValid(true, false))
    {
        if (IsSourceModified())
        {
            UObject* Asset = GetAsset();
            UTIImporterFactory* ImporterFactory = NewObject<UTIImporterFactory>();
            ImporterFactory->AddToRoot();

            if (ReimportAssetInternal(ResourceManager, Asset->GetOuter(), ImporterFactory))
            {
                GetSourceFileModifiedTime(LastImportTime);
                Asset->MarkPackageDirty();
                Success = true;
                SaveAsset();
            }

            ImporterFactory->RemoveFromRoot();
        }
        else
        {
            Success = true;
        }
    }

    return Success;
}

bool UTIResource::DeleteAsset(bool bAskForConfirmation)
{
    if (UObject* Asset = GetAsset())
    {
        FAssetData AssetData(Asset);
        if (AssetData.IsValid())
        {
            TArray<FAssetData> AssetsToDelete;
            AssetsToDelete.Add(AssetData);
            return ObjectTools::DeleteAssets(AssetsToDelete, bAskForConfirmation) > 0;
        }

        return false;
    }
    else
    {
        return true;
    }
}

bool UTIResource::SaveAsset()
{   
    if (UObject* Asset = GetAsset())
    {
        if (UPackage* Package = Asset->GetPackage())
        {
            if (UEditorLoadingAndSavingUtils::SavePackages({ Package }, true))
            {
                // Save dependencies as well
                for (UTIResource* Dependency : GetDependencies())
                {
                    Dependency->SaveAsset();
                }

                return true;
            }
        }
    }

    return false;
}

void UTIResource::AddUser(UTIResource* Resource)
{
    if (Resource)
    {
        UsedBy.Add(Resource->ID, Resource);
    }
}

void UTIResource::RemoveUser(UTIResource* Resource)
{
    if (Resource)
    {
        UsedBy.Remove(Resource->ID);
    }
}

bool UTIResource::IsInUse(UTIResource* Resource) const
{
    bool bInUse = false;
    if (Resource == nullptr)
    {
        if (!bManuallyImported)
        {
            for (const auto& User : UsedBy)
            {
                if (User.Value.IsValid())
                {
                    bInUse = true;
                    break;
                }
            }
        }
        else
        {
            bInUse = true;
        }
    }
    else
    {
        for (const auto& User : UsedBy)
        {
            if (User.Value.IsValid() && Resource->ID == User.Value->ID)
            {
                bInUse = true;
                break;
            }
        }
    }

    return bInUse;
}

void UTIResource::AddDependency(UTIResource* Resource)
{
    if (Resource)
    {
        Dependencies.Add(Resource->ID, Resource);
        Resource->AddUser(this);
    }
}

void UTIResource::RemoveDependency(UTIResource* Resource)
{
    if (Resource)
    {
        Dependencies.Remove(Resource->ID);
        Resource->RemoveUser(this);
    }
}

void UTIResource::RemoveAllDependencies()
{
    TArray<UTIResource*> ResourceDependencies = GetDependencies();
    for (UTIResource* Dependency : ResourceDependencies)
    {
        RemoveDependency(Dependency);
    }

    Dependencies.Empty();
}

bool UTIResource::HasDependency(UTIResource* Resource) const
{
    if (Resource == nullptr)
    {
        return !Dependencies.IsEmpty();
    }
    else
    {
        return Dependencies.Find(Resource->ID) != nullptr;
    }
}

bool UTIResource::ShouldAutoReimport() const
{
    if (bShouldAutoReimport && bCanAutoReimport)
    {
        return IsSourceModified();
    }

    return false;
}

bool UTIResource::IsSourceModified() const
{
    FDateTime ModifiedTime;
    if (GetSourceFileModifiedTime(ModifiedTime))
    {
        return ModifiedTime > LastImportTime;
    }

    return false;
}

TArray<UTIResource*> UTIResource::GetDependencies(bool bUsedBy) const
{
    TArray<UTIResource*> OutDependencies;
    if (bUsedBy)
    {
        for (auto User : UsedBy)
        {
            if (User.Value.IsValid())
            {
                OutDependencies.Add(User.Value.Get());
            }
        }
    }
    else
    {
        for (auto Dependency : Dependencies)
        {
            if (Dependency.Value)
            {
                OutDependencies.Add(Dependency.Value);
            }
        }
    }

    return OutDependencies;
}

bool UTIResource::GetSourceFileModifiedTime(FDateTime& OutModifiedTime) const
{
    if (IsSourceValid())
    {
        FFileManagerGeneric FileManager;
        OutModifiedTime = FileManager.GetTimeStamp(*GetAbsoluteSourcePath());
        return true;
    }

    return false;
}

bool UTIResource::IsValid(bool bCheckAsset, bool bCheckDependencyAssets) const
{
    if (Type != ETIResourceType::Invalid)
    {
        if (IsSourceValid() && (!bCheckAsset || IsAssetValid()) && AreDependenciesValid(false, bCheckDependencyAssets))
        {
            return true;
        }
    }

    return false;
}

bool UTIResource::IsAssetValid() const
{
    return GetAsset() != nullptr;
}

bool UTIResource::IsSourceValid() const
{
    return FPaths::FileExists(GetAbsoluteSourcePath());
}

bool UTIResource::AreDependenciesValid(bool bUsedBy, bool bCheckDependencyAssets) const
{
    if (bUsedBy)
    {
        for (auto User : UsedBy)
        {
            if (!User.Value.IsValid() || !User.Value->IsValid(bCheckDependencyAssets, bCheckDependencyAssets))
            {
                return false;
            }
        }
    }
    else
    {
        for (auto Dependency : Dependencies)
        {
            if (!Dependency.Value || !Dependency.Value->IsValid(bCheckDependencyAssets, bCheckDependencyAssets))
            {
                return false;
            }
        }
    }

    return true;
}

UObject* UTIResource::GetAsset() const
{
    return LoadObject<UObject>(NULL, *AssetPath, nullptr, LOAD_Quiet | LOAD_NoWarn);
}

FString UTIResource::GetRelativeSourcePath() const
{
    return SourcePath;
}

FString UTIResource::GetRelativeAssetPath() const
{
    return AssetPath;
}

FString UTIResource::GetAbsoluteSourcePath() const
{
    FString ProjectDir = FPaths::ProjectDir();
    if (ProjectDir.EndsWith(TEXT("/")))
    {
        ProjectDir.RemoveFromEnd(TEXT("/"));
    }

    return ProjectDir + SourcePath;
}

FString UTIResource::GetAbsoluteAssetPath() const
{
    FString ProjectDir = FPaths::ProjectDir();
    if (ProjectDir.EndsWith(TEXT("/")))
    {
        ProjectDir.RemoveFromEnd(TEXT("/"));
    }

    FString AbsolutePath = ProjectDir + AssetPath + TEXT(".uasset");
    AbsolutePath.ReplaceInline(TEXT("/Game/"), TEXT("/Content/"));
    return AbsolutePath;
}

UTITextureResource::UTITextureResource(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
    Type = ETIResourceType::Texture;
}

UObject* UTITextureResource::ImportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory)
{
    return Factory->ImportTexture(this, ResourceManager, Package);
}

bool UTITextureResource::ReimportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory)
{
    if (LoadFromFile(GetAbsoluteSourcePath(), ResourceManager, true, true, this))
    {
        return Factory->ImportTexture(this, ResourceManager, Package) != nullptr;
    }

    return false;
}

UTITextureResource* UTITextureResource::LoadFromFile(const FString& FilePath, UTIResourceManager* ResourceManager, bool bSilent, bool bReloadDependencies, UTITextureResource* Resource)
{
    UTITextureResource* Texture = nullptr;

    if (FPaths::FileExists(FilePath))
    {
        bool bShouldUpdate = false;
        bool bShouldCreate = Resource == nullptr;

        if (Resource == nullptr)
        {
            // Check if the resource already exists
            if (UTIResource* ExistingTexture = ResourceManager->FindResource(FilePath, ETIResourceType::Texture))
            {
                Texture = Cast<UTITextureResource>(ExistingTexture);
                bShouldCreate = false;
                bShouldUpdate = bReloadDependencies;
            }
        }
        else
        {
            Texture = Resource;
            bShouldUpdate = bReloadDependencies;
        }

        if (bShouldCreate || bShouldUpdate)
        {
            if (bShouldCreate)
            {
                Texture = NewObject<UTITextureResource>();
                Texture->ID = ResourceManager->GenerateResourceID();
                Texture->SourcePath = FilePath;
                Texture->SourcePath.ReplaceInline(*FPaths::ProjectDir(), TEXT("/"));
                Texture->Name = FPaths::GetBaseFilename(FilePath);
            }
        }
    }
    else if (!bSilent)
    {
        FString ErrorTitle = TEXT("Failed to import Tiled resource");
        FString ErrorMsg = FString::Printf(TEXT("The file '%s' was not found"), *FilePath);
        FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
    }

    return Texture;
}

UTISpriteResource::UTISpriteResource(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
    Type = ETIResourceType::Sprite;
    bCanAutoReimport = false;
}

TArray<UTISpriteResource*> UTISpriteResource::LoadFromFlipbook(UTIFlipbookResource* Flipbook, UTIResourceManager* ResourceManager, bool bSilent, bool bReloadDependencies, UTISpriteResource* Resource)
{
    TArray<UTISpriteResource*> Sprites;
    if (Flipbook)
    {
        const TArray<TSharedPtr<FJsonValue>>* AnimationsData;
        if (Flipbook->GetJsonData()->TryGetArrayField(TEXT("animation"), AnimationsData) && AnimationsData && !AnimationsData->IsEmpty())
        {
            for (TSharedPtr<FJsonValue> AnimationDataPtr : *AnimationsData)
            {
                TSharedPtr<FJsonObject> AnimationData = AnimationDataPtr->AsObject();

                int32 TileID;
                if (AnimationData->TryGetNumberField(TEXT("tileid"), TileID))
                {
                    UTISpriteResource* Sprite = nullptr;

                    bool bShouldUpdate = false;
                    bool bShouldCreate = Resource == nullptr;

                    const FString SpriteName = FString::FromInt(TileID);

                    if (Resource == nullptr)
                    {
                        // Check if the resource already exists
                        if (UTIResource* ExistingSprite = ResourceManager->FindResource(Flipbook->GetAbsoluteSourcePath(), ETIResourceType::Sprite, SpriteName))
                        {
                            Sprite = Cast<UTISpriteResource>(ExistingSprite);
                            bShouldCreate = false;
                            bShouldUpdate = bReloadDependencies;
                        }
                    }
                    else if (SpriteName == Resource->Name)
                    {
                        Sprite = Resource;
                        bShouldUpdate = bReloadDependencies;
                    }

                    if (bShouldCreate || bShouldUpdate)
                    {
                        if (bShouldCreate)
                        {
                            Sprite = NewObject<UTISpriteResource>();
                            Sprite->ID = ResourceManager->GenerateResourceID();
                            Sprite->SourcePath = Flipbook->SourcePath;
                            Sprite->Name = SpriteName;
                        }
                        else
                        {
                            // Remove the dependencies and re add them if reloading
                            Sprite->RemoveAllDependencies();
                        }

                        if (UTITileSetResource* TileSet = Flipbook->GetTileSet())
                        {
                            Sprite->AddDependency(TileSet->GetTexture());
                        }
                    }

                    Sprites.Add(Sprite);
                }
                else if (!bSilent)
                {
                    FString ErrorTitle = TEXT("Failed to import Tiled resource");
                    FString ErrorMsg = FString::Printf(TEXT("The tile set file '%s' has an invalid animated tile"), *Flipbook->SourcePath);
                    FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
                }
            }
        }
    }

    return Sprites;
}

UTITileSetResource* UTISpriteResource::GetTileSet() const
{
    UTITileSetResource* TileSet = nullptr;
    if (UTIFlipbookResource* Flipbook = GetFlipbook())
    {
        TileSet = Flipbook->GetTileSet();
    }

    return TileSet;
}

UTITextureResource* UTISpriteResource::GetTexture() const
{
    UTITextureResource* Texture = nullptr;
    for (const auto& Dependency : Dependencies)
    {
        if (Dependency.Value && Dependency.Value->Type == ETIResourceType::Texture)
        {
            Texture = Cast<UTITextureResource>(Dependency.Value);
        }
    }

    return Texture;
}

UTIFlipbookResource* UTISpriteResource::GetFlipbook() const
{
    UTIFlipbookResource* Flipbook = nullptr;
    for (const auto& Pair : UsedBy)
    {
        if (Pair.Value.IsValid() && Pair.Value->Type == ETIResourceType::Flipbook)
        {
            Flipbook = Cast<UTIFlipbookResource>(Pair.Value.Get());
            break;
        }
    }

    return Flipbook;
}

int32 UTISpriteResource::GetTileID() const
{
    return !Name.IsEmpty() ? FCString::Atoi(*Name) : INDEX_NONE;
}

UObject* UTISpriteResource::ImportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory)
{
    return Factory->ImportSprite(this, ResourceManager, Package);
}

bool UTISpriteResource::ReimportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory)
{
    if (!LoadFromFlipbook(GetFlipbook(), ResourceManager, true, true, this).IsEmpty())
    {
        return Factory->ImportSprite(this, ResourceManager, Package) != nullptr;
    }

    return false;
}

UTIFlipbookResource::UTIFlipbookResource(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
    Type = ETIResourceType::Flipbook;
    bCanAutoReimport = false;
}

bool UTIFlipbookResource::LoadJson()
{
    if (UTITileSetResource* TileSet = GetTileSet())
    {
        if (!TileSet->IsJsonDataValid())
        {
            TileSet->LoadJson();
        }

        if (TileSet->IsJsonDataValid())
        {
            const TArray<TSharedPtr<FJsonValue>>* TilesData;
            if (TileSet->GetJsonData()->TryGetArrayField(TEXT("tiles"), TilesData) && TilesData && !TilesData->IsEmpty())
            {
                const int32 TileID = FCString::Atoi(*Name);
                for (TSharedPtr<FJsonValue> TileDataPtr : *TilesData)
                {
                    TSharedPtr<FJsonObject> TileData = TileDataPtr->AsObject();

                    int32 PossibleTileID;
                    if (TileData->TryGetNumberField(TEXT("id"), PossibleTileID) && PossibleTileID == TileID)
                    {
                        JsonData = MakeShared<FJsonObject>();
                        FJsonObject::Duplicate(TileData, JsonData);
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

TArray<UTIFlipbookResource*> UTIFlipbookResource::LoadFromTileSet(UTITileSetResource* TileSet, UTIResourceManager* ResourceManager, bool bSilent, bool bReloadDependencies, UTIFlipbookResource* Resource)
{
    TArray<UTIFlipbookResource*> Flipbooks;
    if (TileSet)
    {
        const TArray<TSharedPtr<FJsonValue>>* TilesData;
        if (TileSet->GetJsonData()->TryGetArrayField(TEXT("tiles"), TilesData) && TilesData && !TilesData->IsEmpty())
        {
            for (TSharedPtr<FJsonValue> TileDataPtr : *TilesData)
            {
                // Check if the tile has animation data
                TSharedPtr<FJsonObject> TileData = TileDataPtr->AsObject();
                if (TileData->HasField(TEXT("animation")))
                {
                    int32 TileID;
                    if (TileData->TryGetNumberField(TEXT("id"), TileID))
                    {
                        UTIFlipbookResource* Flipbook = nullptr;

                        bool bShouldUpdate = false;
                        bool bShouldCreate = Resource == nullptr;

                        const FString FlipbookName = FString::FromInt(TileID);
                        if (Resource == nullptr)
                        {
                            // Check if the resource already exists
                            if (UTIResource* ExistingFlipbook = ResourceManager->FindResource(TileSet->GetAbsoluteSourcePath(), ETIResourceType::Flipbook, FlipbookName))
                            {
                                Flipbook = Cast<UTIFlipbookResource>(ExistingFlipbook);
                                bShouldCreate = false;
                                bShouldUpdate = bReloadDependencies;
                            }
                        }
                        else if (FlipbookName == Resource->Name)
                        {
                            Flipbook = Resource;
                            bShouldUpdate = bReloadDependencies;
                        }
                        
                        if (bShouldCreate || bShouldUpdate)
                        {
                            if (bShouldCreate)
                            {
                                Flipbook = NewObject<UTIFlipbookResource>();
                                Flipbook->ID = ResourceManager->GenerateResourceID();
                                Flipbook->SourcePath = TileSet->SourcePath;
                                Flipbook->Name = FlipbookName;
                            }
                            else
                            {
                                // Remove the dependencies and re add them if reloading
                                Flipbook->RemoveAllDependencies();
                            }

                            TileSet->AddDependency(Flipbook);
                            Flipbook->LoadJson();

                            // Load sprites
                            TArray<UTISpriteResource*> Sprites = UTISpriteResource::LoadFromFlipbook(Flipbook, ResourceManager, bSilent, bReloadDependencies);
                            for (UTISpriteResource* Sprite : Sprites)
                            {
                                Flipbook->AddDependency(Sprite);
                            }
                        }

                        if (Flipbook)
                        {
                            Flipbooks.Add(Flipbook);
                        }
                    }
                    else if (!bSilent)
                    {
                        FString ErrorTitle = TEXT("Failed to import Tiled resource");
                        FString ErrorMsg = FString::Printf(TEXT("The tile set file '%s' has an invalid animated tile"), *TileSet->SourcePath);
                        FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
                    }
                }
            }
        }
    }

    return Flipbooks;
}

UTITileSetResource* UTIFlipbookResource::GetTileSet() const
{
    UTITileSetResource* TileSet = nullptr;
    for (const auto& Pair : UsedBy)
    {
        if (Pair.Value.IsValid() && Pair.Value->Type == ETIResourceType::TileSet)
        {
            TileSet = Cast<UTITileSetResource>(Pair.Value.Get());
            break;
        }
    }

    return TileSet;
}

TArray<UTISpriteResource*> UTIFlipbookResource::GetSprites() const
{
    TArray<UTISpriteResource*> Sprites;
    for (const auto& Dependency : Dependencies)
    {
        if (Dependency.Value && Dependency.Value->Type == ETIResourceType::Sprite)
        {
            Sprites.Add(Cast<UTISpriteResource>(Dependency.Value));
        }
    }

    return Sprites;
}

int32 UTIFlipbookResource::GetTileID() const
{
    return !Name.IsEmpty() ? FCString::Atoi(*Name) : INDEX_NONE;
}

UObject* UTIFlipbookResource::ImportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory)
{
    return Factory->ImportFlipbook(this, ResourceManager, Package);
}

bool UTIFlipbookResource::ReimportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory)
{
    if (!LoadFromTileSet(GetTileSet(), ResourceManager, true, true, this).IsEmpty())
    {
        return Factory->ImportFlipbook(this, ResourceManager, Package) != nullptr;
    }

    return false;
}

UTITileSetResource::UTITileSetResource(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
    Type = ETIResourceType::TileSet;
    bIsEmbedded = false;
}

UObject* UTITileSetResource::ImportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory)
{
    return Factory->ImportTileSet(this, ResourceManager, Package);
}

bool UTITileSetResource::ReimportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory)
{
    if (!LoadFromFile(GetAbsoluteSourcePath(), ResourceManager, true, true, this).IsEmpty())
    {
        return Factory->ImportTileSet(this, ResourceManager, Package) != nullptr;
    }

    return false;
}

bool UTITileSetResource::LoadJson()
{
    if (Super::LoadJson())
    {
        if (bIsEmbedded)
        {
            // Find the tile set data in the tile map json
            const TArray<TSharedPtr<FJsonValue>>* TileSetsData;
            if (JsonData->TryGetArrayField(TEXT("tilesets"), TileSetsData) && TileSetsData && !TileSetsData->IsEmpty())
            {
                for (TSharedPtr<FJsonValue> TileSet : *TileSetsData)
                {
                    TSharedPtr<FJsonObject> TileSetData = TileSet->AsObject();

                    FString TileSetName;
                    if (TileSetData->TryGetStringField(TEXT("name"), TileSetName))
                    {
                        if (TileSetName == Name)
                        {
                            JsonData = MakeShared<FJsonObject>();
                            FJsonObject::Duplicate(TileSetData, JsonData);
                            return true;
                        }
                    }
                }
            }
        }
        else
        {
            return true;
        }
    }

    return false;
}

TArray<UTITileSetResource*> UTITileSetResource::LoadFromFile(const FString& FilePath, UTIResourceManager* ResourceManager, bool bSilent, bool bReloadDependencies, UTITileSetResource* Resource)
{
    TArray<UTITileSetResource*> TileSets;
    if (FPaths::FileExists(FilePath))
    {
        TSharedPtr<FJsonObject> FileData = MakeShareable(new FJsonObject);
        if (UTIResourceManager::LoadJsonFromFile(FilePath, FileData))
        {
            auto LoadTileSet = [&FilePath, &ResourceManager, &bSilent, &bReloadDependencies, &Resource](const FString& TileSetFilePath, const TSharedPtr<FJsonObject>& InJsonData) -> UTITileSetResource*
            {
                UTITileSetResource* TileSet = nullptr;

                bool bShouldUpdate = false;
                bool bShouldCreate = Resource == nullptr;

                FString TileSetName;
                if (InJsonData->TryGetStringField(TEXT("name"), TileSetName))
                {
                    if (Resource == nullptr)
                    {
                        // Check if the resource already exists
                        if (UTIResource* ExistingTileSet = ResourceManager->FindResource(TileSetFilePath, ETIResourceType::TileSet, TileSetName))
                        {
                            TileSet = Cast<UTITileSetResource>(ExistingTileSet);
                            bShouldCreate = false;
                            bShouldUpdate = bReloadDependencies;
                        }
                    }
                    else if (TileSetName == Resource->Name)
                    {
                        TileSet = Resource;
                        bShouldUpdate = bReloadDependencies;
                    }
                    
                    if (bShouldCreate || bShouldUpdate)
                    {
                        FString TextureFilePath;
                        if (InJsonData->TryGetStringField(TEXT("image"), TextureFilePath))
                        {
                            TextureFilePath = FPaths::GetPath(TileSetFilePath) / TextureFilePath;
                            if (UTITextureResource* Texture = UTITextureResource::LoadFromFile(TextureFilePath, ResourceManager, bSilent, bReloadDependencies))
                            {
                                if (bShouldCreate)
                                {
                                    TileSet = NewObject<UTITileSetResource>();
                                    TileSet->ID = ResourceManager->GenerateResourceID();
                                    TileSet->SourcePath = TileSetFilePath;
                                    TileSet->bIsEmbedded = TileSetFilePath == FilePath;
                                    TileSet->SourcePath.ReplaceInline(*FPaths::ProjectDir(), TEXT("/"));
                                    TileSet->Name = TileSetName;
                                }
                                else
                                {
                                    // Remove the dependencies and re add them if reloading
                                    TileSet->RemoveAllDependencies();
                                }

                                TileSet->LoadJson();
                                TileSet->AddDependency(Texture);

                                // Load animated tiles (flipbooks)
                                TArray<UTIFlipbookResource*> Flipbooks = UTIFlipbookResource::LoadFromTileSet(TileSet, ResourceManager, bSilent, bReloadDependencies);
                                for (UTIFlipbookResource* Flipbook : Flipbooks)
                                {
                                    TileSet->AddDependency(Flipbook);
                                }
                            }
                        }
                    }
                }

                return TileSet;
            };

            // Check if the file is a tilemap
            FString FileType;
            if (FileData->TryGetStringField(TEXT("type"), FileType) && (FileType == TEXT("tileset")))
            {
                if (UTITileSetResource* TileSet = LoadTileSet(FilePath, FileData))
                {
                    TileSets.Add(TileSet);
                }
            }
            else if (FileType == TEXT("map"))
            {
                const TArray<TSharedPtr<FJsonValue>>* TileSetsData;
                if (FileData->TryGetArrayField(TEXT("tilesets"), TileSetsData) && TileSetsData && !TileSetsData->IsEmpty())
                {
                    for (TSharedPtr<FJsonValue> TileSetDataPtr : *TileSetsData)
                    {
                        TSharedPtr<FJsonObject> TileSetData = TileSetDataPtr->AsObject();

                        FString TileSetFileName;
                        FString TileSetFilePath = FilePath;
                        if (TileSetData->TryGetStringField(TEXT("source"), TileSetFileName))
                        {
                            TileSetFilePath = FPaths::GetPath(FilePath) / TileSetFileName;
                            if (!UTIResourceManager::LoadJsonFromFile(TileSetFilePath, TileSetData))
                            {
                                if (!bSilent)
                                {
                                    FString ErrorTitle = TEXT("Failed to import Tiled resource");
                                    FString ErrorMsg = FString::Printf(TEXT("The file '%s' is referencing to an invalid tile set '%s'"), *FilePath, *TileSetFileName);
                                    FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
                                }
                                
                                break;
                            }
                        }

                        if (UTITileSetResource* TileSet = LoadTileSet(TileSetFilePath, TileSetData))
                        {
                            TileSets.Add(TileSet);
                        }
                    }
                }
            }

            if (!bSilent && TileSets.IsEmpty())
            {
                FString ErrorTitle = TEXT("Failed to import Tiled resource");
                FString ErrorMsg = FString::Printf(TEXT("The file '%s' is not a valid tile set file"), *FilePath);
                FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
            }
        }
        else if (!bSilent)
        {
            FString ErrorTitle = TEXT("Failed to import Tiled resource");
            FString ErrorMsg = FString::Printf(TEXT("The file '%s' is not a valid json file"), *FilePath);
            FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
        }
    }
    else if (!bSilent)
    {
        FString ErrorTitle = TEXT("Failed to import Tiled resource");
        FString ErrorMsg = FString::Printf(TEXT("The file '%s' was not found"), *FilePath);
        FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
    }

    return TileSets;
}

UTITextureResource* UTITileSetResource::GetTexture() const
{
    UTITextureResource* Texture = nullptr;
    for (const auto& Dependency : Dependencies)
    {
        if (Dependency.Value && Dependency.Value->Type == ETIResourceType::Texture)
        {
            Texture = Cast<UTITextureResource>(Dependency.Value);
        }
    }

    return Texture;
}

TArray<UTIFlipbookResource*> UTITileSetResource::GetFlipbooks() const
{
    TArray<UTIFlipbookResource*> Flipbooks;
    for (const auto& Dependency : Dependencies)
    {
        if (Dependency.Value && Dependency.Value->Type == ETIResourceType::Flipbook)
        {
            Flipbooks.Add(Cast<UTIFlipbookResource>(Dependency.Value));
        }
    }

    return Flipbooks;
}

TArray<UTITileMapResource*> UTITileSetResource::GetTileMaps() const
{
    TArray<UTITileMapResource*> TileMaps;
    for (const auto& Pair : UsedBy)
    {
        if (Pair.Value.IsValid() && Pair.Value->Type == ETIResourceType::TileMap)
        {
            if (UTITileMapResource* TileMap = Cast<UTITileMapResource>(Pair.Value.Get()))
            {
                TileMaps.Add(TileMap);
            }
        }
    }

    return TileMaps;
}

UTITileMapResource::UTITileMapResource(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
    Type = ETIResourceType::TileMap;
}

void UTITileMapResource::RemoveDependency(UTIResource* Resource)
{
    if (Resource->Type == ETIResourceType::TileSet)
    {
        RemoveTileSet(Cast<UTITileSetResource>(Resource));
    }
    else
    {
        Super::RemoveDependency(Resource);
    }
}

UObject* UTITileMapResource::ImportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory)
{
    return Factory->ImportTileMap(this, ResourceManager, Package);
}

bool UTITileMapResource::ReimportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory)
{
    if (LoadFromFile(GetAbsoluteSourcePath(), ResourceManager, true, true, this))
    {
        return Factory->ImportTileMap(this, ResourceManager, Package) != nullptr;
    }

    return false;
}

void UTITileMapResource::AddTileSet(UTITileSetResource* TileSet, int32 FirstGID)
{
    TileSets.Add(FirstGID, TileSet);
    Super::AddDependency(TileSet);
}

void UTITileMapResource::RemoveTileSet(UTITileSetResource* InTileSet)
{
    for (const auto& TileSet : TileSets)
    {
        if (TileSet.Value.Get() == InTileSet)
        {
            TileSets.Remove(TileSet.Key);
            break;
        }
    }

    Super::RemoveDependency(InTileSet);
}

UTITileMapResource* UTITileMapResource::LoadFromFile(const FString& FilePath, UTIResourceManager* ResourceManager, bool bSilent, bool bReloadDependencies, UTITileMapResource* Resource)
{
    UTITileMapResource* TileMap = nullptr;
    if (FPaths::FileExists(FilePath))
    {
        TSharedPtr<FJsonObject> FileData = MakeShareable(new FJsonObject);
        if (UTIResourceManager::LoadJsonFromFile(FilePath, FileData))
        {
            // Check if the file is a tilemap
            FString FileType;
            if (FileData->TryGetStringField(TEXT("type"), FileType) && (FileType == TEXT("map")))
            {
                bool bShouldUpdate = false;
                bool bShouldCreate = Resource == nullptr;

                // Check if the resource already exists
                const FString TileMapName = FPaths::GetBaseFilename(FilePath);

                if (Resource == nullptr)
                {
                    if (UTIResource* ExistingTileMap = ResourceManager->FindResource(FilePath, ETIResourceType::TileMap, TileMapName))
                    {
                        TileMap = Cast<UTITileMapResource>(ExistingTileMap);
                        bShouldCreate = false;
                        bShouldUpdate = bReloadDependencies;
                    }
                }
                else if (TileMapName == Resource->Name)
                {
                    TileMap = Resource;
                    bShouldUpdate = bReloadDependencies;
                }
                
                if (bShouldCreate || bShouldUpdate)
                {
                    // Check if the file has a valid tile set
                    const TArray<TSharedPtr<FJsonValue>>* TileSetsData;
                    TMap<int32, TWeakObjectPtr<UTITileSetResource>> TileSets;
                    if (FileData->TryGetArrayField(TEXT("tilesets"), TileSetsData) && TileSetsData && !TileSetsData->IsEmpty())
                    {
                        // Find the gid for the tile sets
                        TArray<UTITileSetResource*> LoadedTileSets = UTITileSetResource::LoadFromFile(FilePath, ResourceManager, bSilent, bReloadDependencies);
                        for (UTITileSetResource* TileSet : LoadedTileSets)
                        {
                            int32 FirstGID = INDEX_NONE;
                            for (TSharedPtr<FJsonValue> TileSetDataPtr : *TileSetsData)
                            {
                                TSharedPtr<FJsonObject> TileSetData = TileSetDataPtr->AsObject();
                                
                                int32 PossibleFirstGID;
                                TileSetData->TryGetNumberField(TEXT("firstgid"), PossibleFirstGID);

                                FString TileSetFileName;
                                if (TileSetData->TryGetStringField(TEXT("source"), TileSetFileName))
                                {
                                    FString TileSetFilePath = FPaths::GetPath(FilePath) / TileSetFileName;
                                    UTIResourceManager::LoadJsonFromFile(TileSetFilePath, TileSetData);
                                }

                                FString TileSetName;
                                TileSetData->TryGetStringField(TEXT("name"), TileSetName);

                                if (TileSetName == TileSet->Name)
                                {
                                    FirstGID = PossibleFirstGID;
                                    break;
                                }
                            }

                            if (FirstGID >= 0)
                            {
                                TileSets.Add(FirstGID, TileSet);
                            }
                        }
                    }

                    if (!TileSets.IsEmpty())
                    {
                        if (bShouldCreate)
                        {
                            TileMap = NewObject<UTITileMapResource>();
                            TileMap->ID = ResourceManager->GenerateResourceID();
                            TileMap->SourcePath = FilePath;
                            TileMap->SourcePath.ReplaceInline(*FPaths::ProjectDir(), TEXT("/"));
                            TileMap->Name = TileMapName;
                        }
                        else
                        {
                            // Remove the dependencies and re add them if reloading
                            TileMap->RemoveAllDependencies();
                        }
                        
                        TileMap->LoadJson();
                        for (const auto& TileSet : TileSets)
                        {
                            TileMap->AddTileSet(TileSet.Value.Get(), TileSet.Key);
                        }
                    }
                    else
                    {
                        FString ErrorTitle = TEXT("Failed to import Tiled resource");
                        FString ErrorMsg = FString::Printf(TEXT("The tile map file '%s' does not contain a valid tileset"), *FilePath);
                        FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
                    }
                }
            }
            else if (!bSilent)
            {
                FString ErrorTitle = TEXT("Failed to import Tiled resource");
                FString ErrorMsg = FString::Printf(TEXT("The file '%s' is not a valid tile map file"), *FilePath);
                FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
            }
        }
        else if (!bSilent)
        {
            FString ErrorTitle = TEXT("Failed to import Tiled resource");
            FString ErrorMsg = FString::Printf(TEXT("The file '%s' is not a valid json file"), *FilePath);
            FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
        }
    }
    else if (!bSilent)
    {
        FString ErrorTitle = TEXT("Failed to import Tiled resource");
        FString ErrorMsg = FString::Printf(TEXT("The file '%s' was not found"), *FilePath);
        FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
    }

    return TileMap;
}

UTIJsonResource::UTIJsonResource(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
    JsonData = MakeShared<FJsonObject>();
}

bool UTIJsonResource::IsValid(bool bCheckAsset, bool bCheckDependencyAssets) const
{
    if (Super::IsValid(bCheckAsset, bCheckDependencyAssets))
    {
        return IsJsonDataValid();
    }

    return false;
}

bool UTIJsonResource::IsJsonDataValid() const
{
    return JsonData.IsValid() && !JsonData->Values.IsEmpty();
}

bool UTIJsonResource::LoadJson()
{
    if (FPaths::FileExists(GetAbsoluteSourcePath()))
    {
        return UTIResourceManager::LoadJsonFromFile(GetAbsoluteSourcePath(), JsonData);
    }

    return false;
}

UTIResourceManager::UTIResourceManager(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, LastResourceIDGenerated(INDEX_NONE)
, bCheckResourceLifespan(false)
{
    
}

UTIResource* UTIResourceManager::ImportResource(const FString& SourceFilePath, const FString& TargetFilePath, bool bSilent)
{
    UTIResource* Resource = nullptr;

    // Check if the path is within the project directory
    if (SourceFilePath.StartsWith(FPaths::ProjectDir()))
    {
        // Check if the path is within the project directory
        if (TargetFilePath.StartsWith(FPaths::ProjectContentDir()))
        {
            // Check if the resource was already imported
            if (FindResource(SourceFilePath) == nullptr)
            {
                // Check if the file location does not conflict with other assets
                if (!FPaths::FileExists(TargetFilePath))
                {
                    ETIResourceType Type = GetResourceType(SourceFilePath);
                    if (Type == ETIResourceType::TileMap)
                    {
                        Resource = UTITileMapResource::LoadFromFile(SourceFilePath, this, bSilent);
                    }
                    else if (Type == ETIResourceType::TileSet)
                    {
                        TArray<UTITileSetResource*> TileSets = UTITileSetResource::LoadFromFile(SourceFilePath, this, bSilent);
                        Resource = !TileSets.IsEmpty() ? TileSets[0] : nullptr;
                    }
                    else if (!bSilent)
                    {
                        FString ErrorTitle = TEXT("Failed to import Tiled resource");
                        FString ErrorMsg = FString::Printf(TEXT("The file '%s' is not a valid tile map or tile set file"), *SourceFilePath);
                        FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
                    }
                }
                else if (!bSilent)
                {
                    FString ErrorTitle = TEXT("Failed to import resource");
                    FString ErrorMsg = FString::Printf(TEXT("The file '%s' already exists. Write an unused file name or select another location"), *TargetFilePath);
                    FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
                }
            }
            else if (!bSilent)
            {
                FString ErrorTitle = TEXT("Failed to import resource");
                FString ErrorMsg = FString::Printf(TEXT("The file '%s' was already imported. You should reimport it instead."), *SourceFilePath);
                FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
            }
        }
        else if (!bSilent)
        {
            FString ErrorTitle = TEXT("Failed to import resource");
            FString ErrorMsg = FString::Printf(TEXT("The destination file must be within the project content folder '%s'"), *FPaths::ProjectContentDir());
            FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
        }
    }
    else if (!bSilent)
    {
        FString ErrorTitle = TEXT("Failed to import resource");
        FString ErrorMsg = TEXT("The file to import must be within the project folder in order to allow the automated reimport system to work.");
        FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
    }

    if (Resource)
    {
        // Import the resource and it's dependencies into the editor
        if (Resource->ImportAsset(this, TargetFilePath) != nullptr)
        {
            Resource->bManuallyImported = true;
            OnResourceAdded(Resource);
            SaveState();
        }
        else
        {
            if (!bSilent)
            {
                FString ErrorTitle = TEXT("Failed to import resource");
                FString ErrorMsg = FString::Printf(TEXT("Failed to import the file '%s'."), *FPaths::GetCleanFilename(Resource->SourcePath));
                FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
            }

            OnResourceDeleted(Resource, false);
            Resource = nullptr;
        }

        bCheckResourceLifespan = true;
    }

    return Resource;
}

bool UTIResourceManager::ReimportResource(int32 ResourceID, bool bSilent)
{
    bool Success = false;
    if (UTIResource* Resource = GetResource(ResourceID))
    {
        Success = ReimportResource(Resource, bSilent);
    }
    else if (!bSilent)
    {
        FString ErrorTitle = TEXT("Tiled Integration: Failed to reimport resource");
        FString ErrorMsg = FString::Printf(TEXT("Failed to get the resource with ID %d. Check the resource status for more info."), ResourceID);
        FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
    }

    return Success;
}

bool UTIResourceManager::ReimportResource(UTIResource* Resource, bool bSilent)
{
    bool Success = false;
    if (Resource)
    {
        if (Resource->ReimportAsset(this))
        {
            SaveState();
            Success = true;
            OnResourceReimportedEvent.Broadcast(Resource);
        }
        else if (!bSilent)
        {
            FString ErrorTitle = TEXT("Tiled Integration: Failed to reimport resources");
            FString ErrorMsg = FString::Printf(TEXT("Failed to reimport the resource %s. Check the resource status for more info."), *Resource->SourcePath);
            FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
        }

        bCheckResourceLifespan = true;
    }

    return Success;
}

bool UTIResourceManager::DeleteResource(int32 ResourceID, bool bAskForConfirmation)
{
    bool Success = false;
    if (UTIResource* Resource = GetResource(ResourceID))
    {
        if (OnResourceDeleted(Resource, bAskForConfirmation))
        {
            SaveState();
            Success = true;
        }

        bCheckResourceLifespan = true;
    }

    return Success;
}

UTIResource* UTIResourceManager::GetResource(int32 ResourceID) const
{
    auto ResourceItr = Resources.Find(ResourceID);
    return ResourceItr ? *ResourceItr : nullptr;
}

UTIResource* UTIResourceManager::FindResource(const FString& SourcePath, ETIResourceType Type, FString Name) const
{
    UTIResource* ResourceFound = nullptr;
    for (const auto& Pair : Resources)
    {
        UTIResource* Resource = Pair.Value;
        if (Type == ETIResourceType::Invalid || Type == Resource->Type)
        {
            if (Name.IsEmpty() || Name == Resource->Name)
            {
                if (Resource->GetAbsoluteSourcePath() == SourcePath)
                {
                    ResourceFound = Resource;
                    break;
                }
            }
        }
    }

    return ResourceFound;
}

void UTIResourceManager::SetResourceAutoImport(int32 ResourceID, bool bAutoImport)
{
    if (UTIResource* Resource = GetResource(ResourceID))
    {
        Resource->bShouldAutoReimport = bAutoImport;

        // Set auto reimport for the dependencies as well
        TArray<UTIResource*> ResourceDependencies = Resource->GetDependencies();
        for (UTIResource* Dependency : ResourceDependencies)
        {
            Dependency->bShouldAutoReimport = bAutoImport;
        }
    }

    SaveState();
}

void UTIResourceManager::LoadState()
{
    const FString TISaveFilePath = FPaths::ProjectDir() / GetDefault<UTISettings>()->SaveFilePath;
    if (FPaths::FileExists(TISaveFilePath) && Resources.IsEmpty())
    {
        TArray<TPair<UTIResource*, TArray<int32>>> ResourcesToLoad;
        TSharedPtr<FJsonObject> State = MakeShareable(new FJsonObject);
        if (UTIResourceManager::LoadJsonFromFile(TISaveFilePath, State))
        {
            const TArray<TSharedPtr<FJsonValue>>* ResourceStates;
            if (State->TryGetArrayField(TEXT("resources"), ResourceStates))
            {
                for (TSharedPtr<FJsonValue> ResourceStateValue : *ResourceStates)
                {
                    TSharedPtr<FJsonObject> ResourceState = ResourceStateValue->AsObject();

                    bool bFailed = false;
                    int32 ResourceID;
                    if (!ResourceState->TryGetNumberField(TEXT("id"), ResourceID))
                    {
                        bFailed = true;
                    }

                    FString ResourceName;
                    if (!ResourceState->TryGetStringField(TEXT("name"), ResourceName))
                    {
                        bFailed = true;
                    }

                    FString ResourceTypeStr;
                    if (!ResourceState->TryGetStringField(TEXT("type"), ResourceTypeStr))
                    {
                        bFailed = true;
                    }

                    FString ResourceSourcePath;
                    if (!ResourceState->TryGetStringField(TEXT("source"), ResourceSourcePath))
                    {
                        bFailed = true;
                    }

                    FString ResourceAssetPath;
                    if (!ResourceState->TryGetStringField(TEXT("asset"), ResourceAssetPath))
                    {
                        bFailed = true;
                    }

                    bool bResourceShouldAutoReimport;
                    ResourceState->TryGetBoolField(TEXT("auto_reimport"), bResourceShouldAutoReimport);

                    bool bResourceEmbedded;
                    ResourceState->TryGetBoolField(TEXT("embedded"), bResourceEmbedded);

                    bool bResourceManuallyImported;
                    ResourceState->TryGetBoolField(TEXT("manually_imported"), bResourceManuallyImported);

                    TArray<int32> ResourceDependencies;
                    const TArray<TSharedPtr<FJsonValue>>* ResourceDependenciesObj;
                    if (ResourceState->TryGetArrayField(TEXT("dependencies"), ResourceDependenciesObj))
                    {
                        for (TSharedPtr<FJsonValue> ResourceDependencyValue : *ResourceDependenciesObj)
                        {
                            ResourceDependencies.Add(ResourceDependencyValue->AsNumber());
                        }
                    }

                    if (!bFailed)
                    {
                        UTIResource* Resource = nullptr;
                        ETIResourceType ResourceType = GetResourceTypeFromString(ResourceTypeStr);
                        if (ResourceType == ETIResourceType::TileMap)
                        {
                            Resource = NewObject<UTITileMapResource>();
                        }
                        else if (ResourceType == ETIResourceType::TileSet)
                        {
                            UTITileSetResource* TileSetResource = NewObject<UTITileSetResource>();
                            TileSetResource->bIsEmbedded = bResourceEmbedded;
                            Resource = TileSetResource;
                        }
                        else if (ResourceType == ETIResourceType::Texture)
                        {
                            Resource = NewObject<UTITextureResource>();
                        }
                        else if (ResourceType == ETIResourceType::Flipbook)
                        {
                            Resource = NewObject<UTIFlipbookResource>();
                        }
                        else if (ResourceType == ETIResourceType::Sprite)
                        {
                            Resource = NewObject<UTISpriteResource>();
                        }

                        if (Resource)
                        {
                            Resource->ID = ResourceID;
                            Resource->Name = ResourceName;
                            Resource->SourcePath = ResourceSourcePath;
                            Resource->AssetPath = ResourceAssetPath;
                            Resource->bShouldAutoReimport = bResourceShouldAutoReimport;
                            Resource->bManuallyImported = bResourceManuallyImported;
                            ResourcesToLoad.Add(TPair<UTIResource*, TArray<int32>>(Resource, ResourceDependencies));
                        }
                    }
                    else
                    {
                        FString ErrorTitle = TEXT("Tiled Integration: Failed to load resources");
                        FString ErrorMsg = FString::Printf(TEXT("Failed to parse saved data."));
                        FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
                        return;
                    }
                }
            }
        }

        // Resolve the resource dependencies
        for (const auto& Pair : ResourcesToLoad)
        {
            UTIResource* Resource = Pair.Key;
            const TArray<int32>& ResourceDependencies = Pair.Value;
            if (!ResourceDependencies.IsEmpty())
            {
                auto FindResourceByID = [&ResourcesToLoad](int32 ResourceID) -> UTIResource*
                {
                    UTIResource* Resource = nullptr;
                    for (const auto& DependencyPair : ResourcesToLoad)
                    {
                        if (DependencyPair.Key->ID == ResourceID)
                        {
                            Resource = DependencyPair.Key;
                            break;
                        }
                    }

                    return Resource;
                };

                // Tile maps need to be loaded differently
                if (Resource->Type == ETIResourceType::TileMap)
                {
                    UTITileMapResource* TileMapResource = Cast<UTITileMapResource>(Resource);
                    if (TileMapResource->LoadJson())
                    {
                        for (int32 ResourceID : ResourceDependencies)
                        {
                            if (UTIResource* Dependency = FindResourceByID(ResourceID))
                            {
                                if (Dependency->Type == ETIResourceType::TileSet)
                                {
                                    UTITileSetResource* TileSetResource = Cast<UTITileSetResource>(Dependency);

                                    int32 FirstGID = 0;
                                    bool bFound = false;
                                    const TArray<TSharedPtr<FJsonValue>>* TileSets;

                                    if (TileMapResource->GetJsonData()->TryGetArrayField(TEXT("tilesets"), TileSets) && TileSets && !TileSets->IsEmpty())
                                    {
                                        for (TSharedPtr<FJsonValue> TileSet : *TileSets)
                                        {
                                            auto TileSetData = TileSet->AsObject();
                                            if (TileSetData->TryGetNumberField(TEXT("firstgid"), FirstGID))
                                            {
                                                if (TileSetResource->bIsEmbedded)
                                                {
                                                    FString TileSetName;
                                                    if (TileSetData->TryGetStringField(TEXT("name"), TileSetName) && TileSetName == TileSetResource->Name)
                                                    {
                                                        bFound = true;
                                                        break;
                                                    }
                                                }
                                                else
                                                {
                                                    FString TileSetFileName;
                                                    if (TileSetData->TryGetStringField(TEXT("source"), TileSetFileName))
                                                    {
                                                        FString TileSetFilePath = FPaths::GetPath(TileMapResource->SourcePath) / TileSetFileName;
                                                        if (TileSetResource->SourcePath == TileSetFilePath)
                                                        {
                                                            bFound = true;
                                                            break;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    if (bFound)
                                    {
                                        TileMapResource->AddTileSet(TileSetResource, FirstGID);
                                    }
                                    else
                                    {
                                        FString ErrorTitle = TEXT("Tiled Integration: Failed to load resources");
                                        FString ErrorMsg = FString::Printf(TEXT("Failed to get Tile Set resource %s for the Tile Map %s. Check the resource status for more info."), *TileSetResource->SourcePath, *TileMapResource->SourcePath);
                                        FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
                                    }
                                }
                                else
                                {
                                    Resource->AddDependency(Dependency);
                                }
                            }
                            else
                            {
                                FString ErrorTitle = TEXT("Tiled Integration: Failed to load resources");
                                FString ErrorMsg = FString::Printf(TEXT("Failed to get the dependency with id %d for the resource %s. Check the resource status for more info."), ResourceID, *Resource->SourcePath);
                                FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
                            }
                        }
                    }
                }
                else
                {
                    for (int32 ResourceID : ResourceDependencies)
                    {
                        if (UTIResource* Dependency = FindResourceByID(ResourceID))
                        {
                            Resource->AddDependency(Dependency);
                        }
                        else
                        {
                            FString ErrorTitle = TEXT("Tiled Integration: Failed to load resources");
                            FString ErrorMsg = FString::Printf(TEXT("Failed to get the dependency with id %d for the resource %s. Check the resource status for more info."), ResourceID, *Resource->SourcePath);
                            FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
                        }
                    }
                }
            }
        }

        // Validate the resources
        for (const auto& Pair : ResourcesToLoad)
        {
            UTIResource* Resource = Pair.Key;

            // Load the Json data
            if (UTIJsonResource* JsonResource = Cast<UTIJsonResource>(Resource))
            {
                JsonResource->LoadJson();
            }

            if (!Resource->IsValid() || (Resource->ShouldAutoReimport() && !Resource->ReimportAsset(this)))
            {
                FString ErrorTitle = TEXT("Tiled Integration: Failed to load resources");
                FString ErrorMsg = FString::Printf(TEXT("Failed to load the resource %s. Check the resource status for more info."), *Resource->SourcePath);
                FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
            }

            OnResourceAdded(Resource);
        }
    }
    else if (!Resources.IsEmpty())
    {
        for (const auto& Pair : Resources)
        {
            OnResourceAddedEvent.Broadcast(Pair.Value);
        }
    }
}

void UTIResourceManager::SaveState()
{
    TSharedPtr<FJsonObject> State = MakeShareable(new FJsonObject);
    TArray<TSharedPtr<FJsonValue>> ResourceStates;
    for (const auto& Pair : Resources)
    {
        UTIResource* Resource = Pair.Value;
        if (Resource && Resource->IsValid())
        {
            TSharedPtr<FJsonObject> ResourceState = MakeShareable(new FJsonObject);
            ResourceState->SetNumberField(TEXT("id"), Resource->ID);
            ResourceState->SetStringField(TEXT("name"), Resource->Name);
            ResourceState->SetStringField(TEXT("type"), GetStringFromResourceType(Resource->Type));
            ResourceState->SetStringField(TEXT("source"), Resource->SourcePath);
            ResourceState->SetStringField(TEXT("asset"), Resource->AssetPath);
            ResourceState->SetBoolField(TEXT("auto_reimport"), Resource->bShouldAutoReimport);
            ResourceState->SetBoolField(TEXT("manually_imported"), Resource->bManuallyImported);

            if (Resource->Type == ETIResourceType::TileSet)
            {
                UTITileSetResource* TileSetDependency = Cast<UTITileSetResource>(Resource);
                ResourceState->SetBoolField(TEXT("embedded"), TileSetDependency->bIsEmbedded);
            }

            TArray<TSharedPtr<FJsonValue>> ResourceDependencies;
            for (UTIResource* Dependency : Resource->GetDependencies())
            {
                ResourceDependencies.Add(MakeShareable(new FJsonValueNumber(Dependency->ID)));
            }

            ResourceState->SetArrayField(TEXT("dependencies"), ResourceDependencies);

            ResourceStates.Add(MakeShareable(new FJsonValueObject(ResourceState)));
        }
    }

    State->SetArrayField(TEXT("resources"), ResourceStates);

    // Save to file
    FString StateStr;
    auto JsonWriter = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&StateStr, 0);
    if (FJsonSerializer::Serialize(State.ToSharedRef(), JsonWriter, true))
    {
        const FString TISaveFilePath = FPaths::ProjectDir() / GetDefault<UTISettings>()->SaveFilePath;
        FFileHelper::SaveStringToFile(StateStr, *TISaveFilePath);
    }
}

void UTIResourceManager::OnTick(float DeltaTime)
{
    CheckResourceLifespan();

    for (const auto& Pair : Resources)
    {
        UTIResource* Resource = Pair.Value;
        if (Resource)
        {
            if (Resource->ShouldAutoReimport())
            {
                if (!ReimportResource(Resource, true))
                {
                    SetResourceAutoImport(Resource->ID, false);

                    FString ErrorTitle = TEXT("Tiled Integration: Failed to reimport resources");
                    FString ErrorMsg = FString::Printf(TEXT("Failed to reimport the resource %s. Auto import has been disabled for this resource."), *Resource->SourcePath);
                    FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok, FText::FromString(ErrorMsg), FText::FromString(ErrorTitle));
                }
            }
        }
        else
        {
            bCheckResourceLifespan = true;
        }
    }
}

void UTIResourceManager::OnResourceAdded(UTIResource* Resource)
{
    if (!Resources.Contains(Resource->ID))
    {
        // Add the dependencies as well
        for (UTIResource* Dependency : Resource->GetDependencies())
        {
            OnResourceAdded(Dependency);
        }

        Resources.Add(Resource->ID, Resource);
        OnResourceAddedEvent.Broadcast(Resource);
    }
}

bool UTIResourceManager::OnResourceDeleted(UTIResource* Resource, bool bAskForConfirmation)
{
    if (Resource->DeleteAsset(bAskForConfirmation))
    {
        // Remove user references from dependencies
        for (UTIResource* Dependency : Resource->GetDependencies())
        {
            Dependency->RemoveUser(Resource);

            // If nobody is referencing the dependency, remove it too
            if (!Dependency->IsInUse())
            {
                OnResourceDeleted(Dependency, bAskForConfirmation);
            }
        }

        OnResourceRemovedEvent.Broadcast(Resource);
        Resources.Remove(Resource->ID);
        Resource->MarkAsGarbage();
        return true;
    }

    return false;
}

void UTIResourceManager::CheckResourceLifespan()
{
    if (bCheckResourceLifespan)
    {
        bCheckResourceLifespan = false;
        bool bChangeDetected = false;
        for (TObjectIterator<UTIResource> Resource; Resource; ++Resource)
        {
            if (Resource->IsA(UTIResource::StaticClass()) && Resource->IsValidLowLevel())
            {
                // Check if a registered resource is not in use anymore
                if (!Resource->IsInUse())
                {
                    if (OnResourceDeleted(*Resource, false))
                    {
                        bChangeDetected = true;
                    }
                    else
                    {
                        // There is a resource that can't be deleted yet (pending references)
                        bCheckResourceLifespan = true;
                    }
                }

                // Check if the resource is not registered
                if (!Resources.Contains(Resource->ID) && Resource->IsInUse())
                {
                    OnResourceAdded(*Resource);
                    bChangeDetected = true;
                }
            }
        }

        if (bChangeDetected)
        {
            SaveState();
        }

        TArray<uint32> InvalidResourceIDs;
        for (const auto& Pair : Resources)
        {
            if (!Pair.Value)
            {
                InvalidResourceIDs.Add(Pair.Key);
            }
        }

        for (uint32 InvalidResourceID : InvalidResourceIDs)
        {
            Resources.Remove(InvalidResourceID);
        }
    }
}

int UTIResourceManager::GenerateResourceID()
{
    int32 ResourceID = 0;
    for (int32 Index = 1; Index <= INT32_MAX; ++Index)
    {
        if (!Resources.Contains(Index) && Index > LastResourceIDGenerated)
        {
            LastResourceIDGenerated = Index;
            ResourceID = Index;
            break;
        }
    }

    return ResourceID;
}

ETIResourceType UTIResourceManager::GetResourceTypeFromString(const FString& StrType)
{
    if (StrType == TEXT("map"))
    {
        return ETIResourceType::TileMap;
    }
    else if (StrType == TEXT("tileset"))
    {
        return ETIResourceType::TileSet;
    }
    else if (StrType == TEXT("texture"))
    {
        return ETIResourceType::Texture;
    }
    else if (StrType == TEXT("sprite"))
    {
        return ETIResourceType::Sprite;
    }
    else if (StrType == TEXT("flipbook"))
    {
        return ETIResourceType::Flipbook;
    }
    else
    {
        return ETIResourceType::Invalid;
    }
}

FString UTIResourceManager::GetStringFromResourceType(ETIResourceType Type)
{
    switch (Type)
    {
        case ETIResourceType::TileMap: { return TEXT("map"); }
        case ETIResourceType::TileSet: { return TEXT("tileset"); }
        case ETIResourceType::Texture: { return TEXT("texture"); }
        case ETIResourceType::Sprite: { return TEXT("sprite"); }
        case ETIResourceType::Flipbook: { return TEXT("flipbook"); }
        default: { return TEXT(""); }
    }
}

ETIResourceType UTIResourceManager::GetResourceType(const FString& FilePath)
{
    ETIResourceType Type = ETIResourceType::Invalid;
    TSharedPtr<FJsonObject> FileData = MakeShareable(new FJsonObject);
    if (LoadJsonFromFile(FilePath, FileData))
    {
        FString FileType;
        if (FileData->TryGetStringField(TEXT("type"), FileType))
        {
            Type = GetResourceTypeFromString(FileType);
        }
    }

    return Type;
}

bool UTIResourceManager::LoadJsonFromFile(const FString& FilePath, TSharedPtr<FJsonObject>& OutJson)
{
    FString FileExtension = FPaths::GetExtension(FilePath).ToLower();
    if (FileExtension == TEXT("json") || FileExtension == TEXT("tmj") || FileExtension == TEXT("tsj"))
    {
        if (FPaths::FileExists(FilePath))
        {
            FString JsonString;
            if (FFileHelper::LoadFileToString(JsonString, *FilePath))
            {
                auto JsonReader = TJsonReaderFactory<>::Create(JsonString);
                return FJsonSerializer::Deserialize(JsonReader, OutJson);
            }
        }
    }

    return false;
}