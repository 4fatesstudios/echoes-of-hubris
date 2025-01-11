// Copyright Flekz Games. All Rights Reserved.

#include "TITileMapActorFactory.h"

#include "AssetRegistry/AssetData.h"
#include "PaperImporterSettings.h"
#include "PaperTileMapComponent.h"
#include "TISettings.h"
#include "TITileMapActor.h"
#include "TITileMap.h"
#include "TITileSet.h"

UTITileMapActorFactory::UTITileMapActorFactory(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
    DisplayName = NSLOCTEXT("TiledIntegration", "TileMapFactoryDisplayName", "TI Tile Map");
    NewActorClass = GetDefault<UTISettings>()->TileMapActorClass;
}

void UTITileMapActorFactory::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
    Super::PostSpawnActor(Asset, NewActor);

    ATITileMapActor* TypedActor = CastChecked<ATITileMapActor>(NewActor);
    UPaperTileMapComponent* RenderComponent = TypedActor->GetRenderComponent();
    check(RenderComponent);

    if (UTITileMap* TileMapAsset = Cast<UTITileMap>(Asset))
    {
        RenderComponent->UnregisterComponent();
        RenderComponent->TileMap = TileMapAsset;
        RenderComponent->RegisterComponent();
    }
    else if (RenderComponent->OwnsTileMap())
    {
        RenderComponent->UnregisterComponent();

        UTITileMap* OwnedTileMap = Cast<UTITileMap>(RenderComponent->TileMap);
        check(OwnedTileMap);

        GetDefault<UPaperImporterSettings>()->ApplySettingsForTileMapInit(OwnedTileMap, Cast<UTITileSet>(Asset));

        RenderComponent->RegisterComponent();
    }
}

bool UTITileMapActorFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
    if (AssetData.IsValid())
    {
        UClass* AssetClass = AssetData.GetClass();
        if ((AssetClass != nullptr) && (AssetClass->IsChildOf(UTITileMap::StaticClass()) || AssetClass->IsChildOf(UTITileSet::StaticClass())))
        {
            return true;
        }
    }

    return false;
}