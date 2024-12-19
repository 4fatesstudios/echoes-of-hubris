// Copyright Flekz Games. All Rights Reserved.

#pragma once
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;
class UTITileMapActorFactory;

class FTIEditorModule : public IModuleInterface
{
public:
	void StartupModule() override;
	void ShutdownModule() override;

	void OnToolbarButtonClicked();

private:
	void RegisterMenus();
	void OnCloseAssetEditor(UObject* Asset, IAssetEditorInstance* AssetEditor);

private:
    TSharedPtr<class FUICommandList> PluginCommands;
	FName TabID;

	TStrongObjectPtr<UTITileMapActorFactory> TileMapActorFactory;
	FDelegateHandle OnAssetClosedHandle;
};
