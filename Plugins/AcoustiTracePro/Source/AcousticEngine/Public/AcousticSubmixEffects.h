// Copyright AcoustiTrace Pro. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundEffectSubmix.h"
#include "DSP/Dsp.h"
#include "AcousticTypes.h"
#include "AcousticSubmixEffects.generated.h"

// ============================================================================
// ZONE REVERB SUBMIX EFFECT
// ============================================================================

/**
 * Zone Reverb Submix Effect Settings
 *
 * Configures the algorithmic reverb driven by acoustic zones.
 */
USTRUCT(BlueprintType)
struct ACOUSTICENGINE_API FAcousticZoneReverbSettings
{
    GENERATED_BODY()

    /** Reverb time in seconds (RT60) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.1", ClampMax = "20.0"))
    float RT60 = 1.5f;

    /** Pre-delay in milliseconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float PreDelayMs = 20.0f;

    /** High frequency decay multiplier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float HFDecay = 0.8f;

    /** Low frequency decay multiplier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float LFDecay = 1.0f;

    /** Diffusion amount */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Diffusion = 0.7f;

    /** Density amount */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Density = 0.5f;

    /** Early reflection level */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float EarlyLevel = 1.0f;

    /** Late reverb level */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float LateLevel = 1.0f;

    /** Wet/dry mix */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WetLevel = 0.5f;

    /** Room size factor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float RoomSize = 1.0f;

    /** Stereo width of reverb */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float StereoWidth = 1.0f;

    /** Blend time when changing settings (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float BlendTime = 0.5f;

    /** Initialize from zone preset */
    void InitFromZonePreset(const FAcousticZonePreset& Preset);
};

/**
 * Zone Reverb Submix Effect Preset
 */
UCLASS()
class ACOUSTICENGINE_API UAcousticZoneReverbPreset : public USoundEffectSubmixPreset
{
    GENERATED_BODY()

public:
    EFFECT_PRESET_METHODS(AcousticZoneReverb)

    /** Settings for this preset */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FAcousticZoneReverbSettings Settings;

    /** Update settings at runtime */
    UFUNCTION(BlueprintCallable, Category = "Reverb")
    void SetSettings(const FAcousticZoneReverbSettings& InSettings);

    /** Update from zone preset */
    UFUNCTION(BlueprintCallable, Category = "Reverb")
    void SetFromZonePreset(const FAcousticZonePreset& ZonePreset);
};

/**
 * Zone Reverb Submix Effect
 *
 * Algorithmic reverb effect that can be driven by acoustic zones.
 * Supports smooth blending between different reverb settings.
 */
UCLASS()
class ACOUSTICENGINE_API FAcousticZoneReverbEffect : public FSoundEffectSubmix
{
public:
    virtual void Init(const FSoundEffectSubmixInitData& InitData) override;
    virtual void OnPresetChanged() override;
    virtual uint32 GetDesiredInputChannelCountOverride() const override { return 2; }
    virtual void OnProcessAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData) override;

    /** Set target settings with blend */
    void SetTargetSettings(const FAcousticZoneReverbSettings& InSettings);

private:
    /** Current active settings */
    FAcousticZoneReverbSettings CurrentSettings;

    /** Target settings for blending */
    FAcousticZoneReverbSettings TargetSettings;

    /** Blend progress (0-1) */
    float BlendProgress = 1.0f;

    /** Is currently blending */
    bool bIsBlending = false;

    /** Sample rate */
    float SampleRate = 48000.0f;

    /** Number of channels */
    int32 NumChannels = 2;

    // Reverb DSP components (simplified representation)
    // In practice, these would be more complex structures

    /** Pre-delay buffer */
    TArray<float> PreDelayBuffer;
    int32 PreDelayWriteIndex = 0;

    /** Early reflection taps */
    struct FEarlyTap
    {
        float DelayMs;
        float Gain;
        float Pan;
    };
    TArray<FEarlyTap> EarlyTaps;

    /** Allpass diffusers */
    struct FAllpassDiffuser
    {
        TArray<float> Buffer;
        int32 WriteIndex;
        float Feedback;
    };
    TArray<FAllpassDiffuser> Diffusers;

    /** Feedback delay network */
    struct FFDNTank
    {
        TArray<float> Buffer;
        int32 WriteIndex;
        float Feedback;
        float LPFState;
        float HPFState;
    };
    TArray<FFDNTank> FDNTanks;

    /** Output processing */
    float OutputLPFState[2] = {0.0f, 0.0f};

    /** Initialize DSP structures */
    void InitializeDSP();

    /** Process early reflections */
    void ProcessEarlyReflections(const float* InBuffer, float* OutBuffer, int32 NumFrames);

    /** Process late reverb (FDN) */
    void ProcessLateReverb(const float* InBuffer, float* OutBuffer, int32 NumFrames);

    /** Update blend */
    void UpdateBlend(int32 NumFrames);

    /** Interpolate settings */
    FAcousticZoneReverbSettings InterpolateSettings(float Alpha) const;
};

// ============================================================================
// HEADPHONE CROSSFEED SUBMIX EFFECT
// ============================================================================

/**
 * Headphone Crossfeed Settings
 *
 * Applies subtle crossfeed for more natural headphone listening.
 */
USTRUCT(BlueprintType)
struct ACOUSTICENGINE_API FHeadphoneCrossfeedSettings
{
    GENERATED_BODY()

    /** Enable crossfeed processing */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crossfeed")
    bool bEnabled = true;

    /** Crossfeed amount (0-1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crossfeed", meta = (ClampMin = "0.0", ClampMax = "0.5"))
    float CrossfeedAmount = 0.15f;

    /** Crossfeed delay in microseconds (simulates head width) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crossfeed", meta = (ClampMin = "0.0", ClampMax = "500.0"))
    float CrossfeedDelayUs = 200.0f;

    /** High frequency rolloff for crossfeed (Hz) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crossfeed", meta = (ClampMin = "500.0", ClampMax = "8000.0"))
    float CrossfeedLPFHz = 2000.0f;

    /** Bass boost for crossfeed signal (dB) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crossfeed", meta = (ClampMin = "-6.0", ClampMax = "6.0"))
    float BassBoostDb = 0.0f;
};

/**
 * Headphone Crossfeed Submix Effect Preset
 */
UCLASS()
class ACOUSTICENGINE_API UHeadphoneCrossfeedPreset : public USoundEffectSubmixPreset
{
    GENERATED_BODY()

public:
    EFFECT_PRESET_METHODS(HeadphoneCrossfeed)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FHeadphoneCrossfeedSettings Settings;
};

/**
 * Headphone Crossfeed Submix Effect
 *
 * Applies subtle crossfeed between channels for more natural
 * headphone listening, reducing the "in-head" sensation.
 */
UCLASS()
class ACOUSTICENGINE_API FHeadphoneCrossfeedEffect : public FSoundEffectSubmix
{
public:
    virtual void Init(const FSoundEffectSubmixInitData& InitData) override;
    virtual void OnPresetChanged() override;
    virtual uint32 GetDesiredInputChannelCountOverride() const override { return 2; }
    virtual void OnProcessAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData) override;

private:
    FHeadphoneCrossfeedSettings CurrentSettings;
    float SampleRate = 48000.0f;

    // Crossfeed delay lines
    TArray<float> CrossfeedDelayL;
    TArray<float> CrossfeedDelayR;
    int32 DelayWriteIndex = 0;

    // LPF state for crossfeed
    float CrossfeedLPFStateL = 0.0f;
    float CrossfeedLPFStateR = 0.0f;

    // Bass shelf filter states
    float BassFilterStateL = 0.0f;
    float BassFilterStateR = 0.0f;
};

// ============================================================================
// ACOUSTIC MASTER SUBMIX EFFECT
// ============================================================================

/**
 * Acoustic Master Settings
 *
 * Global acoustic processing applied to the final mix.
 */
USTRUCT(BlueprintType)
struct ACOUSTICENGINE_API FAcousticMasterSettings
{
    GENERATED_BODY()

    /** Enable acoustic master processing */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Master")
    bool bEnabled = true;

    /** Output mode (affects processing chain) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Master")
    EAudioOutputMode OutputMode = EAudioOutputMode::Speakers;

    /** Global reverb level */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Master", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float GlobalReverbLevel = 1.0f;

    /** Distance perception compression (for headphones) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Master", meta = (ClampMin = "0.5", ClampMax = "1.0"))
    float DistanceCompression = 1.0f;

    /** Final limiter threshold (dB) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Master", meta = (ClampMin = "-12.0", ClampMax = "0.0"))
    float LimiterThresholdDb = -1.0f;

    /** Final output gain (dB) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Master", meta = (ClampMin = "-24.0", ClampMax = "12.0"))
    float OutputGainDb = 0.0f;
};

/**
 * Acoustic Master Submix Effect Preset
 */
UCLASS()
class ACOUSTICENGINE_API UAcousticMasterPreset : public USoundEffectSubmixPreset
{
    GENERATED_BODY()

public:
    EFFECT_PRESET_METHODS(AcousticMaster)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FAcousticMasterSettings Settings;
};

/**
 * Acoustic Master Submix Effect
 *
 * Final processing stage for the acoustic system.
 * Handles headphone-specific processing and limiting.
 */
UCLASS()
class ACOUSTICENGINE_API FAcousticMasterEffect : public FSoundEffectSubmix
{
public:
    virtual void Init(const FSoundEffectSubmixInitData& InitData) override;
    virtual void OnPresetChanged() override;
    virtual uint32 GetDesiredInputChannelCountOverride() const override { return 2; }
    virtual void OnProcessAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData) override;

private:
    FAcousticMasterSettings CurrentSettings;
    float SampleRate = 48000.0f;

    // Limiter state
    float LimiterEnvelope = 0.0f;
    float LimiterGain = 1.0f;

    // Smoothed gain
    float SmoothedOutputGain = 1.0f;
};
