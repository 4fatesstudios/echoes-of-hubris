// Copyright Flekz Games. All Rights Reserved.

#pragma once
#include "Framework/Commands/Commands.h"

class TILEDINTEGRATIONEDITOR_API FTIToolbarButtonCommands : public TCommands<FTIToolbarButtonCommands>
{
public:
    FTIToolbarButtonCommands();

    void RegisterCommands() override;

public:
    TSharedPtr<FUICommandInfo> PluginAction;
};