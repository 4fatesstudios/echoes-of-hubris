// Copyright Flekz Games. All Rights Reserved.

#pragma once
#include "TIResource.generated.h"

class FJsonObject;
class UTIEditorSubsystem;
class UTIImporterFactory;
class UTIResource;
class UTIResourceManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTIResourceAdded, const UTIResource*, AddedResource);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTIResourceRemoved, const UTIResource*, RemovedResource);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTIResourceReimported, const UTIResource*, ReimportedResource);

UENUM()
enum class ETIResourceType : uint8
{
    Invalid,
    TileMap,
    TileSet,
    Texture,
    Sprite,
    Flipbook
};

UCLASS(BlueprintType)
class TILEDINTEGRATIONEDITOR_API UTIResource : public UObject
{
    GENERATED_UCLASS_BODY()

public:
    UObject* ImportAsset(UTIResourceManager* ResourceManager, const FString& TargetPath);
    bool ReimportAsset(UTIResourceManager* ResourceManager);
    virtual bool DeleteAsset(bool bAskForConfirmation);
    virtual bool SaveAsset();

    void AddUser(UTIResource* Resource);
    void RemoveUser(UTIResource* Resource);
    bool IsInUse(UTIResource* Resource = nullptr) const;

    virtual void AddDependency(UTIResource* Resource);
    virtual void RemoveDependency(UTIResource* Resource);
    void RemoveAllDependencies();
    bool HasDependency(UTIResource* Resource = nullptr) const;

    bool ShouldAutoReimport() const;
    bool IsSourceModified() const;

protected:
    virtual UObject* ImportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory) { return nullptr; }
    virtual bool ReimportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory) { return false; }

private:
    bool GetSourceFileModifiedTime(FDateTime& OutModifiedTime) const;

public:
    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    virtual bool IsValid(bool bCheckAsset = true, bool bCheckDependencyAssets = true) const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    bool IsAssetValid() const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    bool IsSourceValid() const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    bool AreDependenciesValid(bool bUsedBy = false, bool bCheckDependencyAssets = true) const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    UObject* GetAsset() const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    FString GetRelativeSourcePath() const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    FString GetRelativeAssetPath() const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    FString GetAbsoluteSourcePath() const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    FString GetAbsoluteAssetPath() const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    TArray<UTIResource*> GetDependencies(bool bUsedBy = false) const;

public:
    UPROPERTY(BlueprintReadOnly, Category = "TiledIntegration")
    int32 ID;

    UPROPERTY(BlueprintReadOnly, Category = "TiledIntegration")
    FString Name;

    UPROPERTY(BlueprintReadOnly, Category = "TiledIntegration")
    ETIResourceType Type;

    UPROPERTY(BlueprintReadOnly, Category = "TiledIntegration")
    FString SourcePath;

    UPROPERTY(BlueprintReadOnly, Category = "TiledIntegration")
    FString AssetPath;

    UPROPERTY(BlueprintReadOnly, Category = "TiledIntegration")
    bool bShouldAutoReimport;

    UPROPERTY(BlueprintReadOnly, Category = "TiledIntegration")
    bool bManuallyImported;

protected:
    UPROPERTY()
    TMap<int32, UTIResource*> Dependencies;

    TMap<int32, TWeakObjectPtr<UTIResource>> UsedBy;

    FDateTime LastImportTime;

    bool bCanAutoReimport;
};

UCLASS()
class TILEDINTEGRATIONEDITOR_API UTIJsonResource : public UTIResource
{
    GENERATED_UCLASS_BODY()

public:
    virtual bool IsValid(bool bCheckAsset = true, bool bCheckDependencyAssets = true) const override;
    bool IsJsonDataValid() const;
    virtual bool LoadJson();
    const TSharedPtr<FJsonObject>& GetJsonData() const { return JsonData; }

protected:
    TSharedPtr<FJsonObject> JsonData;
};

UCLASS()
class TILEDINTEGRATIONEDITOR_API UTITextureResource : public UTIResource
{
    GENERATED_UCLASS_BODY()

public:
    static UTITextureResource* LoadFromFile(const FString& FilePath, UTIResourceManager* ResourceManager, bool bSilent = false, bool bReloadDependencies = false, UTITextureResource* Resource = nullptr);

private:
    UObject* ImportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory) override;
    bool ReimportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory) override;
};

UCLASS()
class TILEDINTEGRATIONEDITOR_API UTISpriteResource : public UTIResource
{
    GENERATED_UCLASS_BODY()

public:
    static TArray<UTISpriteResource*> LoadFromFlipbook(UTIFlipbookResource* Flipbook, UTIResourceManager* ResourceManager, bool bSilent = false, bool bReloadDependencies = false, UTISpriteResource* Resource = nullptr);

    UTITileSetResource* GetTileSet() const;
    UTITextureResource* GetTexture() const;
    UTIFlipbookResource* GetFlipbook() const;
    int32 GetTileID() const;

private:
    UObject* ImportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory) override;
    bool ReimportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory) override;
};

UCLASS()
class TILEDINTEGRATIONEDITOR_API UTIFlipbookResource : public UTIJsonResource
{
    GENERATED_UCLASS_BODY()

public:
    static TArray<UTIFlipbookResource*> LoadFromTileSet(UTITileSetResource* TileSet, UTIResourceManager* ResourceManager, bool bSilent = false, bool bReloadDependencies = false, UTIFlipbookResource* Resource = nullptr);

    bool LoadJson() override;
    UTITileSetResource* GetTileSet() const;
    TArray<UTISpriteResource*> GetSprites() const;
    int32 GetTileID() const;

private:
    UObject* ImportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory) override;
    bool ReimportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory) override;
};

UCLASS()
class TILEDINTEGRATIONEDITOR_API UTITileSetResource : public UTIJsonResource
{
    GENERATED_UCLASS_BODY()

public:
    static TArray<UTITileSetResource*> LoadFromFile(const FString& FilePath, UTIResourceManager* ResourceManager, bool bSilent = false, bool bReloadDependencies = false, UTITileSetResource* Resource = nullptr);

    bool LoadJson() override;
    UTITextureResource* GetTexture() const;
    TArray<UTIFlipbookResource*> GetFlipbooks() const;
    TArray<UTITileMapResource*> GetTileMaps() const;

private:
    UObject* ImportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory) override;
    bool ReimportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory) override;

public:
    bool bIsEmbedded;
};

UCLASS()
class TILEDINTEGRATIONEDITOR_API UTITileMapResource : public UTIJsonResource
{
    GENERATED_UCLASS_BODY()

public:
    void AddTileSet(UTITileSetResource* TileSet, int32 FirstGID);
    void RemoveTileSet(UTITileSetResource* TileSet);

    const TMap<int32, TWeakObjectPtr<UTITileSetResource>>& GetTileSets() const { return TileSets; }
    
    static UTITileMapResource* LoadFromFile(const FString& FilePath, UTIResourceManager* ResourceManager, bool bSilent = false, bool bReloadDependencies = false, UTITileMapResource* Resource = nullptr);

private:
    void RemoveDependency(UTIResource* Resource) override;

    UObject* ImportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory) override;
    bool ReimportAssetInternal(UTIResourceManager* ResourceManager, UObject* Package, UTIImporterFactory* Factory) override;

private:
    TMap<int32, TWeakObjectPtr<UTITileSetResource>> TileSets;
};

UCLASS()
class TILEDINTEGRATIONEDITOR_API UTIResourceManager : public UObject
{
    GENERATED_UCLASS_BODY()

    friend class UTIResource;

public:
    UTIResource* ImportResource(const FString& SourcePath, const FString& TargetPath, bool bSilent = false);
    bool ReimportResource(int32 ResourceID, bool bSilent = false);
    bool ReimportResource(UTIResource* Resource, bool bSilent = false);
    bool DeleteResource(int32 ResourceID, bool bAskForConfirmation = false);
    UTIResource* GetResource(int32 ResourceID) const;
    UTIResource* FindResource(const FString& SourcePath, ETIResourceType Type = ETIResourceType::Invalid, FString Name = FString("")) const;
    void SetResourceAutoImport(int32 ResourceID, bool bAutoImport);
    int GenerateResourceID();

    void LoadState();
    void SaveState();

    void OnTick(float DeltaTime);

    static ETIResourceType GetResourceType(const FString& FilePath);
    static bool LoadJsonFromFile(const FString& FilePath, TSharedPtr<FJsonObject>& OutJson);

private:
    void OnResourceAdded(UTIResource* Resource);
    bool OnResourceDeleted(UTIResource* Resource, bool bAskForConfirmation);
    
    void CheckResourceLifespan();

    static ETIResourceType GetResourceTypeFromString(const FString& StrType);
    static FString GetStringFromResourceType(ETIResourceType Type);

public:
    UPROPERTY(BlueprintAssignable, Category = "TiledIntegration")
    FTIResourceAdded OnResourceAddedEvent;

    UPROPERTY(BlueprintAssignable, Category = "TiledIntegration")
    FTIResourceRemoved OnResourceRemovedEvent;

    UPROPERTY(BlueprintAssignable, Category = "TiledIntegration")
    FTIResourceReimported OnResourceReimportedEvent;

private:
    UPROPERTY()
    TMap<uint32, UTIResource*> Resources;

    int32 LastResourceIDGenerated;

    bool bCheckResourceLifespan;
};