// Copyright Flekz Games. All Rights Reserved.

#include "TiledIntegrationEditor.h"

#include "EditorAssetLibrary.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Subsystems/PlacementSubsystem.h"
#include "TITileMap.h"
#include "TITileMapActorFactory.h"
#include "TITileSet.h"
#include "TIToolbarButtonCommands.h"
#include "TIToolbarButtonStyle.h"
#include "TITileMapDetailsCustomization.h"
#include "TITileSetDetailsCustomization.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FTIEditorModule"

FName TITabName("/TiledIntegration/WBP_TiledIntegration.WBP_TiledIntegration_ActiveTab");
FString TIWidget(TEXT("/Script/Blutility.EditorUtilityWidgetBlueprint'/TiledIntegration/WBP_TiledIntegration.WBP_TiledIntegration'"));

void FTIEditorModule::StartupModule()
{
    // Initialize Plugin menus, styles and commands
    {
        FTIToolbarButtonStyle::Initialize();
        FTIToolbarButtonStyle::ReloadTextures();

	    FTIToolbarButtonCommands::Register();

        PluginCommands = MakeShareable(new FUICommandList);

        PluginCommands->MapAction(
            FTIToolbarButtonCommands::Get().PluginAction,
            FExecuteAction::CreateRaw(this, &FTIEditorModule::OnToolbarButtonClicked),
            FCanExecuteAction());

        UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTIEditorModule::RegisterMenus));
    }

    // Register Custom Details for Tile Sets and Tile Maps
    {
        FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
        PropertyModule.RegisterCustomClassLayout(UTITileSet::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FTITileSetDetailsCustomization::MakeInstance));
        PropertyModule.RegisterCustomClassLayout(UTITileMap::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FTITileMapDetailsCustomization::MakeInstance));
    }
    
    // Register Tile Map Actor Factory
    if (GEditor)
    {
        TileMapActorFactory = TStrongObjectPtr<UTITileMapActorFactory>(NewObject<UTITileMapActorFactory>());

        GEditor->ActorFactories.Add(TileMapActorFactory.Get());
        if (UPlacementSubsystem* PlacementSubsystem = GEditor->GetEditorSubsystem<UPlacementSubsystem>())
        {
            // Reorder factories so ours comes before the default one
            UClass* DefaultTileMapActorFactoryClass = FindObject<UClass>(FTopLevelAssetPath("/Script/Paper2DEditor", "TileMapActorFactory"));
            if (auto DefaultTileMapActorFactory = PlacementSubsystem->GetAssetFactoryFromFactoryClass(DefaultTileMapActorFactoryClass))
            {
                PlacementSubsystem->UnregisterAssetFactory(DefaultTileMapActorFactory);
                PlacementSubsystem->RegisterAssetFactory(TileMapActorFactory.Get());
                PlacementSubsystem->RegisterAssetFactory(DefaultTileMapActorFactory);
            }
        }

        OnAssetClosedHandle = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OnAssetClosedInEditor().AddRaw(this, &FTIEditorModule::OnCloseAssetEditor);
    }
}

void FTIEditorModule::ShutdownModule()
{
    // Shutdown plugin menus, styles and commands
    {
        UToolMenus::UnRegisterStartupCallback(this);

        UToolMenus::UnregisterOwner(this);

        FTIToolbarButtonStyle::Shutdown();

        FTIToolbarButtonCommands::Unregister();
    }

    // Unregister Custom Details for Tile Sets and Tile Maps
    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
        PropertyEditorModule.UnregisterCustomClassLayout(UTITileSet::StaticClass()->GetFName());
        PropertyEditorModule.UnregisterCustomClassLayout(UTITileMap::StaticClass()->GetFName());
    }

    // Unregister Tile Map Actor Factory
    if (GEditor)
    {
        GEditor->ActorFactories.RemoveAll([](const UActorFactory* ActorFactory) { return ActorFactory->IsA<UTITileMapActorFactory>(); });
        if (UPlacementSubsystem* PlacementSubsystem = GEditor->GetEditorSubsystem<UPlacementSubsystem>())
        {
            PlacementSubsystem->UnregisterAssetFactory(TileMapActorFactory.Get());
        }

        GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OnAssetClosedInEditor().Remove(OnAssetClosedHandle);
    }
}

void FTIEditorModule::OnToolbarButtonClicked()
{
    UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
    if (!EditorUtilitySubsystem->DoesTabExist(TITabName))
    {
        UObject* Blueprint = UEditorAssetLibrary::LoadAsset(TIWidget);
        if (IsValid(Blueprint))
        {
            UEditorUtilityWidgetBlueprint* EditorWidget = Cast<UEditorUtilityWidgetBlueprint>(Blueprint);
            if (IsValid(EditorWidget))
            {
                EditorUtilitySubsystem->SpawnAndRegisterTab(EditorWidget);
            }
        }
    }
}

void FTIEditorModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);
    {
        UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
        {
            FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
            Section.AddMenuEntryWithCommandList(FTIToolbarButtonCommands::Get().PluginAction, PluginCommands);
        }
    }

    {
        UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
        {
            FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
            {
                FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FTIToolbarButtonCommands::Get().PluginAction));
                Entry.SetCommandList(PluginCommands);
            }
        }
    }
}

void FTIEditorModule::OnCloseAssetEditor(UObject* Asset, IAssetEditorInstance* AssetEditor)
{
    if (Asset->IsA<UTITileMap>())
    {
        FTITileMapDetailsCustomization::EnableEditProperties(true);
    }
    else if (Asset->IsA<UTITileSet>())
    {
        FTITileSetDetailsCustomization::EnableEditProperties(true);
    }
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTIEditorModule, TiledIntegrationEditor)