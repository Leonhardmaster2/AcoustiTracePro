// Copyright AcoustiTrace Pro. All Rights Reserved.

#include "AcousticSettings.h"

UAcousticSettings::UAcousticSettings()
{
    // Default initialization handled by property defaults
}

UAcousticSettings* UAcousticSettings::Get()
{
    return GetMutableDefault<UAcousticSettings>();
}

// ============================================================================
// UAcousticProfileAsset
// ============================================================================

FAcousticMaterial UAcousticProfileAsset::GetMaterialForPhysMat(FName PhysMatName) const
{
    if (const FAcousticMaterial* Found = MaterialMappings.Find(PhysMatName))
    {
        return *Found;
    }

    // Return default material
    FAcousticMaterial Default;
    Default.MaterialName = NAME_None;
    Default.MaterialType = EAcousticMaterialType::Default;
    return Default;
}

const FAcousticZonePreset* UAcousticProfileAsset::GetZonePreset(EAcousticZoneType ZoneType) const
{
    for (const FAcousticZonePreset& Preset : ZonePresets)
    {
        if (Preset.ZoneType == ZoneType)
        {
            return &Preset;
        }
    }
    return nullptr;
}
