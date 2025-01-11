// Copyright Flekz Games. All Rights Reserved.

#include "TITileMapActor.h"

#include "TITileMapComponent.h"

ATITileMapActor::ATITileMapActor(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
    CreateTileMapComponent(UTITileMapComponent::StaticClass(), TEXT("TIRenderComponent"));
}

void ATITileMapActor::CreateTileMapComponent(TSubclassOf<UTITileMapComponent> TileMapComponentClass, FName TileMapComponentName)
{
    if (!RootComponent || !RootComponent->GetClass()->IsChildOf(TileMapComponentClass))
    {
        // Swap the default render component with our own
        FProperty* Property = FindFProperty<FProperty>(APaperTileMapActor::StaticClass(), TEXT("RenderComponent"));
        if (Property && Property->IsA<FObjectProperty>())
        {
            UPaperTileMapComponent** RenderComponentPtr = reinterpret_cast<UPaperTileMapComponent**>(Property->ContainerPtrToValuePtr<void>(this));
            if (RenderComponentPtr)
            {
                if (*RenderComponentPtr)
                {
                    (*RenderComponentPtr)->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
                    (*RenderComponentPtr)->DestroyComponent();
                    (*RenderComponentPtr)->MarkAsGarbage();
                    *RenderComponentPtr = nullptr;
                }

                *RenderComponentPtr = static_cast<UTITileMapComponent*>(CreateDefaultSubobject(TileMapComponentName, TileMapComponentClass, TileMapComponentClass, true, false));
                RootComponent = *RenderComponentPtr;
            }
        }
    }
}