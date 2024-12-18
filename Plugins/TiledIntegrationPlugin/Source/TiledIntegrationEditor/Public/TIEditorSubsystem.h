// Copyright Flekz Games. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Stats/Stats.h"
#include "Tickable.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "TIEditorSubsystem.generated.h"

class UTIResource;
class UTIResourceManager;

UCLASS(BlueprintType, meta = (ShowWorldContextPin))
class TILEDINTEGRATIONEDITOR_API UTIEditorSubsystem : public UEditorSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    void Initialize(FSubsystemCollectionBase& Collection) override;
    void Deinitialize() override;

    ETickableTickType GetTickableTickType() const override
    {
        return ETickableTickType::Always;
    }

    TStatId GetStatId() const override
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UEditorScriptsSystem, STATGROUP_Tickables);
    }

    bool IsTickableInEditor() const override
    {
        return true;
    }

    bool IsTickable() const override
    {
        return true;
    }

    UWorld* GetWorld() const override;

public:
    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    void LoadResources();

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    bool ImportResource(int32& OutResourceID);

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    bool ReimportResource(int32 ResourceID);

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    void BrowseToResourceAsset(int32 ResourceID) const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    void BrowseToResourceSource(int32 ResourceID) const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    bool DeleteResource(int32 ResourceID, bool bAskForConfirmation = false);

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    UTIResource* GetResource(int32 ResourceID) const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    void SetResourceAutoImport(int32 ResourceID, bool bAutoImport);

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    UTIResourceManager* GetResourceManager() const { return ResourceManager; }

private:
    void Tick(float DeltaTime) override;
    void OnTick(float DeltaTime);

private:
    UPROPERTY()
    UTIResourceManager* ResourceManager;

    FString LastImportedFileDirectory;
    FString LastImportedFileDestinationDirectory;
    uint32 LastTickFrame = INDEX_NONE;
    float TickInterval;
    float TimeSinceLastTick;
};