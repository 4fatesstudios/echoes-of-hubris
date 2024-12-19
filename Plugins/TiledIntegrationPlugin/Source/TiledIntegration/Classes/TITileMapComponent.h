// Copyright Flekz Games. All Rights Reserved.

#pragma once
#include "PaperTileMapComponent.h"
#include "TITileMapComponent.generated.h"

class APaperFlipbookActor;
class UTITileInstance;
class UTITileMap;
class UTITileLayer;

struct FDynamicMeshVertex;
struct FSpriteRenderSection;

UCLASS(hideCategories = Object, ClassGroup = TiledIntegration, meta = (BlueprintSpawnableComponent))
class TILEDINTEGRATION_API UTITileMapComponent : public UPaperTileMapComponent
{
	GENERATED_UCLASS_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
	UTITileMap* GetTileMap() const;

	UFUNCTION(BlueprintCallable, Category = "TiledIntegration")
	UTITileInstance* GetTileInstance(const FIntVector& Coordinates) const;

private:
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual void SendRenderDynamicData_Concurrent() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick, FActorComponentTickFunction*) override;

	void CacheAnimatedTiles();

private:
	TArray<TWeakObjectPtr<UTITileInstance>> AnimatedTiles;
};
