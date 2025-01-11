// Copyright Flekz Games. All Rights Reserved.

#pragma once
#include "ActorFactories/ActorFactory.h"
#include "TITileMapActorFactory.generated.h"

class AActor;
struct FAssetData;

UCLASS()
class TILEDINTEGRATIONEDITOR_API UTITileMapActorFactory : public UActorFactory
{
    GENERATED_UCLASS_BODY()

    virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
    virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
};

#if UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_2
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#endif