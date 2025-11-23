// Copyright AcoustiTrace Pro. All Rights Reserved.

#include "AcousticTypes.h"

// ============================================================================
// TYPE IMPLEMENTATIONS
// ============================================================================

// Most type implementations are in the header as they are simple structs.
// This file contains any additional utility functions that require
// implementation outside the header.

// ============================================================================
// ACOUSTIC MATERIAL UTILITIES
// ============================================================================

FAcousticMaterial FAcousticMaterial::CreateFromType(EAcousticMaterialType Type)
{
    FAcousticMaterial Material;
    Material.MaterialType = Type;

    switch (Type)
    {
        case EAcousticMaterialType::Concrete:
            Material.MaterialName = FName("Concrete");
            Material.LowAbsorption = 0.01f;
            Material.MidAbsorption = 0.02f;
            Material.HighAbsorption = 0.02f;
            Material.Transmission = 0.0f;
            Material.Scattering = 0.05f;
            break;

        case EAcousticMaterialType::Wood:
            Material.MaterialName = FName("Wood");
            Material.LowAbsorption = 0.15f;
            Material.MidAbsorption = 0.11f;
            Material.HighAbsorption = 0.10f;
            Material.Transmission = 0.05f;
            Material.Scattering = 0.10f;
            break;

        case EAcousticMaterialType::Metal:
            Material.MaterialName = FName("Metal");
            Material.LowAbsorption = 0.01f;
            Material.MidAbsorption = 0.01f;
            Material.HighAbsorption = 0.02f;
            Material.Transmission = 0.0f;
            Material.Scattering = 0.02f;
            break;

        case EAcousticMaterialType::Glass:
            Material.MaterialName = FName("Glass");
            Material.LowAbsorption = 0.18f;
            Material.MidAbsorption = 0.06f;
            Material.HighAbsorption = 0.04f;
            Material.Transmission = 0.3f;
            Material.Scattering = 0.02f;
            break;

        case EAcousticMaterialType::Fabric:
            Material.MaterialName = FName("Fabric");
            Material.LowAbsorption = 0.03f;
            Material.MidAbsorption = 0.12f;
            Material.HighAbsorption = 0.35f;
            Material.Transmission = 0.4f;
            Material.Scattering = 0.30f;
            break;

        case EAcousticMaterialType::Water:
            Material.MaterialName = FName("Water");
            Material.LowAbsorption = 0.01f;
            Material.MidAbsorption = 0.01f;
            Material.HighAbsorption = 0.02f;
            Material.Transmission = 0.2f;
            Material.Scattering = 0.50f;
            break;

        case EAcousticMaterialType::Foliage:
            Material.MaterialName = FName("Foliage");
            Material.LowAbsorption = 0.03f;
            Material.MidAbsorption = 0.06f;
            Material.HighAbsorption = 0.11f;
            Material.Transmission = 0.8f;
            Material.Scattering = 0.70f;
            break;

        case EAcousticMaterialType::Earth:
            Material.MaterialName = FName("Earth");
            Material.LowAbsorption = 0.15f;
            Material.MidAbsorption = 0.25f;
            Material.HighAbsorption = 0.40f;
            Material.Transmission = 0.0f;
            Material.Scattering = 0.40f;
            break;

        case EAcousticMaterialType::Ice:
            Material.MaterialName = FName("Ice");
            Material.LowAbsorption = 0.01f;
            Material.MidAbsorption = 0.01f;
            Material.HighAbsorption = 0.02f;
            Material.Transmission = 0.1f;
            Material.Scattering = 0.05f;
            break;

        case EAcousticMaterialType::Default:
        case EAcousticMaterialType::Custom:
        default:
            Material.MaterialName = FName("Default");
            Material.LowAbsorption = 0.10f;
            Material.MidAbsorption = 0.20f;
            Material.HighAbsorption = 0.30f;
            Material.Transmission = 0.0f;
            Material.Scattering = 0.10f;
            break;
    }

    return Material;
}

// ============================================================================
// ACOUSTIC ZONE PRESET UTILITIES
// ============================================================================

FAcousticZonePreset FAcousticZonePreset::CreateFromType(EAcousticZoneType Type)
{
    FAcousticZonePreset Preset;
    Preset.ZoneType = Type;

    switch (Type)
    {
        case EAcousticZoneType::SmallRoom:
            Preset.PresetName = FName("SmallRoom");
            Preset.RT60 = 0.3f;
            Preset.HFDecay = 0.9f;
            Preset.LFDecay = 1.0f;
            Preset.Density = 0.7f;
            Preset.Diffusion = 0.6f;
            Preset.EarlyReflectionLevel = 1.2f;
            Preset.LateReverbLevel = 0.8f;
            Preset.PreDelayMs = 5.0f;
            Preset.RoomSize = 0.3f;
            Preset.DefaultReverbSend = 0.25f;
            break;

        case EAcousticZoneType::LargeRoom:
            Preset.PresetName = FName("LargeRoom");
            Preset.RT60 = 0.8f;
            Preset.HFDecay = 0.8f;
            Preset.LFDecay = 1.0f;
            Preset.Density = 0.5f;
            Preset.Diffusion = 0.5f;
            Preset.EarlyReflectionLevel = 1.0f;
            Preset.LateReverbLevel = 1.0f;
            Preset.PreDelayMs = 15.0f;
            Preset.RoomSize = 1.0f;
            Preset.DefaultReverbSend = 0.35f;
            break;

        case EAcousticZoneType::Hallway:
            Preset.PresetName = FName("Hallway");
            Preset.RT60 = 1.2f;
            Preset.HFDecay = 0.7f;
            Preset.LFDecay = 1.1f;
            Preset.Density = 0.3f;
            Preset.Diffusion = 0.3f;
            Preset.EarlyReflectionLevel = 1.5f;
            Preset.LateReverbLevel = 0.7f;
            Preset.PreDelayMs = 8.0f;
            Preset.RoomSize = 0.6f;
            Preset.DefaultReverbSend = 0.4f;
            break;

        case EAcousticZoneType::Cave:
            Preset.PresetName = FName("Cave");
            Preset.RT60 = 3.0f;
            Preset.HFDecay = 0.6f;
            Preset.LFDecay = 1.2f;
            Preset.Density = 0.8f;
            Preset.Diffusion = 0.7f;
            Preset.EarlyReflectionLevel = 1.3f;
            Preset.LateReverbLevel = 1.2f;
            Preset.PreDelayMs = 25.0f;
            Preset.RoomSize = 2.0f;
            Preset.DefaultReverbSend = 0.5f;
            break;

        case EAcousticZoneType::Cathedral:
            Preset.PresetName = FName("Cathedral");
            Preset.RT60 = 4.0f;
            Preset.HFDecay = 0.5f;
            Preset.LFDecay = 1.0f;
            Preset.Density = 0.6f;
            Preset.Diffusion = 0.8f;
            Preset.EarlyReflectionLevel = 0.8f;
            Preset.LateReverbLevel = 1.5f;
            Preset.PreDelayMs = 40.0f;
            Preset.RoomSize = 5.0f;
            Preset.DefaultReverbSend = 0.6f;
            break;

        case EAcousticZoneType::Forest:
            Preset.PresetName = FName("Forest");
            Preset.RT60 = 0.2f;
            Preset.HFDecay = 1.0f;
            Preset.LFDecay = 0.8f;
            Preset.Density = 0.2f;
            Preset.Diffusion = 0.9f;
            Preset.EarlyReflectionLevel = 0.5f;
            Preset.LateReverbLevel = 0.3f;
            Preset.PreDelayMs = 3.0f;
            Preset.RoomSize = 0.5f;
            Preset.DefaultReverbSend = 0.15f;
            break;

        case EAcousticZoneType::OpenAir:
            Preset.PresetName = FName("OpenAir");
            Preset.RT60 = 0.1f;
            Preset.HFDecay = 1.0f;
            Preset.LFDecay = 1.0f;
            Preset.Density = 0.1f;
            Preset.Diffusion = 0.5f;
            Preset.EarlyReflectionLevel = 0.2f;
            Preset.LateReverbLevel = 0.1f;
            Preset.PreDelayMs = 0.0f;
            Preset.RoomSize = 0.1f;
            Preset.DefaultReverbSend = 0.05f;
            break;

        case EAcousticZoneType::Underwater:
            Preset.PresetName = FName("Underwater");
            Preset.RT60 = 0.5f;
            Preset.HFDecay = 0.3f;
            Preset.LFDecay = 1.5f;
            Preset.Density = 0.9f;
            Preset.Diffusion = 0.9f;
            Preset.EarlyReflectionLevel = 0.8f;
            Preset.LateReverbLevel = 1.0f;
            Preset.PreDelayMs = 10.0f;
            Preset.RoomSize = 1.0f;
            Preset.DefaultReverbSend = 0.7f;
            break;

        case EAcousticZoneType::Default:
        case EAcousticZoneType::Custom:
        default:
            Preset.PresetName = FName("Default");
            Preset.RT60 = 1.0f;
            Preset.HFDecay = 1.0f;
            Preset.LFDecay = 1.0f;
            Preset.Density = 0.5f;
            Preset.Diffusion = 0.5f;
            Preset.EarlyReflectionLevel = 1.0f;
            Preset.LateReverbLevel = 1.0f;
            Preset.PreDelayMs = 10.0f;
            Preset.RoomSize = 1.0f;
            Preset.DefaultReverbSend = 0.3f;
            break;
    }

    return Preset;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

namespace AcousticUtils
{
    float DbToLinear(float Db)
    {
        return FMath::Pow(10.0f, Db / 20.0f);
    }

    float LinearToDb(float Linear)
    {
        if (Linear <= 0.0f)
        {
            return -100.0f;
        }
        return 20.0f * FMath::LogX(10.0f, Linear);
    }

    float DistanceToDelayMs(float DistanceCm)
    {
        // Speed of sound = 34300 cm/s
        // Delay = Distance / Speed * 1000 (for ms)
        constexpr float SpeedOfSoundCmPerMs = 34.3f;
        return DistanceCm / SpeedOfSoundCmPerMs;
    }

    float DelayMsToDistance(float DelayMs)
    {
        constexpr float SpeedOfSoundCmPerMs = 34.3f;
        return DelayMs * SpeedOfSoundCmPerMs;
    }

    float CalculateLPFCutoffFromOcclusion(float Occlusion, float MinCutoff, float MaxCutoff)
    {
        // Logarithmic interpolation for more natural feel
        float LogMin = FMath::Loge(MinCutoff);
        float LogMax = FMath::Loge(MaxCutoff);
        float LogCutoff = FMath::Lerp(LogMax, LogMin, Occlusion);
        return FMath::Exp(LogCutoff);
    }

    float CalculateDistanceAttenuation(float Distance, float ReferenceDistance, float MaxDistance)
    {
        if (Distance <= ReferenceDistance)
        {
            return 1.0f;
        }

        if (Distance >= MaxDistance)
        {
            return 0.0f;
        }

        // Inverse distance falloff with rolloff
        float NormalizedDistance = (Distance - ReferenceDistance) / (MaxDistance - ReferenceDistance);
        return 1.0f - NormalizedDistance;
    }

    FVector CalculateReflectionDirection(const FVector& Incident, const FVector& Normal)
    {
        // R = I - 2(I·N)N
        return Incident - 2.0f * FVector::DotProduct(Incident, Normal) * Normal;
    }
}
