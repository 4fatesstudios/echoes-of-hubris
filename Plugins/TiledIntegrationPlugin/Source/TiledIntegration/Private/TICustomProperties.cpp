// Copyright Flekz Games. All Rights Reserved.

#include "TICustomProperties.h"

#include "Misc/Paths.h"

bool GetColorFromHex(const FString& Hex, FColor& OutColor)
{
    if (Hex.Len() >= 7 && Hex[0] == '#')
    {
        FString HexString = Hex;

        // Convert the hex string from ARGB to RGBA
        if (Hex.Len() == 9)
        {
            FString ColorWithoutHash = Hex.Mid(1);
            FString FirstTwoChars = ColorWithoutHash.Left(2);
            FString RemainingPart = ColorWithoutHash.Mid(2);
            FString Result = RemainingPart + FirstTwoChars;
            HexString = "#" + Result;
        }

        OutColor = FColor::FromHex(HexString);
        return true;
    }

    return false;
}

template<typename T>
const bool ParseClassPropertyFormat(const FString& PropertyName, const T* CustomProperties, FString& OutProperty, const UTIClassCustomProperties** OutClass)
{
    const UTIClassCustomProperties* Result = nullptr;
    if (CustomProperties && PropertyName.Contains("."))
    {
        TArray<FString> PropertyNames;
        PropertyName.ParseIntoArray(PropertyNames, TEXT("."));

        if (PropertyNames.Num() >= 2)
        {
            const UTIClassCustomProperties* PrevClass = CustomProperties->GetClassProperty(PropertyNames[0]);
            if (PrevClass)
            {
                const UTIClassCustomProperties* Class = PrevClass;
                for (uint8 i = 1; i < PropertyNames.Num() - 1; i++)
                {
                    const UTIClassCustomProperties* TmpClass = Class;
                    Class = PrevClass->GetClassProperty(PropertyNames[i]);
                    PrevClass = TmpClass;
                }

                OutProperty = PropertyNames.Last();
                *OutClass = Class;
                return true;
            }
        }
    }

    *OutClass = nullptr;
    return false;
}

void UTIClassCustomProperties::Empty()
{
    BoolProperties.Empty();
    NumberProperties.Empty();
    StringProperties.Empty();
    FileProperties.Empty();
    ColorProperties.Empty();
}

void UTIClassCustomProperties::LoadProperty(const FString& PropertyName, TSharedPtr<FJsonValue> PropertyValue)
{
    if (PropertyValue)
    {
        if (PropertyValue->Type == EJson::Boolean)
        {
            BoolProperties.Add(PropertyName, PropertyValue->AsBool());
        }
        else if (PropertyValue->Type == EJson::Number)
        {
            NumberProperties.Add(PropertyName, PropertyValue->AsNumber());
        }
        else if (PropertyValue->Type == EJson::String)
        {
            const FString PropertyValueStr = PropertyValue->AsString();

            // Check if color property
            FColor Color;
            if (GetColorFromHex(PropertyValueStr, Color))
            {
                ColorProperties.Add(PropertyName, Color);
            }
            // Check if file property
            else if (!FPaths::GetExtension(PropertyValueStr).IsEmpty())
            {
                FileProperties.Add(PropertyName, PropertyValueStr);
            }
            else
            {
                StringProperties.Add(PropertyName, PropertyValueStr);
            }
        }
        else if (PropertyValue->Type == EJson::Object)
        {
            TSharedPtr<FJsonObject> ClassPropertyValue = PropertyValue->AsObject();
            if (!ClassPropertyValue->Values.IsEmpty())
            {
                UTIClassCustomProperties* ClassProperty = ClassProperties.Add(PropertyName, NewObject<UTIClassCustomProperties>());
                for (auto Pair : ClassPropertyValue->Values)
                {
                    ClassProperty->LoadProperty(Pair.Key, Pair.Value);
                }
            }
        }
    }
}

bool UTIClassCustomProperties::GetBoolProperty(const FString& PropertyName, bool& OutValue) const
{
    const auto* Property = BoolProperties.Find(PropertyName);
    if (Property != nullptr)
    {
        OutValue = *Property;
        return true;
    }

    return false;
}

bool UTIClassCustomProperties::GetNumberProperty(const FString& PropertyName, double& OutValue) const
{
    const auto* Property = NumberProperties.Find(PropertyName);
    if (Property != nullptr)
    {
        OutValue = *Property;
        return true;
    }

    return false;
}

bool UTIClassCustomProperties::GetNumberProperty(const FString& PropertyName, int8& OutValue) const
{
    double PropertyValue;
    if (GetNumberProperty(PropertyName, PropertyValue))
    {
        OutValue = static_cast<int8>(PropertyValue);
        return true;
    }

    return false;
}

bool UTIClassCustomProperties::GetNumberProperty(const FString& PropertyName, uint8& OutValue) const
{
    double PropertyValue;
    if (GetNumberProperty(PropertyName, PropertyValue))
    {
        OutValue = static_cast<uint8>(PropertyValue);
        return true;
    }

    return false;
}

bool UTIClassCustomProperties::GetNumberProperty(const FString& PropertyName, int32& OutValue) const
{
    double PropertyValue;
    if (GetNumberProperty(PropertyName, PropertyValue))
    {
        OutValue = static_cast<int32>(PropertyValue);
        return true;
    }

    return false;
}

bool UTIClassCustomProperties::GetNumberProperty(const FString& PropertyName, uint32& OutValue) const
{
    double PropertyValue;
    if (GetNumberProperty(PropertyName, PropertyValue))
    {
        OutValue = static_cast<uint32>(PropertyValue);
        return true;
    }

    return false;
}

bool UTIClassCustomProperties::GetNumberProperty(const FString& PropertyName, float& OutValue) const
{
    double PropertyValue;
    if (GetNumberProperty(PropertyName, PropertyValue))
    {
        OutValue = static_cast<float>(PropertyValue);
        return true;
    }

    return false;
}

bool UTIClassCustomProperties::GetStringProperty(const FString& PropertyName, FString& OutValue) const
{
    const auto* Property = StringProperties.Find(PropertyName);
    if (Property != nullptr)
    {
        OutValue = *Property;
        return true;
    }

    return false;
}

bool UTIClassCustomProperties::GetFileProperty(const FString& PropertyName, FString& OutValue) const
{
    const auto* Property = FileProperties.Find(PropertyName);
    if (Property != nullptr)
    {
        OutValue = *Property;
        return true;
    }

    return false;
}

bool UTIClassCustomProperties::GetColorProperty(const FString& PropertyName, FColor& OutValue) const
{
    const auto* Property = ColorProperties.Find(PropertyName);
    if (Property != nullptr)
    {
        OutValue = *Property;
        return true;
    }

    return false;
}

const UTIClassCustomProperties* UTIClassCustomProperties::GetClassProperty(const FString& PropertyName) const
{
    const auto* Property = ClassProperties.Find(PropertyName);
    return Property ? *Property : nullptr;
}

bool UTICustomProperties::GetBoolProperty(const FString& PropertyName, bool& OutValue) const
{
    FString RealPropertyName = PropertyName;
    const UTIClassCustomProperties* ClassProperty = nullptr;
    if (ParseClassPropertyFormat(PropertyName, this, RealPropertyName, &ClassProperty))
    {
        return ClassProperty->GetBoolProperty(RealPropertyName, OutValue);
    }
    else
    {
        const auto* Property = BoolProperties.Find(RealPropertyName);
        if (Property != nullptr)
        {
            OutValue = *Property;
            return true;
        }
    }

    return false;
}

bool UTICustomProperties::GetNumberProperty(const FString& PropertyName, double& OutValue) const
{
    FString RealPropertyName = PropertyName;
    const UTIClassCustomProperties* ClassProperty = nullptr;
    if (ParseClassPropertyFormat(PropertyName, this, RealPropertyName, &ClassProperty))
    {
        return ClassProperty->GetNumberProperty(RealPropertyName, OutValue);
    }
    else
    {
        const auto* Property = NumberProperties.Find(RealPropertyName);
        if (Property != nullptr)
        {
            OutValue = *Property;
            return true;
        }
    }

    return false;
}

bool UTICustomProperties::GetNumberProperty(const FString& PropertyName, int8& OutValue) const
{
    double PropertyValue;
    if (GetNumberProperty(PropertyName, PropertyValue))
    {
        OutValue = static_cast<int8>(PropertyValue);
        return true;
    }

    return false;
}

bool UTICustomProperties::GetNumberProperty(const FString& PropertyName, uint8& OutValue) const
{
    double PropertyValue;
    if (GetNumberProperty(PropertyName, PropertyValue))
    {
        OutValue = static_cast<uint8>(PropertyValue);
        return true;
    }

    return false;
}

bool UTICustomProperties::GetNumberProperty(const FString& PropertyName, int32& OutValue) const
{
    double PropertyValue;
    if (GetNumberProperty(PropertyName, PropertyValue))
    {
        OutValue = static_cast<int32>(PropertyValue);
        return true;
    }

    return false;
}

bool UTICustomProperties::GetNumberProperty(const FString& PropertyName, uint32& OutValue) const
{
    double PropertyValue;
    if (GetNumberProperty(PropertyName, PropertyValue))
    {
        OutValue = static_cast<uint32>(PropertyValue);
        return true;
    }

    return false;
}

bool UTICustomProperties::GetNumberProperty(const FString& PropertyName, float& OutValue) const
{
    double PropertyValue;
    if (GetNumberProperty(PropertyName, PropertyValue))
    {
        OutValue = static_cast<float>(PropertyValue);
        return true;
    }

    return false;
}

bool UTICustomProperties::GetStringProperty(const FString& PropertyName, FString& OutValue) const
{
    FString RealPropertyName = PropertyName;
    const UTIClassCustomProperties* ClassProperty = nullptr;
    if (ParseClassPropertyFormat(PropertyName, this, RealPropertyName, &ClassProperty))
    {
        return ClassProperty->GetStringProperty(RealPropertyName, OutValue);
    }
    else
    {
        const auto* Property = StringProperties.Find(RealPropertyName);
        if (Property != nullptr)
        {
            OutValue = *Property;
            return true;
        }
    }

    return false;
}

bool UTICustomProperties::GetFileProperty(const FString& PropertyName, FString& OutValue) const
{
    FString RealPropertyName = PropertyName;
    const UTIClassCustomProperties* ClassProperty = nullptr;
    if (ParseClassPropertyFormat(PropertyName, this, RealPropertyName, &ClassProperty))
    {
        return ClassProperty->GetFileProperty(RealPropertyName, OutValue);
    }
    else
    {
        const auto* Property = FileProperties.Find(RealPropertyName);
        if (Property != nullptr)
        {
            OutValue = *Property;
            return true;
        }
    }

    return false;
}

bool UTICustomProperties::GetColorProperty(const FString& PropertyName, FColor& OutValue) const
{
    FString RealPropertyName = PropertyName;
    const UTIClassCustomProperties* ClassProperty = nullptr;
    if (ParseClassPropertyFormat(PropertyName, this, RealPropertyName, &ClassProperty))
    {
        return ClassProperty->GetColorProperty(RealPropertyName, OutValue);
    }
    else
    {
        const auto* Property = ColorProperties.Find(RealPropertyName);
        if (Property != nullptr)
        {
            OutValue = *Property;
            return true;
        }
    }

    return false;
}

const UTIClassCustomProperties* UTICustomProperties::GetClassProperty(const FString& PropertyName) const
{
    FString RealPropertyName = PropertyName;
    const UTIClassCustomProperties* ClassProperty = nullptr;
    if (ParseClassPropertyFormat(PropertyName, this, RealPropertyName, &ClassProperty))
    {
        return ClassProperty->GetClassProperty(RealPropertyName);
    }
    else
    {
        const auto* Property = ClassProperties.Find(RealPropertyName);
        return Property ? *Property : nullptr;
    }
}

void UTICustomProperties::Empty()
{
    BoolProperties.Empty();
    NumberProperties.Empty();
    StringProperties.Empty();
    FileProperties.Empty();
    ColorProperties.Empty();
    ClassProperties.Empty();
}

void UTICustomProperties::Load(const TArray<TSharedPtr<FJsonValue>>& Data)
{
    if (!Data.IsEmpty())
    {
        Empty();

        for (TSharedPtr<FJsonValue> PropertyPtr : Data)
        {
            if (TSharedPtr<FJsonObject> Property = PropertyPtr->AsObject())
            {
                FString PropertyName;
                if (Property->TryGetStringField(TEXT("name"), PropertyName) && !PropertyName.IsEmpty())
                {
                    if (TSharedPtr<FJsonValue> PropertyValue = Property->TryGetField(TEXT("value")))
                    {
                        if (PropertyValue->Type == EJson::Object)
                        {
                            TSharedPtr<FJsonObject> ClassPropertyValue = PropertyValue->AsObject();
                            if (!ClassPropertyValue->Values.IsEmpty())
                            {
                                UTIClassCustomProperties* ClassProperty = ClassProperties.Add(PropertyName, NewObject<UTIClassCustomProperties>());
                                for (auto Pair : ClassPropertyValue->Values)
                                {
                                    ClassProperty->LoadProperty(Pair.Key, Pair.Value);
                                }
                            }
                        }
                        else if (PropertyValue->Type == EJson::Boolean)
                        {
                            BoolProperties.Add(PropertyName, PropertyValue->AsBool());
                        }
                        else if (PropertyValue->Type == EJson::Number)
                        {
                            NumberProperties.Add(PropertyName, PropertyValue->AsNumber());
                        }
                        else if (PropertyValue->Type == EJson::String)
                        {
                            FString PropertyType;
                            if (Property->TryGetStringField(TEXT("type"), PropertyType))
                            {
                                if (PropertyType == TEXT("file"))
                                {
                                    FileProperties.Add(PropertyName, PropertyValue->AsString());
                                    continue;
                                }
                                else if (PropertyType == TEXT("color"))
                                {
                                    FColor Color;
                                    if (GetColorFromHex(PropertyValue->AsString(), Color))
                                    {
                                        ColorProperties.Add(PropertyName, Color);
                                    }
                                    
                                    continue;
                                }
                            }

                            StringProperties.Add(PropertyName, PropertyValue->AsString());
                        }
                    }
                }
            }
        }
    }
}

bool UTICustomPropertiesLibrary::GetBoolProperty(const UTIClassCustomProperties* ClassProperties, const FString& PropertyName, bool& OutValue)
{
    return ClassProperties ? ClassProperties->GetBoolProperty(PropertyName, OutValue) : false;
}

bool UTICustomPropertiesLibrary::GetNumberProperty(const UTIClassCustomProperties* ClassProperties, const FString& PropertyName, double& OutValue)
{
    return ClassProperties ? ClassProperties->GetNumberProperty(PropertyName, OutValue) : false;
}

bool UTICustomPropertiesLibrary::GetStringProperty(const UTIClassCustomProperties* ClassProperties, const FString& PropertyName, FString& OutValue)
{
    return ClassProperties ? ClassProperties->GetStringProperty(PropertyName, OutValue) : false;
}

bool UTICustomPropertiesLibrary::GetFileProperty(const UTIClassCustomProperties* ClassProperties, const FString& PropertyName, FString& OutValue)
{
    return ClassProperties ? ClassProperties->GetFileProperty(PropertyName, OutValue) : false;
}

bool UTICustomPropertiesLibrary::GetColorProperty(const UTIClassCustomProperties* ClassProperties, const FString& PropertyName, FColor& OutValue)
{
    return ClassProperties ? ClassProperties->GetColorProperty(PropertyName, OutValue) : false;
}

const UTIClassCustomProperties* UTICustomPropertiesLibrary::GetClassProperty(const UTIClassCustomProperties* ClassProperties, const FString& PropertyName)
{
    return ClassProperties ? ClassProperties->GetClassProperty(PropertyName) : nullptr;
}