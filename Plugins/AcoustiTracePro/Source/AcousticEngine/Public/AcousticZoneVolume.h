// Copyright AcoustiTrace Pro. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Volume.h"
#include "AcousticTypes.h"
#include "AcousticZoneVolume.generated.h"

class UAcousticEngineSubsystem;
class USoundSubmix;

/**
 * Acoustic Zone Volume
 *
 * Defines a spatial region with specific acoustic properties.
 * When listeners enter this volume, the reverb and acoustic
 * characteristics are applied to all audio processing.
 *
 * Features:
 * - Reverb preset configuration (RT60, density, HF decay)
 * - Zone type hints for automatic behavior
 * - Priority for overlapping zones
 * - Smooth blending when transitioning
 */
UCLASS(Blueprintable, meta = (DisplayName = "Acoustic Zone Volume"))
class ACOUSTICENGINE_API AAcousticZoneVolume : public AVolume
{
    GENERATED_BODY()

public:
    AAcousticZoneVolume();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    // ========================================================================
    // ZONE CONFIGURATION
    // ========================================================================

    /** Unique zone identifier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Zone")
    FName ZoneName;

    /** Zone type (affects automatic reverb characteristics) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Zone")
    EAcousticZoneType ZoneType = EAcousticZoneType::Default;

    /** Zone priority (higher = takes precedence when overlapping) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Zone", meta = (ClampMin = "0", ClampMax = "100"))
    int32 Priority = 0;

    /** Blend time when entering/exiting zone (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Zone", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float BlendTime = 0.5f;

    // ========================================================================
    // REVERB SETTINGS
    // ========================================================================

    /** Use preset based on zone type (overrides manual settings) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Zone|Reverb")
    bool bUseZoneTypePreset = true;

    /** Reverb time in seconds (RT60) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Zone|Reverb", meta = (ClampMin = "0.1", ClampMax = "20.0", EditCondition = "!bUseZoneTypePreset"))
    float RT60 = 1.0f;

    /** High frequency decay multiplier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Zone|Reverb", meta = (ClampMin = "0.1", ClampMax = "2.0", EditCondition = "!bUseZoneTypePreset"))
    float HFDecay = 1.0f;

    /** Low frequency decay multiplier */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Zone|Reverb", meta = (ClampMin = "0.1", ClampMax = "2.0", EditCondition = "!bUseZoneTypePreset"))
    float LFDecay = 1.0f;

    /** Reflection density (0 = sparse echoes, 1 = dense reverb) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Zone|Reverb", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "!bUseZoneTypePreset"))
    float Density = 0.5f;

    /** Diffusion (0 = discrete echoes, 1 = smooth tail) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Zone|Reverb", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "!bUseZoneTypePreset"))
    float Diffusion = 0.5f;

    /** Early reflection level */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Zone|Reverb", meta = (ClampMin = "0.0", ClampMax = "2.0", EditCondition = "!bUseZoneTypePreset"))
    float EarlyReflectionLevel = 1.0f;

    /** Late reverb level */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Zone|Reverb", meta = (ClampMin = "0.0", ClampMax = "2.0", EditCondition = "!bUseZoneTypePreset"))
    float LateReverbLevel = 1.0f;

    /** Pre-delay in milliseconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Zone|Reverb", meta = (ClampMin = "0.0", ClampMax = "100.0", EditCondition = "!bUseZoneTypePreset"))
    float PreDelayMs = 10.0f;

    /** Room size factor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Zone|Reverb", meta = (ClampMin = "0.1", ClampMax = "10.0", EditCondition = "!bUseZoneTypePreset"))
    float RoomSize = 1.0f;

    // ========================================================================
    // SOURCE BEHAVIOR
    // ========================================================================

    /** Default reverb send for sources in this zone */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Zone|Sources", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DefaultReverbSend = 0.3f;

    /** Modifier to reflection density for sources */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Zone|Sources", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float ReflectionDensityMod = 1.0f;

    /** Modifier to max trace distance in this zone */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Zone|Sources", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float TraceDistanceMod = 1.0f;

    // ========================================================================
    // SUBMIX ROUTING
    // ========================================================================

    /** Custom reverb submix for this zone (optional) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Zone|Submix")
    USoundSubmix* CustomReverbSubmix = nullptr;

    /** Override master submix settings */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Zone|Submix")
    bool bOverrideSubmixSettings = false;

    // ========================================================================
    // METHODS
    // ========================================================================

    /** Get the computed zone preset */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Zone")
    FAcousticZonePreset GetZonePreset() const;

    /** Check if a point is inside this zone */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Zone")
    bool ContainsPoint(const FVector& Point) const;

    /** Get zone ID */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Zone")
    int32 GetZoneId() const { return ZoneId; }

protected:
    /** Apply zone type preset */
    void ApplyZoneTypePreset();

    /** Register with the acoustic engine */
    void RegisterWithEngine();

    /** Unregister from the acoustic engine */
    void UnregisterFromEngine();

    /** Internal zone ID */
    UPROPERTY()
    int32 ZoneId = -1;

    /** Cached preset data */
    FAcousticZonePreset CachedPreset;
};

/**
 * Acoustic Portal Volume
 *
 * Defines a passage between acoustic zones that allows
 * sound transmission with specific characteristics.
 *
 * Used for:
 * - Doorways (open/closed state)
 * - Windows
 * - Vents and ducts
 * - Any opening between zones
 */
UCLASS(Blueprintable, meta = (DisplayName = "Acoustic Portal Volume"))
class ACOUSTICENGINE_API AAcousticPortalVolume : public AVolume
{
    GENERATED_BODY()

public:
    AAcousticPortalVolume();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

    // ========================================================================
    // PORTAL CONFIGURATION
    // ========================================================================

    /** Portal name for identification */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Portal")
    FName PortalName;

    /** Is the portal currently open (allows sound through) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Portal", Replicated)
    bool bIsOpen = true;

    /** Current openness (0 = closed, 1 = fully open) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Portal", meta = (ClampMin = "0.0", ClampMax = "1.0"), Replicated)
    float Openness = 1.0f;

    // ========================================================================
    // TRANSMISSION PROPERTIES
    // ========================================================================

    /** Transmission when fully closed (0 = blocks all, 1 = no effect) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Portal|Transmission", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ClosedTransmission = 0.1f;

    /** Low frequency transmission multiplier when closed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Portal|Transmission", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float ClosedLFMultiplier = 1.5f;

    /** High frequency transmission multiplier when closed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Portal|Transmission", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ClosedHFMultiplier = 0.3f;

    /** LPF cutoff when fully closed (Hz) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Portal|Transmission", meta = (ClampMin = "200.0", ClampMax = "10000.0"))
    float ClosedLPFCutoff = 800.0f;

    // ========================================================================
    // CONNECTED ZONES
    // ========================================================================

    /** First connected zone (optional - auto-detected if not set) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Portal|Zones")
    AAcousticZoneVolume* ZoneA = nullptr;

    /** Second connected zone (optional - auto-detected if not set) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Portal|Zones")
    AAcousticZoneVolume* ZoneB = nullptr;

    // ========================================================================
    // ANIMATION
    // ========================================================================

    /** Time to transition from open to closed (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic Portal|Animation", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float TransitionTime = 0.3f;

    // ========================================================================
    // METHODS
    // ========================================================================

    /** Set portal open state */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Portal")
    void SetOpen(bool bOpen);

    /** Set portal openness directly */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Portal")
    void SetOpenness(float NewOpenness);

    /** Get current effective transmission */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Portal")
    float GetCurrentTransmission() const;

    /** Get current LPF cutoff */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Portal")
    float GetCurrentLPFCutoff() const;

    /** Check if sound path passes through this portal */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Portal")
    bool IsOnSoundPath(const FVector& SourceLocation, const FVector& ListenerLocation) const;

    /** Get portal center location */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Portal")
    FVector GetPortalCenter() const;

    /** Get portal forward direction */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Portal")
    FVector GetPortalNormal() const;

    // ========================================================================
    // REPLICATION
    // ========================================================================

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    /** Register with the acoustic engine */
    void RegisterWithEngine();

    /** Unregister from the acoustic engine */
    void UnregisterFromEngine();

    /** Auto-detect connected zones */
    void DetectConnectedZones();

    /** Update transition animation */
    void UpdateTransition(float DeltaTime);

    /** Target openness for animation */
    float TargetOpenness = 1.0f;

    /** Internal portal ID */
    UPROPERTY()
    int32 PortalId = -1;
};
