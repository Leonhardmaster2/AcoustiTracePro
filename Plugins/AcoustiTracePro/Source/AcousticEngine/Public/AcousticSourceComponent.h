// Copyright AcoustiTrace Pro. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "AcousticTypes.h"
#include "AcousticSourceComponent.generated.h"

class UAudioComponent;
class USoundBase;
class UAcousticEngineSubsystem;

/**
 * Source flags for special behavior
 */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EAcousticSourceFlags : uint8
{
    None            = 0         UMETA(Hidden),
    IsHero          = 1 << 0    UMETA(DisplayName = "Hero Source"),
    Environmental   = 1 << 1    UMETA(DisplayName = "Environmental"),
    UI              = 1 << 2    UMETA(DisplayName = "UI/Non-Diegetic"),
    AlwaysAudible   = 1 << 3    UMETA(DisplayName = "Always Audible"),
    NeverOcclude    = 1 << 4    UMETA(DisplayName = "Never Occlude"),
    LargeSource     = 1 << 5    UMETA(DisplayName = "Large Source (spread)")
};
ENUM_CLASS_FLAGS(EAcousticSourceFlags);

/**
 * Acoustic Source Component
 *
 * Wraps or works alongside UAudioComponent to provide advanced acoustic
 * simulation including occlusion, reflections, and spatial processing.
 *
 * Usage:
 * - Add alongside UAudioComponent on any actor that emits sound
 * - Configure LOD, importance, and flags
 * - The Acoustic Engine automatically processes and updates parameters
 */
UCLASS(ClassGroup = (Audio), meta = (BlueprintSpawnableComponent, DisplayName = "Acoustic Source"))
class ACOUSTICENGINE_API UAcousticSourceComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UAcousticSourceComponent();

    // ========================================================================
    // COMPONENT LIFECYCLE
    // ========================================================================

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /** Acoustic Level of Detail - controls processing complexity */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic|Configuration")
    EAcousticLOD AcousticLOD = EAcousticLOD::Advanced;

    /** Source importance for prioritization */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic|Configuration")
    EAcousticImportance Importance = EAcousticImportance::Normal;

    /** Source flags for special behavior */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic|Configuration", meta = (Bitmask, BitmaskEnum = "EAcousticSourceFlags"))
    uint8 SourceFlags = 0;

    /** Base loudness (affects priority calculation) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic|Configuration", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BaseLoudness = 1.0f;

    /** Custom priority override (-1 = use automatic) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic|Configuration", meta = (ClampMin = "-1.0", ClampMax = "100.0"))
    float PriorityOverride = -1.0f;

    // ========================================================================
    // SPATIAL SETTINGS
    // ========================================================================

    /** Base spatial width (0 = point, 1 = diffuse) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic|Spatial", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BaseSpatialWidth = 0.0f;

    /** Source radius for large sources (used when LargeSource flag is set) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic|Spatial", meta = (ClampMin = "0.0", ClampMax = "10000.0", EditCondition = "SourceFlags & 32"))
    float SourceRadius = 100.0f;

    /** Custom reverb send override (-1 = use computed) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic|Spatial", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float ReverbSendOverride = -1.0f;

    // ========================================================================
    // LINKED AUDIO COMPONENT
    // ========================================================================

    /** Linked audio component (auto-detected if not set) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic|Audio")
    UAudioComponent* LinkedAudioComponent = nullptr;

    /** Auto-create audio component if none exists */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic|Audio")
    bool bAutoCreateAudioComponent = false;

    /** Sound to play (if auto-creating audio component) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic|Audio", meta = (EditCondition = "bAutoCreateAudioComponent"))
    USoundBase* Sound = nullptr;

    /** Auto-play on begin play */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustic|Audio", meta = (EditCondition = "bAutoCreateAudioComponent"))
    bool bAutoPlay = false;

    // ========================================================================
    // RUNTIME STATE (READ-ONLY)
    // ========================================================================

    /** Current acoustic parameters (computed by engine) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Acoustic|Runtime")
    FAcousticSourceParams CurrentParams;

    /** Effective LOD (may differ from configured due to budget) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Acoustic|Runtime")
    EAcousticLOD EffectiveLOD = EAcousticLOD::Basic;

    /** Registered source ID */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Acoustic|Runtime")
    int32 SourceId = -1;

    /** Is currently registered with the engine */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Acoustic|Runtime")
    bool bIsRegistered = false;

    // ========================================================================
    // BLUEPRINT CALLABLE
    // ========================================================================

    /** Set acoustic LOD at runtime */
    UFUNCTION(BlueprintCallable, Category = "Acoustic")
    void SetAcousticLOD(EAcousticLOD NewLOD);

    /** Set importance at runtime */
    UFUNCTION(BlueprintCallable, Category = "Acoustic")
    void SetImportance(EAcousticImportance NewImportance);

    /** Check if a flag is set */
    UFUNCTION(BlueprintCallable, Category = "Acoustic")
    bool HasFlag(EAcousticSourceFlags Flag) const;

    /** Set a source flag */
    UFUNCTION(BlueprintCallable, Category = "Acoustic")
    void SetFlag(EAcousticSourceFlags Flag, bool bEnabled);

    /** Force immediate acoustic update */
    UFUNCTION(BlueprintCallable, Category = "Acoustic")
    void ForceUpdate();

    /** Get current occlusion value */
    UFUNCTION(BlueprintCallable, Category = "Acoustic")
    float GetOcclusion() const { return CurrentParams.Occlusion; }

    /** Get current reverb send */
    UFUNCTION(BlueprintCallable, Category = "Acoustic")
    float GetReverbSend() const { return CurrentParams.ReverbSend; }

    /** Get computed spatial width */
    UFUNCTION(BlueprintCallable, Category = "Acoustic")
    float GetSpatialWidth() const { return CurrentParams.SpatialWidth; }

    /** Get distance to nearest listener */
    UFUNCTION(BlueprintCallable, Category = "Acoustic")
    float GetDistanceToListener() const { return CurrentParams.Distance; }

    /** Check if source is currently audible */
    UFUNCTION(BlueprintCallable, Category = "Acoustic")
    bool IsAudible() const;

    // ========================================================================
    // METASOUND INTEGRATION
    // ========================================================================

    /** Get parameter name for occlusion (for MetaSound binding) */
    UFUNCTION(BlueprintCallable, Category = "Acoustic|MetaSound")
    static FName GetOcclusionParamName() { return FName("Acoustic_Occlusion"); }

    /** Get parameter name for LPF cutoff */
    UFUNCTION(BlueprintCallable, Category = "Acoustic|MetaSound")
    static FName GetLPFCutoffParamName() { return FName("Acoustic_LPFCutoff"); }

    /** Get parameter name for reverb send */
    UFUNCTION(BlueprintCallable, Category = "Acoustic|MetaSound")
    static FName GetReverbSendParamName() { return FName("Acoustic_ReverbSend"); }

    /** Get parameter name for spatial width */
    UFUNCTION(BlueprintCallable, Category = "Acoustic|MetaSound")
    static FName GetSpatialWidthParamName() { return FName("Acoustic_SpatialWidth"); }

    // ========================================================================
    // INTERNAL
    // ========================================================================

    /** Called by engine when params are updated */
    void OnParamsUpdated(const FAcousticSourceParams& NewParams);

    /** Get computed priority score */
    float ComputePriorityScore(const FVector& ListenerLocation) const;

    /** Get world location for acoustic calculations */
    FVector GetAcousticLocation() const;

protected:
    /** Register with the acoustic engine */
    void RegisterWithEngine();

    /** Unregister from the acoustic engine */
    void UnregisterFromEngine();

    /** Find and link audio component */
    void FindLinkedAudioComponent();

    /** Apply params to linked audio component */
    void ApplyParamsToAudio();

    /** Create audio component if needed */
    void CreateAudioComponentIfNeeded();

    /** Cached subsystem reference */
    UPROPERTY()
    UAcousticEngineSubsystem* CachedSubsystem = nullptr;
};

// ============================================================================
// BLUEPRINT FUNCTION LIBRARY
// ============================================================================

/**
 * Blueprint function library for acoustic operations
 */
UCLASS()
class ACOUSTICENGINE_API UAcousticBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Get the acoustic engine subsystem */
    UFUNCTION(BlueprintCallable, Category = "Acoustic", meta = (WorldContext = "WorldContextObject"))
    static UAcousticEngineSubsystem* GetAcousticEngine(UObject* WorldContextObject);

    /** Set global audio output mode */
    UFUNCTION(BlueprintCallable, Category = "Acoustic", meta = (WorldContext = "WorldContextObject"))
    static void SetAudioOutputMode(UObject* WorldContextObject, EAudioOutputMode Mode);

    /** Get current audio output mode */
    UFUNCTION(BlueprintCallable, Category = "Acoustic", meta = (WorldContext = "WorldContextObject"))
    static EAudioOutputMode GetAudioOutputMode(UObject* WorldContextObject);

    /** Trace occlusion between two points */
    UFUNCTION(BlueprintCallable, Category = "Acoustic", meta = (WorldContext = "WorldContextObject"))
    static float TraceOcclusion(UObject* WorldContextObject, FVector Start, FVector End);

    /** Get acoustic material for surface at location */
    UFUNCTION(BlueprintCallable, Category = "Acoustic", meta = (WorldContext = "WorldContextObject"))
    static FAcousticMaterial GetSurfaceMaterial(UObject* WorldContextObject, FVector Location, FVector Direction);
};
