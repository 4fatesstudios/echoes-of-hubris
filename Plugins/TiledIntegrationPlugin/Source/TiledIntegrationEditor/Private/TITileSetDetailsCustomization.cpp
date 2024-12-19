// Copyright Flekz Games. All Rights Reserved.

#include "TITileSetDetailsCustomization.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "TICustomProperties.h"
#include "TITile.h"
#include "TITileSet.h"
#include "Widgets/Colors/SColorBlock.h"

TSharedRef<IDetailCustomization> FTITileSetDetailsCustomization::MakeInstance()
{
    return MakeShareable(new FTITileSetDetailsCustomization());
}

void FTITileSetDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
    TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
    DetailLayout.GetObjectsBeingCustomized(ObjectsBeingCustomized);
    UTITileSet* TileSet = !ObjectsBeingCustomized.IsEmpty() ? Cast<UTITileSet>(ObjectsBeingCustomized[0]) : nullptr;

    if (TileSet)
    {
        // Tile Set Custom Properties
        if (UTICustomProperties* TileSetCustomProperties = TileSet->GetCustomProperties())
        {
            IDetailCategoryBuilder& TileSetCategory = DetailLayout.EditCategory("TileSet", FText::GetEmpty());
            IDetailGroup& CustomPropertiesGroup = TileSetCategory.AddGroup("Custom Properties", FText::FromString("Custom Properties"));
            AddCustomProperties(TileSetCustomProperties, CustomPropertiesGroup);
        }

        // Selected Tile Custom Properties
        {
            // Hide the custom properties by default
            auto TilesProperty = DetailLayout.GetProperty(TEXT("Tiles"));
            DetailLayout.HideProperty(TilesProperty);
            TilesProperty->SetIgnoreValidation(true);

            // Get the selected tile index
            IDetailCategoryBuilder& SingleTileCategory = DetailLayout.EditCategory("SingleTileEditor", FText::GetEmpty());

            int32 SelectedTileIndex = INDEX_NONE;
            if (GetSelectedTileIndex(SingleTileCategory, SelectedTileIndex))
            {
                if (UTITile* Tile = TileSet->GetTile(SelectedTileIndex))
                {
                    //auto TileProperty = TilesProperty->GetChildHandle(SelectedTileIndex);
                    if (UTICustomProperties* TileCustomProperties = Tile->GetCustomProperties())
                    {
                        IDetailGroup& CustomPropertiesGroup = SingleTileCategory.AddGroup("Custom Properties", FText::FromString("Custom Properties"));
                        AddCustomProperties(TileCustomProperties, CustomPropertiesGroup);
                    }
                }
            }
            else
            {
                DetailLayout.HideCategory("SingleTileEditor");
            }
        }
    }

    // Disable properties that come from tiled
    EnableEditProperties(false);
}

void FTITileSetDetailsCustomization::EnableEditProperties(bool bEnable)
{
    UClass* TileSetClass = UTITileSet::StaticClass();
    auto TileSizeProperty = TileSetClass->FindPropertyByName(TEXT("TileSize"));
    auto TileSheetProperty = TileSetClass->FindPropertyByName(TEXT("TileSheet"));
    auto BorderMarginProperty = TileSetClass->FindPropertyByName(TEXT("BorderMargin"));
    auto PerTileSpacingProperty = TileSetClass->FindPropertyByName(TEXT("PerTileSpacing"));
    auto DrawingOffset = TileSetClass->FindPropertyByName(TEXT("DrawingOffset"));

    if (bEnable)
    {
        TileSizeProperty->ClearPropertyFlags(CPF_EditConst);
        TileSheetProperty->ClearPropertyFlags(CPF_EditConst);
        BorderMarginProperty->ClearPropertyFlags(CPF_EditConst);
        PerTileSpacingProperty->ClearPropertyFlags(CPF_EditConst);
        DrawingOffset->ClearPropertyFlags(CPF_EditConst);
    }
    else
    {
        TileSizeProperty->SetPropertyFlags(CPF_EditConst);
        TileSheetProperty->SetPropertyFlags(CPF_EditConst);
        BorderMarginProperty->SetPropertyFlags(CPF_EditConst);
        PerTileSpacingProperty->SetPropertyFlags(CPF_EditConst);
        DrawingOffset->SetPropertyFlags(CPF_EditConst);
    }
}

bool FTITileSetDetailsCustomization::GetSelectedTileIndex(IDetailCategoryBuilder& SingleTileCategory, int32& OutIndex) const
{
    OutIndex = INDEX_NONE;
    TArray<TSharedRef<IPropertyHandle>> OutDefaultProperties;
    SingleTileCategory.GetDefaultProperties(OutDefaultProperties);
    if (!OutDefaultProperties.IsEmpty())
    {
        FStringView SelectedTilePropertyPath = OutDefaultProperties[0]->GetPropertyPath();

        int32 StartIndex = INDEX_NONE;
        int32 EndIndex = INDEX_NONE;

        for (int32 i = 0; i < SelectedTilePropertyPath.Len(); ++i)
        {
            if (SelectedTilePropertyPath[i] == '[')
            {
                StartIndex = i + 1;
            }
            else if (SelectedTilePropertyPath[i] == ']')
            {
                EndIndex = i;
                break;
            }
        }

        if (StartIndex != INDEX_NONE && EndIndex != INDEX_NONE && StartIndex < EndIndex)
        {
            FString NumberString = FString(SelectedTilePropertyPath.Mid(StartIndex, EndIndex - StartIndex));

            if (NumberString.IsNumeric())
            {
                OutIndex = FCString::Atoi(*NumberString);
            }
        }
    }

    return OutIndex != INDEX_NONE;
}

void FTITileSetDetailsCustomization::AddCustomProperties(const UTICustomProperties* CustomProperties, IDetailGroup& CustomPropertiesGroup)
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

void FTITileSetDetailsCustomization::AddClassCustomProperty(const UTIClassCustomProperties* ClassProperties, IDetailGroup& ClassGroup)
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
