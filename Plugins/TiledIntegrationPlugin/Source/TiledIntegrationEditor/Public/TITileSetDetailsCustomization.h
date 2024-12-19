// Copyright Flekz Games. All Rights Reserved.

#pragma once

#include "IDetailCustomization.h"

class IDetailGroup;
class IDetailLayoutBuilder;
class IDetailCategoryBuilder;
class UTIClassCustomProperties;
class UTICustomProperties;

class FTITileSetDetailsCustomization : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

    static void EnableEditProperties(bool bEnable);

private:
    bool GetSelectedTileIndex(IDetailCategoryBuilder& SingleTileCategory, int32& OutIndex) const;

    void AddCustomProperties(const UTICustomProperties* CustomProperties, IDetailGroup& CustomPropertiesGroup);
    void AddClassCustomProperty(const UTIClassCustomProperties* ClassCustomProperties, IDetailGroup& ClassCustomPropertiesGroup);
};