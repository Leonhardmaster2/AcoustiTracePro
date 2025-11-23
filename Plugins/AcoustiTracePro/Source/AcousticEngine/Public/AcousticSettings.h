// Copyright AcoustiTrace Pro. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/DataAsset.h"
#include "AcousticTypes.h"
#include "AcousticSettings.generated.h"

/**
 * Global acoustic engine settings - accessible via Project Settings
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "AcoustiTrace Pro"))
class ACOUSTICENGINE_API UAcousticSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UAcousticSettings();

    // ========================================================================
    // GLOBAL RAY TRACING SETTINGS
    // ========================================================================

    /** Maximum rays per frame across all sources */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Ray Tracing|Budget", meta = (ClampMin = "16", ClampMax = "1024"))
    int32 MaxRaysPerFrame = 200;

    /** Maximum reflections computed per source */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Ray Tracing|Budget", meta = (ClampMin = "0", ClampMax = "32"))
    int32 MaxReflectionsPerSource = 8;

    /** Maximum ray bounces (1 = first bounce only, 2 = second bounce for hero) */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Ray Tracing|Budget", meta = (ClampMin = "1", ClampMax = "4"))
    int32 MaxBounces = 1;

    /** Maximum trace distance in Unreal units (cm) */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Ray Tracing|Budget", meta = (ClampMin = "1000.0", ClampMax = "50000.0"))
    float MaxTraceDistance = 10000.0f; // 100 meters

    /** Number of rays for reflection hemisphere sampling (Basic LOD) */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Ray Tracing|Quality", meta = (ClampMin = "8", ClampMax = "64"))
    int32 BasicReflectionRays = 16;

    /** Number of rays for reflection hemisphere sampling (Advanced LOD) */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Ray Tracing|Quality", meta = (ClampMin = "16", ClampMax = "64"))
    int32 AdvancedReflectionRays = 24;

    /** Number of rays for reflection hemisphere sampling (Hero LOD) */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Ray Tracing|Quality", meta = (ClampMin = "24", ClampMax = "128"))
    int32 HeroReflectionRays = 32;

    // ========================================================================
    // UPDATE RATES
    // ========================================================================

    /** Occlusion update rate in Hz */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Update Rates", meta = (ClampMin = "5.0", ClampMax = "60.0"))
    float OcclusionUpdateRateHz = 30.0f;

    /** Reflection update rate in Hz */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Update Rates", meta = (ClampMin = "5.0", ClampMax = "30.0"))
    float ReflectionUpdateRateHz = 15.0f;

    /** Zone evaluation update rate in Hz */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Update Rates", meta = (ClampMin = "5.0", ClampMax = "60.0"))
    float ZoneUpdateRateHz = 20.0f;

    /** Number of frames to cache occlusion data for non-priority sources */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Update Rates", meta = (ClampMin = "1", ClampMax = "30"))
    int32 OcclusionCacheFrames = 5;

    // ========================================================================
    // LOD THRESHOLDS
    // ========================================================================

    /** Distance threshold to auto-downgrade to Basic LOD (cm) */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "LOD", meta = (ClampMin = "500.0", ClampMax = "10000.0"))
    float BasicLODDistance = 3000.0f; // 30 meters

    /** Distance threshold to auto-downgrade to Off LOD (cm) */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "LOD", meta = (ClampMin = "1000.0", ClampMax = "30000.0"))
    float OffLODDistance = 8000.0f; // 80 meters

    /** Maximum concurrent sources at Advanced+ LOD */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "LOD", meta = (ClampMin = "1", ClampMax = "32"))
    int32 MaxAdvancedSources = 8;

    /** Maximum concurrent sources at Hero LOD */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "LOD", meta = (ClampMin = "1", ClampMax = "8"))
    int32 MaxHeroSources = 2;

    // ========================================================================
    // REALISM VS GAME MIX
    // ========================================================================

    /** Realism factor (0 = game-friendly, 1 = realistic simulation) */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Mix", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RealismFactor = 0.7f;

    /** Minimum audibility in dB - sounds never go below this (prevents complete silence) */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Mix", meta = (ClampMin = "-80.0", ClampMax = "-20.0"))
    float MinimumAudibilityDb = -50.0f;

    /** Maximum occlusion dB reduction */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Mix", meta = (ClampMin = "-60.0", ClampMax = "-6.0"))
    float MaxOcclusionDb = -30.0f;

    /** Reverb contribution scale (0-2) */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Mix", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float ReverbScale = 1.0f;

    /** Early reflection contribution scale (0-2) */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Mix", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float EarlyReflectionScale = 1.0f;

    // ========================================================================
    // HEADPHONE MODE
    // ========================================================================

    /** Force HRTF spatialization in headphone mode */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Headphones")
    bool bForceHRTFInHeadphones = true;

    /** Reverb boost multiplier for headphones (enhances spatial impression) */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Headphones", meta = (ClampMin = "0.5", ClampMax = "2.0"))
    float HeadphoneReverbBoost = 1.2f;

    /** Distance compression for headphones (reduces extreme distance perception) */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Headphones", meta = (ClampMin = "0.5", ClampMax = "1.0"))
    float HeadphoneDistanceDamp = 0.85f;

    /** Crossfeed amount for headphones (reduces extreme panning) */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Headphones", meta = (ClampMin = "0.0", ClampMax = "0.5"))
    float HeadphoneCrossfeed = 0.15f;

    // ========================================================================
    // COLLISION CHANNELS
    // ========================================================================

    /** Collision channel for audio occlusion traces */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Collision")
    TEnumAsByte<ECollisionChannel> AudioOcclusionChannel = ECC_Visibility;

    /** Collision channel for audio portal detection */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Collision")
    TEnumAsByte<ECollisionChannel> AudioPortalChannel = ECC_GameTraceChannel1;

    /** Use complex collision for more accurate traces (slower) */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Collision")
    bool bUseComplexCollision = false;

    // ========================================================================
    // DEBUG
    // ========================================================================

    /** Enable debug visualization */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bEnableDebugVisualization = false;

    /** Draw occlusion rays */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (EditCondition = "bEnableDebugVisualization"))
    bool bDrawOcclusionRays = true;

    /** Draw reflection rays */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (EditCondition = "bEnableDebugVisualization"))
    bool bDrawReflectionRays = true;

    /** Draw zone boundaries */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (EditCondition = "bEnableDebugVisualization"))
    bool bDrawZoneBoundaries = true;

    /** Show on-screen stats */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (EditCondition = "bEnableDebugVisualization"))
    bool bShowStats = true;

    // ========================================================================
    // METHODS
    // ========================================================================

    /** Get singleton settings */
    static UAcousticSettings* Get();

    /** Get category name for settings */
    virtual FName GetCategoryName() const override { return FName("Plugins"); }

#if WITH_EDITOR
    virtual FText GetSectionText() const override { return FText::FromString("AcoustiTrace Pro"); }
    virtual FText GetSectionDescription() const override { return FText::FromString("Configure acoustic simulation settings"); }
#endif
};

/**
 * Acoustic profile data asset - per-project acoustic configuration
 */
UCLASS(BlueprintType)
class ACOUSTICENGINE_API UAcousticProfileAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    /** Profile name */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile")
    FName ProfileName;

    /** Zone presets available in this profile */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zones")
    TArray<FAcousticZonePreset> ZonePresets;

    /** Material mappings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    TMap<FName, FAcousticMaterial> MaterialMappings;

    /** Get material for physical material name */
    FAcousticMaterial GetMaterialForPhysMat(FName PhysMatName) const;

    /** Get zone preset by type */
    const FAcousticZonePreset* GetZonePreset(EAcousticZoneType ZoneType) const;
};
