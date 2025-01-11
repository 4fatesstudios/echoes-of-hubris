// Copyright Flekz Games. All Rights Reserved.

#include "TITileMapDetailsCustomization.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "TICustomProperties.h"
#include "TITileMap.h"
#include "TITileLayer.h"
#include "Widgets/Colors/SColorBlock.h"

TSharedRef<IDetailCustomization> FTITileMapDetailsCustomization::MakeInstance()
{
    return MakeShareable(new FTITileMapDetailsCustomization());
}

void FTITileMapDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
    TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
    DetailLayout.GetObjectsBeingCustomized(ObjectsBeingCustomized);
    UTITileMap* TileMap = !ObjectsBeingCustomized.IsEmpty() ? Cast<UTITileMap>(ObjectsBeingCustomized[0]) : nullptr;

    // Add Custom Properties
    if (TileMap)
    {
        if (UTICustomProperties* TileMapCustomProperties = TileMap->GetCustomProperties())
        {
            IDetailCategoryBuilder& TileMapCategory = DetailLayout.EditCategory("Setup", FText::GetEmpty());
            IDetailGroup& CustomPropertiesGroup = TileMapCategory.AddGroup("Custom Properties", FText::FromString("Custom Properties"));
            AddCustomProperties(TileMapCustomProperties, CustomPropertiesGroup);
        }

        if (UTITileLayer* TileLayer = TileMap->GetLayer(TileMap->SelectedLayerIndex))
        {
            if (UTICustomProperties* TileLayerCustomProperties = TileMap->GetCustomProperties())
            {
                IDetailCategoryBuilder& TileLayerCategory = DetailLayout.EditCategory("SelectedLayer", FText::GetEmpty());
                IDetailGroup& CustomPropertiesGroup = TileLayerCategory.AddGroup("Custom Properties", FText::FromString("Custom Properties"));
                AddCustomProperties(TileLayerCustomProperties, CustomPropertiesGroup);
            }
        }
    }

    // Disable properties that come from tiled
    EnableEditProperties(false);
}

void FTITileMapDetailsCustomization::EnableEditProperties(bool bEnable)
{
    UClass* TileMapClass = UTITileMap::StaticClass();
    auto MapWidthProperty = TileMapClass->FindPropertyByName(TEXT("MapWidth"));
    auto MapHeightProperty = TileMapClass->FindPropertyByName(TEXT("MapHeight"));
    auto TileWidthProperty = TileMapClass->FindPropertyByName(TEXT("TileWidth"));
    auto TileHeightProperty = TileMapClass->FindPropertyByName(TEXT("TileHeight"));
    auto ProjectionModeProperty = TileMapClass->FindPropertyByName(TEXT("ProjectionMode"));

    if (bEnable)
    {
        MapWidthProperty->ClearPropertyFlags(CPF_EditConst);
        MapHeightProperty->ClearPropertyFlags(CPF_EditConst);
        TileWidthProperty->ClearPropertyFlags(CPF_EditConst);
        TileHeightProperty->ClearPropertyFlags(CPF_EditConst);
        ProjectionModeProperty->ClearPropertyFlags(CPF_EditConst);
    }
    else
    {
        MapWidthProperty->SetPropertyFlags(CPF_EditConst);
        MapHeightProperty->SetPropertyFlags(CPF_EditConst);
        TileWidthProperty->SetPropertyFlags(CPF_EditConst);
        TileHeightProperty->SetPropertyFlags(CPF_EditConst);
        ProjectionModeProperty->SetPropertyFlags(CPF_EditConst);
    }
}

void FTITileMapDetailsCustomization::AddCustomProperties(const UTICustomProperties* CustomProperties, IDetailGroup& CustomPropertiesGroup)
{
    if (CustomProperties)
    {
        for (const auto& PropertyPair : CustomProperties->BoolProperties)
        {
            const FString& PropertyName = PropertyPair.Key;
            const bool& PropertyValue = PropertyPair.Value;

            CustomPropertiesGroup.AddWidgetRow()
            .IsEnabled(false)
            .NameContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString(PropertyName))
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            .ValueContent()
            [
                SNew(SCheckBox)
                .IsChecked(PropertyValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
            ];
        }

        for (const auto& PropertyPair : CustomProperties->NumberProperties)
        {
            const FString& PropertyName = PropertyPair.Key;
            const double& PropertyValue = PropertyPair.Value;

            CustomPropertiesGroup.AddWidgetRow()
            .IsEnabled(false)
            .NameContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString(PropertyName))
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            .ValueContent()
            [
                SNew(SEditableTextBox)
                .Text(FText::AsNumber(PropertyValue))
            ];
        }

        for (const auto& PropertyPair : CustomProperties->StringProperties)
        {
            const FString& PropertyName = PropertyPair.Key;
            const FString& PropertyValue = PropertyPair.Value;

            CustomPropertiesGroup.AddWidgetRow()
            .IsEnabled(false)
            .NameContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString(PropertyName))
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            .ValueContent()
            [
                SNew(SEditableTextBox)
                .Text(FText::FromString(PropertyValue))
            ];
        }

        for (const auto& PropertyPair : CustomProperties->FileProperties)
        {
            const FString& PropertyName = PropertyPair.Key;
            const FString& PropertyValue = PropertyPair.Value;

            CustomPropertiesGroup.AddWidgetRow()
            .IsEnabled(false)
            .NameContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString(PropertyName))
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            .ValueContent()
            [
                SNew(SEditableTextBox)
                .Text(FText::FromString(PropertyValue))
            ];
        }

        for (const auto& PropertyPair : CustomProperties->ColorProperties)
        {
            const FString& PropertyName = PropertyPair.Key;
            const FColor& PropertyValue = PropertyPair.Value;

            IDetailGroup& ColorGroup = CustomPropertiesGroup.AddGroup(*PropertyName, FText::FromString(PropertyName));
            ColorGroup.HeaderRow()
            .IsEnabled(false)
            .NameContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString(PropertyName))
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            .ValueContent()
            [
                SNew(SBox)
                .Padding(FMargin(0, 0, 4.0f, 0.0f))
                .VAlign(VAlign_Center)
                [
                    SNew(SBorder)
                    .Padding(1)
                    .BorderImage(FAppStyle::Get().GetBrush("ColorPicker.RoundedSolidBackground"))
                    .BorderBackgroundColor(FAppStyle::Get().GetSlateColor("Colors.InputOutline"))
                    .VAlign(VAlign_Center)
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot()
                        .VAlign(VAlign_Center)
                        [
                            SNew(SColorBlock)
                            .AlphaBackgroundBrush(FAppStyle::Get().GetBrush("ColorPicker.RoundedAlphaBackground"))
                            .Color(PropertyValue.ReinterpretAsLinear())
                            .ShowBackgroundForAlpha(true)
                            .AlphaDisplayMode(EColorBlockAlphaDisplayMode::Separate)
                            .Size(FVector2D(70.0f, 20.0f))
                            .CornerRadius(FVector4(4.0f, 4.0f, 4.0f, 4.0f))
                        ]
                    ]
                ]
            ];

            ColorGroup.AddWidgetRow()
            .IsEnabled(false)
            .NameContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString("R"))
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            .ValueContent()
            [
                SNew(SEditableTextBox)
                .Text(FText::AsNumber(PropertyValue.R))
            ];

            ColorGroup.AddWidgetRow()
            .IsEnabled(false)
            .NameContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString("G"))
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            .ValueContent()
            [
                SNew(SEditableTextBox)
                .Text(FText::AsNumber(PropertyValue.G))
            ];

            ColorGroup.AddWidgetRow()
            .IsEnabled(false)
            .NameContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString("B"))
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            .ValueContent()
            [
                SNew(SEditableTextBox)
                .Text(FText::AsNumber(PropertyValue.B))
            ];

            ColorGroup.AddWidgetRow()
            .IsEnabled(false)
            .NameContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString("A"))
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            .ValueContent()
            [
                SNew(SEditableTextBox)
                .Text(FText::AsNumber(PropertyValue.A))
            ];
        }

        for (const auto& ClassPropertyPair : CustomProperties->ClassProperties)
        {
            const FString& ClassName = ClassPropertyPair.Key;
            const UTIClassCustomProperties* ClassProperties = ClassPropertyPair.Value;

            if (ClassProperties)
            {
                IDetailGroup& ClassGroup = CustomPropertiesGroup.AddGroup(*ClassName, FText::FromString(ClassName));
                ClassGroup.HeaderRow()
                .IsEnabled(false)
                .NameContent()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(ClassName))
                    .Font(IDetailLayoutBuilder::GetDetailFont())
                ]
                .ValueContent()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("Class"))
                    .Font(IDetailLayoutBuilder::GetDetailFont())
                ];

                AddClassCustomProperty(ClassProperties, ClassGroup);
            }
        }
    }
}

void FTITileMapDetailsCustomization::AddClassCustomProperty(const UTIClassCustomProperties* ClassProperties, IDetailGroup& ClassGroup)
{
    if (ClassProperties)
    {
        for (const auto& PropertyPair : ClassProperties->BoolProperties)
        {
            const FString& PropertyName = PropertyPair.Key;
            const bool& PropertyValue = PropertyPair.Value;

            ClassGroup.AddWidgetRow()
            .IsEnabled(false)
            .NameContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString(PropertyName))
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            .ValueContent()
            [
                SNew(SCheckBox)
                .IsChecked(PropertyValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
            ];
        }

        for (const auto& PropertyPair : ClassProperties->NumberProperties)
        {
            const FString& PropertyName = PropertyPair.Key;
            const double& PropertyValue = PropertyPair.Value;

            ClassGroup.AddWidgetRow()
            .IsEnabled(false)
            .NameContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString(PropertyName))
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            .ValueContent()
            [
                SNew(SEditableTextBox)
                .Text(FText::AsNumber(PropertyValue))
            ];
        }

        for (const auto& PropertyPair : ClassProperties->StringProperties)
        {
            const FString& PropertyName = PropertyPair.Key;
            const FString& PropertyValue = PropertyPair.Value;

            ClassGroup.AddWidgetRow()
            .IsEnabled(false)
            .NameContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString(PropertyName))
                    .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            .ValueContent()
            [
                SNew(SEditableTextBox)
                .Text(FText::FromString(PropertyValue))
            ];
        }

        for (const auto& PropertyPair : ClassProperties->FileProperties)
        {
            const FString& PropertyName = PropertyPair.Key;
            const FString& PropertyValue = PropertyPair.Value;

            ClassGroup.AddWidgetRow()
            .IsEnabled(false)
            .NameContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString(PropertyName))
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            .ValueContent()
            [
                SNew(SEditableTextBox)
                .Text(FText::FromString(PropertyValue))
            ];
        }

        for (const auto& PropertyPair : ClassProperties->ColorProperties)
        {
            const FString& PropertyName = PropertyPair.Key;
            const FColor& PropertyValue = PropertyPair.Value;

            IDetailGroup& ColorGroup = ClassGroup.AddGroup(*PropertyName, FText::FromString(PropertyName));
            ColorGroup.HeaderRow()
            .IsEnabled(false)
            .NameContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString(PropertyName))
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            .ValueContent()
            [
                SNew(SBox)
                .Padding(FMargin(0, 0, 4.0f, 0.0f))
                .VAlign(VAlign_Center)
                [
                    SNew(SBorder)
                    .Padding(1)
                    .BorderImage(FAppStyle::Get().GetBrush("ColorPicker.RoundedSolidBackground"))
                    .BorderBackgroundColor(FAppStyle::Get().GetSlateColor("Colors.InputOutline"))
                    .VAlign(VAlign_Center)
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot()
                        .VAlign(VAlign_Center)
                        [
                            SNew(SColorBlock)
                            .AlphaBackgroundBrush(FAppStyle::Get().GetBrush("ColorPicker.RoundedAlphaBackground"))
                            .Color(PropertyValue.ReinterpretAsLinear())
                            .ShowBackgroundForAlpha(true)
                            .AlphaDisplayMode(EColorBlockAlphaDisplayMode::Separate)
                            .Size(FVector2D(70.0f, 20.0f))
                            .CornerRadius(FVector4(4.0f, 4.0f, 4.0f, 4.0f))
                        ]
                    ]
                ]
            ];

            ColorGroup.AddWidgetRow()
            .IsEnabled(false)
            .NameContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString("R"))
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            .ValueContent()
            [
                SNew(SEditableTextBox)
                .Text(FText::AsNumber(PropertyValue.R))
            ];

            ColorGroup.AddWidgetRow()
            .IsEnabled(false)
            .NameContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString("G"))
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            .ValueContent()
            [
                SNew(SEditableTextBox)
                .Text(FText::AsNumber(PropertyValue.G))
            ];

            ColorGroup.AddWidgetRow()
            .IsEnabled(false)
            .NameContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString("B"))
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            .ValueContent()
            [
                SNew(SEditableTextBox)
                .Text(FText::AsNumber(PropertyValue.B))
            ];

            ColorGroup.AddWidgetRow()
            .IsEnabled(false)
            .NameContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString("A"))
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            .ValueContent()
            [
                SNew(SEditableTextBox)
                .Text(FText::AsNumber(PropertyValue.A))
            ];
        }

        for (const auto& PropertyPair : ClassProperties->ClassProperties)
        {
            const FString& ClassName = PropertyPair.Key;
            const UTIClassCustomProperties* SubclassProperties = PropertyPair.Value;

            if (SubclassProperties)
            {
                IDetailGroup& SubclassGroup = ClassGroup.AddGroup(*ClassName, FText::FromString(ClassName));
                SubclassGroup.HeaderRow()
                .IsEnabled(false)
                .NameContent()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(ClassName))
                    .Font(IDetailLayoutBuilder::GetDetailFont())
                ]
                .ValueContent()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("Class"))
                    .Font(IDetailLayoutBuilder::GetDetailFont())
                ];

                AddClassCustomProperty(SubclassProperties, SubclassGroup);
            }
        }
    }
}
