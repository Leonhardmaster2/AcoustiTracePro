// Copyright AcoustiTrace Pro. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AcousticTypes.h"
#include "AcousticEngineSubsystem.generated.h"

class UAcousticSourceComponent;
class AAcousticZoneVolume;
class AAcousticPortalVolume;
class UAcousticSettings;
class UAcousticProfileAsset;

/**
 * Ray budget allocation for a single frame
 */
USTRUCT()
struct FRayBudgetAllocation
{
    GENERATED_BODY()

    int32 OcclusionRays = 0;
    int32 ReflectionRays = 0;
    int32 TotalRaysUsed = 0;
    int32 TotalRaysBudget = 0;
};

/**
 * Registered acoustic source entry
 */
USTRUCT()
struct FAcousticSourceEntry
{
    GENERATED_BODY()

    /** Weak pointer to the source component */
    TWeakObjectPtr<UAcousticSourceComponent> SourceComponent;

    /** Unique source ID */
    int32 SourceId = -1;

    /** Current computed parameters */
    FAcousticSourceParams CurrentParams;

    /** Previous frame parameters (for interpolation) */
    FAcousticSourceParams PreviousParams;

    /** Effective LOD after budget considerations */
    EAcousticLOD EffectiveLOD = EAcousticLOD::Basic;

    /** Priority score for ray budget allocation */
    float PriorityScore = 0.0f;

    /** Last occlusion update time */
    double LastOcclusionUpdateTime = 0.0;

    /** Last reflection update time */
    double LastReflectionUpdateTime = 0.0;

    /** Is this source currently audible */
    bool bIsAudible = true;
};

/**
 * Acoustic Engine World Subsystem
 *
 * Core manager for all acoustic processing in a world.
 * Handles ray tracing, source prioritization, zone management,
 * and parameter distribution to audio components.
 */
UCLASS()
class ACOUSTICENGINE_API UAcousticEngineSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UAcousticEngineSubsystem();

    // ========================================================================
    // SUBSYSTEM LIFECYCLE
    // ========================================================================

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    /** Tick the acoustic engine */
    void Tick(float DeltaTime);

    // ========================================================================
    // SOURCE MANAGEMENT
    // ========================================================================

    /** Register an acoustic source component */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Engine")
    int32 RegisterSource(UAcousticSourceComponent* Source);

    /** Unregister an acoustic source component */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Engine")
    void UnregisterSource(int32 SourceId);

    /** Get current parameters for a source */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Engine")
    bool GetSourceParams(int32 SourceId, FAcousticSourceParams& OutParams) const;

    /** Force immediate update for a source */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Engine")
    void ForceSourceUpdate(int32 SourceId);

    // ========================================================================
    // LISTENER MANAGEMENT
    // ========================================================================

    /** Update listener data (called by audio system) */
    void UpdateListener(int32 ListenerIndex, const FAcousticListenerData& ListenerData);

    /** Get current listener data */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Engine")
    FAcousticListenerData GetListenerData(int32 ListenerIndex = 0) const;

    /** Get number of active listeners */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Engine")
    int32 GetNumListeners() const { return ListenerDataArray.Num(); }

    // ========================================================================
    // ZONE MANAGEMENT
    // ========================================================================

    /** Register an acoustic zone volume */
    void RegisterZone(AAcousticZoneVolume* Zone);

    /** Unregister an acoustic zone volume */
    void UnregisterZone(AAcousticZoneVolume* Zone);

    /** Register a portal volume */
    void RegisterPortal(AAcousticPortalVolume* Portal);

    /** Unregister a portal volume */
    void UnregisterPortal(AAcousticPortalVolume* Portal);

    /** Get current zone for a location */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Engine")
    AAcousticZoneVolume* GetZoneAtLocation(const FVector& Location) const;

    /** Get current zone preset for listener */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Engine")
    FAcousticZonePreset GetCurrentZonePreset(int32 ListenerIndex = 0) const;

    // ========================================================================
    // AUDIO MODE
    // ========================================================================

    /** Set audio output mode */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Engine")
    void SetAudioOutputMode(EAudioOutputMode NewMode);

    /** Get current audio output mode */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Engine")
    EAudioOutputMode GetAudioOutputMode() const { return CurrentOutputMode; }

    // ========================================================================
    // MATERIAL SYSTEM
    // ========================================================================

    /** Get acoustic material for a physical material */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Engine")
    FAcousticMaterial GetAcousticMaterial(UPhysicalMaterial* PhysMat) const;

    /** Register custom material mapping */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Engine")
    void RegisterMaterialMapping(FName PhysMatName, const FAcousticMaterial& AcousticMat);

    // ========================================================================
    // RUNTIME QUERIES
    // ========================================================================

    /** Perform a single occlusion trace */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Engine|Queries")
    float TraceOcclusion(const FVector& Start, const FVector& End, FAcousticRayHit& OutHit) const;

    /** Perform reflection sampling from a point */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Engine|Queries")
    void SampleReflections(const FVector& Origin, const FVector& Forward, int32 NumRays, TArray<FAcousticRayHit>& OutHits) const;

    // ========================================================================
    // STATS & DEBUG
    // ========================================================================

    /** Get current frame ray budget usage */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Engine|Debug")
    FRayBudgetAllocation GetRayBudgetUsage() const { return CurrentBudget; }

    /** Get number of registered sources */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Engine|Debug")
    int32 GetNumRegisteredSources() const { return RegisteredSources.Num(); }

    /** Get number of active (audible) sources */
    UFUNCTION(BlueprintCallable, Category = "Acoustic Engine|Debug")
    int32 GetNumActiveSources() const;

    // ========================================================================
    // EVENTS
    // ========================================================================

    /** Broadcast when acoustic params update for any source */
    UPROPERTY(BlueprintAssignable, Category = "Acoustic Engine|Events")
    FOnAcousticParamsUpdated OnAcousticParamsUpdated;

    /** Broadcast when listener changes zone */
    UPROPERTY(BlueprintAssignable, Category = "Acoustic Engine|Events")
    FOnAcousticZoneChanged OnAcousticZoneChanged;

protected:
    // ========================================================================
    // INTERNAL PROCESSING
    // ========================================================================

    /** Main update loop */
    void ProcessAcousticUpdate(float DeltaTime);

    /** Update priorities and allocate ray budget */
    void UpdateSourcePriorities();

    /** Process occlusion for sources */
    void ProcessOcclusion(float DeltaTime);

    /** Process reflections for sources */
    void ProcessReflections(float DeltaTime);

    /** Update zone detection for listeners */
    void UpdateListenerZones();

    /** Apply computed params to audio components */
    void ApplyParamsToSources();

    /** Cluster reflection hits into taps */
    void ClusterReflections(const TArray<FAcousticRayHit>& Hits, FEarlyReflectionParams& OutParams);

    /** Compute occlusion factor from ray hit */
    float ComputeOcclusionFactor(const FAcousticRayHit& Hit) const;

    /** Compute LPF cutoff from occlusion */
    float ComputeLPFFromOcclusion(float OcclusionFactor, const FAcousticMaterial& Material) const;

    /** Generate hemisphere ray directions */
    void GenerateHemisphereRays(const FVector& Normal, int32 NumRays, TArray<FVector>& OutDirections) const;

    // ========================================================================
    // DATA
    // ========================================================================

    /** Registered acoustic sources */
    UPROPERTY()
    TMap<int32, FAcousticSourceEntry> RegisteredSources;

    /** Listener data array */
    UPROPERTY()
    TArray<FAcousticListenerData> ListenerDataArray;

    /** Registered zones */
    UPROPERTY()
    TArray<TWeakObjectPtr<AAcousticZoneVolume>> RegisteredZones;

    /** Registered portals */
    UPROPERTY()
    TArray<TWeakObjectPtr<AAcousticPortalVolume>> RegisteredPortals;

    /** Material mappings */
    UPROPERTY()
    TMap<FName, FAcousticMaterial> MaterialMappings;

    /** Current audio output mode */
    UPROPERTY()
    EAudioOutputMode CurrentOutputMode = EAudioOutputMode::Speakers;

    /** Active acoustic profile */
    UPROPERTY()
    UAcousticProfileAsset* ActiveProfile = nullptr;

    /** Current frame ray budget */
    FRayBudgetAllocation CurrentBudget;

    /** Next source ID */
    int32 NextSourceId = 1;

    /** Accumulated time for occlusion updates */
    float OcclusionUpdateAccumulator = 0.0f;

    /** Accumulated time for reflection updates */
    float ReflectionUpdateAccumulator = 0.0f;

    /** Accumulated time for zone updates */
    float ZoneUpdateAccumulator = 0.0f;

    /** Settings reference */
    UPROPERTY()
    UAcousticSettings* Settings = nullptr;

    /** Tick delegate handle */
    FDelegateHandle TickDelegateHandle;

    /** Is system initialized */
    bool bIsInitialized = false;
};
