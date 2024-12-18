// Copyright Flekz Games. All Rights Reserved.

#include "TIToolbarButtonCommands.h"

#include "TIToolbarButtonStyle.h"

#define LOCTEXT_NAMESPACE "FTIEditorModule"

FTIToolbarButtonCommands::FTIToolbarButtonCommands()
: TCommands<FTIToolbarButtonCommands>(TEXT("TiledIntegration"), NSLOCTEXT("Contexts", "TiledIntegration", "TiledIntegration Plugin"), NAME_None, FTIToolbarButtonStyle::GetStyleSetName())
{

}

void FTIToolbarButtonCommands::RegisterCommands()
{
    UI_COMMAND(PluginAction, "TiledIntegration", "Open Tiled Integration Window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE