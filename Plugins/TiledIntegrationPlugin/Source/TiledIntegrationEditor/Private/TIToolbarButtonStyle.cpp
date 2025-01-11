// Copyright Flekz Games. All Rights Reserved.

#include "TIToolbarButtonStyle.h"

#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "Slate/SlateGameResources.h"
#include "Styling/SlateStyleMacros.h"
#include "Styling/SlateStyleRegistry.h"
#include "TiledIntegration.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FTIToolbarButtonStyle::StyleInstance = nullptr;

void FTIToolbarButtonStyle::Initialize()
{
    if (!StyleInstance.IsValid())
    {
        StyleInstance = Create();
        FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
    }
}

void FTIToolbarButtonStyle::Shutdown()
{
    FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
    ensure(StyleInstance.IsUnique());
    StyleInstance.Reset();
}

FName FTIToolbarButtonStyle::GetStyleSetName()
{
    static FName StyleSetName(TEXT("TIToolbarButtonStyle"));
    return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef<FSlateStyleSet> FTIToolbarButtonStyle::Create()
{
    TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet("TIToolbarButtonStyle"));
    Style->SetContentRoot(IPluginManager::Get().FindPlugin("TiledIntegration")->GetBaseDir() / TEXT("Resources"));

    Style->Set("TiledIntegration.PluginAction", new IMAGE_BRUSH_SVG(TEXT("ButtonIcon"), Icon20x20));
    return Style;
}

void FTIToolbarButtonStyle::ReloadTextures()
{
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
    }
}

const ISlateStyle& FTIToolbarButtonStyle::Get()
{
    return *StyleInstance;
}