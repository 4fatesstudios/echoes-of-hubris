// Copyright Flekz Games. All Rights Reserved.

#pragma once

#include "IDetailCustomization.h"

class IDetailGroup;
class IDetailLayoutBuilder;
class UTIClassCustomProperties;
class UTICustomProperties;

class FTITileMapDetailsCustomization : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

    static void EnableEditProperties(bool bEnable);

private:
    void AddCustomProperties(const UTICustomProperties* CustomProperties, IDetailGroup& CustomPropertiesGroup);
    void AddClassCustomProperty(const UTIClassCustomProperties* ClassCustomProperties, IDetailGroup& ClassCustomPropertiesGroup);
};