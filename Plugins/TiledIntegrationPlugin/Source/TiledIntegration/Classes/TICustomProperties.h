// Copyright Flekz Games. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Dom/JsonObject.h"
#include "JsonObjectWrapper.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TICustomProperties.generated.h"

class FJsonValue;

UCLASS(BlueprintType)
class TILEDINTEGRATION_API UTIClassCustomProperties : public UObject
{
    GENERATED_BODY()

public:
    void Empty();
    void LoadProperty(const FString& PropertyName, TSharedPtr<FJsonValue> PropertyValue);

    bool GetBoolProperty(const FString& PropertyName, bool& OutValue) const;
    bool GetNumberProperty(const FString& PropertyName, double& OutValue) const;
    bool GetNumberProperty(const FString& PropertyName, int8& OutValue) const;
    bool GetNumberProperty(const FString& PropertyName, uint8& OutValue) const;
    bool GetNumberProperty(const FString& PropertyName, int32& OutValue) const;
    bool GetNumberProperty(const FString& PropertyName, uint32& OutValue) const;
    bool GetNumberProperty(const FString& PropertyName, float& OutValue) const;
    bool GetStringProperty(const FString& PropertyName, FString& OutValue) const;
    bool GetFileProperty(const FString& PropertyName, FString& OutValue) const;
    bool GetColorProperty(const FString& PropertyName, FColor& OutValue) const;
    const UTIClassCustomProperties* GetClassProperty(const FString& PropertyName) const;

public:
    UPROPERTY()
    TMap<FString, bool> BoolProperties;

    UPROPERTY()
    TMap<FString, double> NumberProperties;

    UPROPERTY()
    TMap<FString, FString> StringProperties;

    UPROPERTY()
    TMap<FString, FString> FileProperties;

    UPROPERTY()
    TMap<FString, FColor> ColorProperties;

    UPROPERTY()
    TMap<FString, UTIClassCustomProperties*> ClassProperties;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew)
class TILEDINTEGRATION_API UTICustomProperties : public UObject
{
    GENERATED_BODY()

    friend class FTITileMapDetailsCustomization;
    friend class FTITileSetDetailsCustomization;

public:
    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    bool GetBoolProperty(const FString& PropertyName, bool& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    bool GetNumberProperty(const FString& PropertyName, double& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    bool GetStringProperty(const FString& PropertyName, FString& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    bool GetFileProperty(const FString& PropertyName, FString& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    bool GetColorProperty(const FString& PropertyName, FColor& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    const UTIClassCustomProperties* GetClassProperty(const FString& PropertyName) const;

public:
    void Empty();
    void Load(const TArray<TSharedPtr<FJsonValue>>& Data);

    bool GetNumberProperty(const FString& PropertyName, int8& OutValue) const;
    bool GetNumberProperty(const FString& PropertyName, uint8& OutValue) const;
    bool GetNumberProperty(const FString& PropertyName, int32& OutValue) const;
    bool GetNumberProperty(const FString& PropertyName, uint32& OutValue) const;
    bool GetNumberProperty(const FString& PropertyName, float& OutValue) const;

private:
    UPROPERTY()
    TMap<FString, bool> BoolProperties;

    UPROPERTY()
    TMap<FString, double> NumberProperties;

    UPROPERTY()
    TMap<FString, FString> StringProperties;

    UPROPERTY()
    TMap<FString, FString> FileProperties;

    UPROPERTY()
    TMap<FString, FColor> ColorProperties;

    UPROPERTY()
    TMap<FString, UTIClassCustomProperties*> ClassProperties;
};

UCLASS()
class TILEDINTEGRATION_API UTICustomPropertiesLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    static bool GetBoolProperty(const UTIClassCustomProperties* ClassProperties, const FString& PropertyName, bool& OutValue);

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    static bool GetNumberProperty(const UTIClassCustomProperties* ClassProperties, const FString& PropertyName, double& OutValue);

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    static bool GetStringProperty(const UTIClassCustomProperties* ClassProperties, const FString& PropertyName, FString& OutValue);

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    static bool GetFileProperty(const UTIClassCustomProperties* ClassProperties, const FString& PropertyName, FString& OutValue);

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    static bool GetColorProperty(const UTIClassCustomProperties* ClassProperties, const FString& PropertyName, FColor& OutValue);

    UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
    static const UTIClassCustomProperties* GetClassProperty(const UTIClassCustomProperties* ClassProperties, const FString& PropertyName);
};