// Copyright AcoustiTrace Pro. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "AcousticTypes.generated.h"

// ============================================================================
// ENUMS
// ============================================================================

/**
 * Audio output mode - determines spatialization strategy
 */
UENUM(BlueprintType)
enum class EAudioOutputMode : uint8
{
    Speakers    UMETA(DisplayName = "Speakers/Standard"),
    Headphones  UMETA(DisplayName = "Headphones (HRTF)")
};

/**
 * Acoustic Level of Detail - controls processing complexity per source
 */
UENUM(BlueprintType)
enum class EAcousticLOD : uint8
{
    Off         UMETA(DisplayName = "Off - Distance attenuation only"),
    Basic       UMETA(DisplayName = "Basic - Direct ray + zone reverb"),
    Advanced    UMETA(DisplayName = "Advanced - Direct + reflections"),
    Hero        UMETA(DisplayName = "Hero - Full quality, more rays, diffraction")
};

/**
 * Source importance for prioritization
 */
UENUM(BlueprintType)
enum class EAcousticImportance : uint8
{
    Background  UMETA(DisplayName = "Background - Lowest priority"),
    Normal      UMETA(DisplayName = "Normal - Standard priority"),
    Important   UMETA(DisplayName = "Important - Higher priority"),
    Critical    UMETA(DisplayName = "Critical - Always processed")
};

/**
 * Zone type hint for reverb characteristics
 */
UENUM(BlueprintType)
enum class EAcousticZoneType : uint8
{
    Default     UMETA(DisplayName = "Default"),
    SmallRoom   UMETA(DisplayName = "Small Room"),
    LargeRoom   UMETA(DisplayName = "Large Room"),
    Hallway     UMETA(DisplayName = "Hallway/Corridor"),
    Cave        UMETA(DisplayName = "Cave/Underground"),
    Cathedral   UMETA(DisplayName = "Cathedral/Large Hall"),
    Forest      UMETA(DisplayName = "Forest/Outdoor"),
    OpenAir     UMETA(DisplayName = "Open Air/Field"),
    Underwater  UMETA(DisplayName = "Underwater"),
    Custom      UMETA(DisplayName = "Custom")
};

/**
 * Material acoustic category
 */
UENUM(BlueprintType)
enum class EAcousticMaterialType : uint8
{
    Default     UMETA(DisplayName = "Default"),
    Concrete    UMETA(DisplayName = "Concrete/Stone"),
    Wood        UMETA(DisplayName = "Wood"),
    Metal       UMETA(DisplayName = "Metal"),
    Glass       UMETA(DisplayName = "Glass"),
    Fabric      UMETA(DisplayName = "Fabric/Carpet"),
    Water       UMETA(DisplayName = "Water"),
    Foliage     UMETA(DisplayName = "Foliage/Vegetation"),
    Earth       UMETA(DisplayName = "Earth/Dirt"),
    Ice         UMETA(DisplayName = "Ice/Snow"),
    Custom      UMETA(DisplayName = "Custom")
};

// ============================================================================
// STRUCTS
// ============================================================================

/**
 * Acoustic material properties - defines how sound interacts with surfaces
 */
USTRUCT(BlueprintType)
struct ACOUSTICENGINE_API FAcousticMaterial
{
    GENERATED_BODY()

    /** Material display name */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Material")
    FName MaterialName = NAME_None;

    /** Material type preset */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Material")
    EAcousticMaterialType MaterialType = EAcousticMaterialType::Default;

    /** Low frequency absorption coefficient (0-1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Absorption", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float LowAbsorption = 0.1f;

    /** Mid frequency absorption coefficient (0-1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Absorption", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MidAbsorption = 0.2f;

    /** High frequency absorption coefficient (0-1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Absorption", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float HighAbsorption = 0.3f;

    /** Transmission coefficient (0=fully blocks, 1=fully transmits) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmission", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Transmission = 0.0f;

    /** Scattering coefficient - how diffuse reflections are (0=specular, 1=fully diffuse) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scattering", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Scattering = 0.1f;

    /** Calculate average absorption */
    float GetAverageAbsorption() const
    {
        return (LowAbsorption + MidAbsorption + HighAbsorption) / 3.0f;
    }
};

/**
 * Single reflection tap - represents one early reflection path
 */
USTRUCT(BlueprintType)
struct ACOUSTICENGINE_API FReflectionTap
{
    GENERATED_BODY()

    /** Delay time in milliseconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reflection", meta = (ClampMin = "0.0", ClampMax = "500.0"))
    float DelayMs = 0.0f;

    /** Gain/amplitude (0-1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reflection", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Gain = 0.0f;

    /** Low-pass filter cutoff frequency in Hz */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reflection", meta = (ClampMin = "200.0", ClampMax = "20000.0"))
    float LPFCutoff = 20000.0f;

    /** Azimuth angle in degrees (for spatial positioning) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reflection", meta = (ClampMin = "-180.0", ClampMax = "180.0"))
    float Azimuth = 0.0f;

    /** Elevation angle in degrees (for spatial positioning) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reflection", meta = (ClampMin = "-90.0", ClampMax = "90.0"))
    float Elevation = 0.0f;

    /** Is this tap currently valid/active */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reflection")
    bool bIsValid = false;
};

/**
 * Early reflection parameters - clustered reflection data
 */
USTRUCT(BlueprintType)
struct ACOUSTICENGINE_API FEarlyReflectionParams
{
    GENERATED_BODY()

    /** Maximum number of reflection taps */
    static constexpr int32 MaxTaps = 8;

    /** Individual reflection taps */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Early Reflections")
    TArray<FReflectionTap> Taps;

    /** Number of valid taps */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Early Reflections")
    int32 ValidTapCount = 0;

    /** Average reflection delay */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Early Reflections")
    float AverageDelayMs = 0.0f;

    /** Reflection density estimate (0-1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Early Reflections")
    float ReflectionDensity = 0.0f;

    FEarlyReflectionParams()
    {
        Taps.SetNum(MaxTaps);
    }

    /** Reset all taps */
    void Reset()
    {
        ValidTapCount = 0;
        AverageDelayMs = 0.0f;
        ReflectionDensity = 0.0f;
        for (FReflectionTap& Tap : Taps)
        {
            Tap = FReflectionTap();
        }
    }
};

/**
 * Complete acoustic parameters for a single source
 */
USTRUCT(BlueprintType)
struct ACOUSTICENGINE_API FAcousticSourceParams
{
    GENERATED_BODY()

    // === Occlusion ===

    /** Occlusion factor (0=no occlusion, 1=fully occluded) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Occlusion = 0.0f;

    /** Low-pass filter cutoff frequency in Hz */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion", meta = (ClampMin = "200.0", ClampMax = "20000.0"))
    float LowPassCutoff = 20000.0f;

    /** High-pass filter cutoff frequency in Hz */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion", meta = (ClampMin = "20.0", ClampMax = "2000.0"))
    float HighPassCutoff = 20.0f;

    /** Transmission gain through occluder */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float TransmissionGain = 1.0f;

    // === Reverb ===

    /** Reverb send amount (0-1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ReverbSend = 0.3f;

    /** Dry signal gain */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DryGain = 1.0f;

    // === Spatialization ===

    /** Spatial width (0=point source, 1=fully diffuse) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatialization", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SpatialWidth = 0.0f;

    /** HRTF spread multiplier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatialization", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float HRTFSpreadMultiplier = 1.0f;

    // === Early Reflections ===

    /** Early reflection parameters */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reflections")
    FEarlyReflectionParams EarlyReflections;

    // === Distance ===

    /** Actual distance to listener */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Distance")
    float Distance = 0.0f;

    /** Perceived distance (may differ due to acoustics) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Distance")
    float PerceivedDistance = 0.0f;

    // === State ===

    /** Frame number when last updated */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    uint64 LastUpdateFrame = 0;

    /** Is data currently valid */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    bool bIsValid = false;

    /** Reset to defaults */
    void Reset()
    {
        Occlusion = 0.0f;
        LowPassCutoff = 20000.0f;
        HighPassCutoff = 20.0f;
        TransmissionGain = 1.0f;
        ReverbSend = 0.3f;
        DryGain = 1.0f;
        SpatialWidth = 0.0f;
        HRTFSpreadMultiplier = 1.0f;
        EarlyReflections.Reset();
        Distance = 0.0f;
        PerceivedDistance = 0.0f;
        LastUpdateFrame = 0;
        bIsValid = false;
    }
};

/**
 * Zone reverb preset data
 */
USTRUCT(BlueprintType)
struct ACOUSTICENGINE_API FAcousticZonePreset
{
    GENERATED_BODY()

    /** Preset name */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone Preset")
    FName PresetName = NAME_None;

    /** Zone type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone Preset")
    EAcousticZoneType ZoneType = EAcousticZoneType::Default;

    /** Reverb time in seconds (RT60) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.1", ClampMax = "20.0"))
    float RT60 = 1.0f;

    /** High frequency decay rate multiplier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float HFDecay = 1.0f;

    /** Low frequency decay rate multiplier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float LFDecay = 1.0f;

    /** Reflection density (0=sparse, 1=dense) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Density = 0.5f;

    /** Diffusion (0=discrete echoes, 1=smooth tail) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Diffusion = 0.5f;

    /** Early reflection level relative to late reverb */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float EarlyReflectionLevel = 1.0f;

    /** Late reverb level */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float LateReverbLevel = 1.0f;

    /** Pre-delay in milliseconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float PreDelayMs = 10.0f;

    /** Room size hint (affects reverb character) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float RoomSize = 1.0f;

    /** Default reverb send for this zone */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DefaultReverbSend = 0.3f;
};

/**
 * Raycast hit result for acoustic processing
 */
USTRUCT(BlueprintType)
struct ACOUSTICENGINE_API FAcousticRayHit
{
    GENERATED_BODY()

    /** Hit location in world space */
    UPROPERTY(BlueprintReadOnly, Category = "Ray Hit")
    FVector HitLocation = FVector::ZeroVector;

    /** Hit normal */
    UPROPERTY(BlueprintReadOnly, Category = "Ray Hit")
    FVector HitNormal = FVector::UpVector;

    /** Distance from ray origin to hit */
    UPROPERTY(BlueprintReadOnly, Category = "Ray Hit")
    float Distance = 0.0f;

    /** Material at hit location */
    UPROPERTY(BlueprintReadOnly, Category = "Ray Hit")
    FAcousticMaterial Material;

    /** Physical material name (for lookup) */
    UPROPERTY(BlueprintReadOnly, Category = "Ray Hit")
    FName PhysicalMaterialName = NAME_None;

    /** Is this a valid hit */
    UPROPERTY(BlueprintReadOnly, Category = "Ray Hit")
    bool bIsValidHit = false;
};

/**
 * Listener data for spatial audio calculations
 */
USTRUCT(BlueprintType)
struct ACOUSTICENGINE_API FAcousticListenerData
{
    GENERATED_BODY()

    /** Listener world location */
    UPROPERTY(BlueprintReadOnly, Category = "Listener")
    FVector Location = FVector::ZeroVector;

    /** Listener forward direction */
    UPROPERTY(BlueprintReadOnly, Category = "Listener")
    FVector Forward = FVector::ForwardVector;

    /** Listener up direction */
    UPROPERTY(BlueprintReadOnly, Category = "Listener")
    FVector Up = FVector::UpVector;

    /** Listener right direction */
    UPROPERTY(BlueprintReadOnly, Category = "Listener")
    FVector Right = FVector::RightVector;

    /** Listener velocity for Doppler */
    UPROPERTY(BlueprintReadOnly, Category = "Listener")
    FVector Velocity = FVector::ZeroVector;

    /** Associated player controller index */
    UPROPERTY(BlueprintReadOnly, Category = "Listener")
    int32 PlayerIndex = 0;

    /** Current acoustic zone ID */
    UPROPERTY(BlueprintReadOnly, Category = "Listener")
    int32 CurrentZoneId = -1;
};

// ============================================================================
// DELEGATES
// ============================================================================

/** Delegate for when acoustic parameters update */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAcousticParamsUpdated, int32, SourceId, const FAcousticSourceParams&, Params);

/** Delegate for zone change events */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAcousticZoneChanged, int32, ListenerId, int32, OldZoneId, int32, NewZoneId);
