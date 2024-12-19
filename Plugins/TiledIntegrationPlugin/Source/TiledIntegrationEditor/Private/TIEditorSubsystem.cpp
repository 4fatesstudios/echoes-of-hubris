// Copyright Flekz Games. All Rights Reserved.

#include "TIEditorSubsystem.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "DesktopPlatformModule.h"
#include "Dom/JsonObject.h"
#include "IContentBrowserSingleton.h"
#include "Misc/MessageDialog.h"
#include "TIImporterFactory.h"
#include "TIResource.h"
#include "TISettings.h"

void UTIEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LastImportedFileDirectory = FPaths::ProjectContentDir();
    LastImportedFileDestinationDirectory = LastImportedFileDirectory;
    TickInterval = 5.0f;
    TimeSinceLastTick = 0.0f;
    ResourceManager = NewObject<UTIResourceManager>(this);
}

void UTIEditorSubsystem::Deinitialize()
{
    ResourceManager->SaveState();
    Super::Deinitialize();
}

UWorld* UTIEditorSubsystem::GetWorld() const
{
    if ((GetFlags() & RF_ClassDefaultObject) == RF_ClassDefaultObject)
    {
        return nullptr;
    }
    else if (GEditor)
    {
        return GWorld;
    }
    else
    {
        return GetOuter()->GetWorld();
    }
}

void UTIEditorSubsystem::LoadResources()
{
    ResourceManager->LoadState();
}

bool UTIEditorSubsystem::ImportResource(int32& OutResourceID)
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (DesktopPlatform)
    {
        FString SourceFilePath, TargetFilePath;
        TArray<FString> ImportedFilePaths;
        bool bOpenedDialog = DesktopPlatform->OpenFileDialog(
            FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
            TEXT("Select a Tiled Tile Map or Tile Set to import"),
            LastImportedFileDirectory,
            TEXT(""),
            TEXT("Tile Map files|*.json;*.tmj;*.tsj"),
            EFileDialogFlags::None,
            ImportedFilePaths
        );

        if (bOpenedDialog && !ImportedFilePaths.IsEmpty() && !ImportedFilePaths[0].IsEmpty())
        {
            SourceFilePath = ImportedFilePaths[0];
            LastImportedFileDirectory = FPaths::GetPath(SourceFilePath);

            FString FileType = TEXT("");
            FString FileName = FPaths::GetCleanFilename(SourceFilePath);
            FString FileNameNoExtension = FPaths::GetBaseFilename(SourceFilePath);
            FString WindowTitle = TEXT("Select the destination folder");
            FString DefaultFileName = TEXT("BP_") + FileNameNoExtension;

            ETIResourceType ResourceType = UTIResourceManager::GetResourceType(SourceFilePath);
            if (ResourceType == ETIResourceType::TileMap)
            {
                FileType = TEXT("Tile Map");
            }
            else if (ResourceType == ETIResourceType::TileSet)
            {
                FileType = TEXT("Tile Set");
            }

            TArray<FString> DestinationFiles;
            bOpenedDialog = DesktopPlatform->SaveFileDialog(
                FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
                FString::Printf(TEXT("Select the destination of the %s file %s"), *FileType, *FileName),
                LastImportedFileDestinationDirectory,
                DefaultFileName,
                TEXT("Unreal Engine Asset|*.uasset"),
                EFileDialogFlags::None,
                DestinationFiles
            );

            if (bOpenedDialog && !DestinationFiles.IsEmpty() && !DestinationFiles[0].IsEmpty())
            {
                TargetFilePath = DestinationFiles[0];
                LastImportedFileDestinationDirectory = FPaths::GetPath(TargetFilePath);

                if (UTIResource* Resource = ResourceManager->ImportResource(SourceFilePath, TargetFilePath))
                {
                    OutResourceID = Resource->ID;
                    return true;
                }
            }
        }
    }

    return false;
}

bool UTIEditorSubsystem::ReimportResource(int32 ResourceID)
{
    return ResourceManager->ReimportResource(ResourceID);
}

void UTIEditorSubsystem::BrowseToResourceAsset(int32 ResourceID) const
{
    if (UTIResource* Resource = GetResource(ResourceID))
    {
        FAssetData AssetData(Resource->GetAsset());
        if (AssetData.IsValid())
        {
            FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
            ContentBrowserModule.Get().SyncBrowserToAssets({ AssetData });
        }
    }
}

void UTIEditorSubsystem::BrowseToResourceSource(int32 ResourceID) const
{
    if (UTIResource* Resource = GetResource(ResourceID))
    {
        FString ResourceSourcePath = Resource->GetAbsoluteSourcePath();
        if (!FPaths::FileExists(ResourceSourcePath))
        {
            ResourceSourcePath = FPaths::GetPath(ResourceSourcePath);
        }

        if (FPaths::FileExists(ResourceSourcePath) || FPaths::DirectoryExists(ResourceSourcePath))
        {
            FPlatformProcess::ExploreFolder(*ResourceSourcePath);
        }
    }
}

bool UTIEditorSubsystem::DeleteResource(int32 ResourceID, bool bAskForConfirmation)
{
    return ResourceManager->DeleteResource(ResourceID, bAskForConfirmation);
}

void UTIEditorSubsystem::Tick(float DeltaTime)
{
    if (LastTickFrame == GFrameCounter)
    {
        return;
    }

    TimeSinceLastTick += DeltaTime;

    if (TimeSinceLastTick >= TickInterval)
    {
        OnTick(TimeSinceLastTick);
        TimeSinceLastTick = 0.0f;
    }

    LastTickFrame = GFrameCounter;
}

void UTIEditorSubsystem::OnTick(float DeltaTime)
{
    if (ResourceManager)
    {
        ResourceManager->OnTick(DeltaTime);
    }
}

UTIResource* UTIEditorSubsystem::GetResource(int32 ResourceID) const
{
    return ResourceManager->GetResource(ResourceID);
}

void UTIEditorSubsystem::SetResourceAutoImport(int32 ResourceID, bool bAutoImport)
{
    ResourceManager->SetResourceAutoImport(ResourceID, bAutoImport);
}