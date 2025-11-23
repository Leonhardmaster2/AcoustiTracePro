// Copyright AcoustiTrace Pro. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "AcousticTypes.h"
#include "AcousticMultiplayer.generated.h"

class UAcousticSourceComponent;
class AAcousticPortalVolume;

// ============================================================================
// MULTIPLAYER DESIGN PRINCIPLES
// ============================================================================
/**
 * Acoustic Multiplayer Architecture
 *
 * KEY RULE: Server replicates events and transforms, clients do ALL acoustic work locally.
 *
 * What IS Replicated:
 * - Actor transforms (position, rotation)
 * - Actor velocity (for Doppler)
 * - Audio state (playing/stopped, loop/one-shot)
 * - Audio type data (BaseLoudness, AcousticLOD, IsHeroSource)
 * - Portal/door states (open/closed)
 * - Zone state changes (cave collapses, etc.)
 *
 * What is NOT Replicated:
 * - Ray trace results
 * - Filter parameters
 * - Reflection data
 * - Any computed acoustic parameters
 *
 * Each client:
 * - Maintains its own listener position (camera)
 * - Runs listener-source raytraces
 * - Performs acoustic LOD selection
 * - Updates parameters into MetaSounds/submixes
 *
 * Result: Each player gets their own correct headphone mix from their POV.
 */

// ============================================================================
// REPLICATED AUDIO STATE
// ============================================================================

/**
 * Minimal replicated state for an acoustic source
 */
USTRUCT(BlueprintType)
struct ACOUSTICENGINE_API FReplicatedAcousticState
{
    GENERATED_BODY()

    /** Is the sound currently playing */
    UPROPERTY()
    bool bIsPlaying = false;

    /** Is this a looping sound */
    UPROPERTY()
    bool bIsLooping = false;

    /** Base loudness (affects priority) */
    UPROPERTY()
    float BaseLoudness = 1.0f;

    /** Acoustic LOD setting */
    UPROPERTY()
    EAcousticLOD AcousticLOD = EAcousticLOD::Basic;

    /** Is this a hero source */
    UPROPERTY()
    bool bIsHeroSource = false;

    /** Importance level */
    UPROPERTY()
    EAcousticImportance Importance = EAcousticImportance::Normal;

    /** Current playback position (for seamless handoff) */
    UPROPERTY()
    float PlaybackPosition = 0.0f;
};

/**
 * Replicated portal state
 */
USTRUCT(BlueprintType)
struct ACOUSTICENGINE_API FReplicatedPortalState
{
    GENERATED_BODY()

    /** Portal identifier */
    UPROPERTY()
    int32 PortalId = -1;

    /** Is the portal open */
    UPROPERTY()
    bool bIsOpen = true;

    /** Current openness (0-1) */
    UPROPERTY()
    float Openness = 1.0f;
};

// ============================================================================
// ACOUSTIC REPLICATION COMPONENT
// ============================================================================

/**
 * Acoustic Replication Component
 *
 * Attach to any actor with acoustic sources to handle multiplayer
 * state synchronization. This component manages:
 * - Replication of audio play/stop states
 * - Synchronization of acoustic LOD changes
 * - Portal state broadcasting
 *
 * Usage:
 * - Add to actors that have UAcousticSourceComponent
 * - Automatically detects and manages child acoustic sources
 * - Server authoritative - clients receive state updates
 */
UCLASS(ClassGroup = (Audio), meta = (BlueprintSpawnableComponent, DisplayName = "Acoustic Replicator"))
class ACOUSTICENGINE_API UAcousticReplicationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAcousticReplicationComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ========================================================================
    // REPLICATED STATE
    // ========================================================================

    /** Replicated audio state */
    UPROPERTY(ReplicatedUsing = OnRep_AudioState)
    FReplicatedAcousticState ReplicatedState;

    /** Rep notify for audio state changes */
    UFUNCTION()
    void OnRep_AudioState();

    // ========================================================================
    // SERVER FUNCTIONS
    // ========================================================================

    /** Server: Start playing the sound */
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Acoustic|Multiplayer")
    void Server_PlaySound();

    /** Server: Stop playing the sound */
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Acoustic|Multiplayer")
    void Server_StopSound();

    /** Server: Update acoustic LOD */
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Acoustic|Multiplayer")
    void Server_SetAcousticLOD(EAcousticLOD NewLOD);

    /** Server: Mark as hero source */
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Acoustic|Multiplayer")
    void Server_SetHeroSource(bool bIsHero);

    // ========================================================================
    // CLIENT FUNCTIONS
    // ========================================================================

    /** Multicast: Broadcast one-shot sound event */
    UFUNCTION(BlueprintCallable, NetMulticast, Unreliable, Category = "Acoustic|Multiplayer")
    void Multicast_PlayOneShotSound();

    /** Client: Apply replicated state locally */
    UFUNCTION(BlueprintCallable, Category = "Acoustic|Multiplayer")
    void ApplyReplicatedState();

    // ========================================================================
    // LOCAL QUERIES
    // ========================================================================

    /** Get the linked acoustic source */
    UFUNCTION(BlueprintCallable, Category = "Acoustic|Multiplayer")
    UAcousticSourceComponent* GetAcousticSource() const { return LinkedSource; }

    /** Is this the local player's source */
    UFUNCTION(BlueprintCallable, Category = "Acoustic|Multiplayer")
    bool IsLocallyControlled() const;

protected:
    /** Linked acoustic source component */
    UPROPERTY()
    UAcousticSourceComponent* LinkedSource = nullptr;

    /** Find linked acoustic source */
    void FindLinkedSource();

    /** Update replicated state from source */
    void UpdateReplicatedStateFromSource();
};

// ============================================================================
// PORTAL REPLICATION COMPONENT
// ============================================================================

/**
 * Portal Replication Component
 *
 * Handles replication of portal state changes (doors, windows, etc.)
 * for multiplayer synchronization.
 */
UCLASS(ClassGroup = (Audio), meta = (BlueprintSpawnableComponent, DisplayName = "Acoustic Portal Replicator"))
class ACOUSTICENGINE_API UAcousticPortalReplicationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAcousticPortalReplicationComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ========================================================================
    // REPLICATED STATE
    // ========================================================================

    /** Is the portal open */
    UPROPERTY(ReplicatedUsing = OnRep_PortalState)
    bool bIsOpen = true;

    /** Current openness (0-1) */
    UPROPERTY(ReplicatedUsing = OnRep_PortalState)
    float Openness = 1.0f;

    /** Rep notify for portal state */
    UFUNCTION()
    void OnRep_PortalState();

    // ========================================================================
    // SERVER FUNCTIONS
    // ========================================================================

    /** Server: Set portal open state */
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Acoustic|Portal")
    void Server_SetOpen(bool bOpen);

    /** Server: Set portal openness directly */
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Acoustic|Portal")
    void Server_SetOpenness(float NewOpenness);

protected:
    /** Linked portal volume */
    UPROPERTY()
    AAcousticPortalVolume* LinkedPortal = nullptr;

    /** Find linked portal */
    void FindLinkedPortal();

    /** Apply state to portal */
    void ApplyStateToPortal();
};

// ============================================================================
// MULTIPLAYER LISTENER MANAGER
// ============================================================================

/**
 * Multiplayer Listener Manager
 *
 * Manages listener state in multiplayer contexts.
 * Each client has its own listener at their camera/pawn location.
 *
 * Features:
 * - Automatic listener registration per local player
 * - Handles split-screen with multiple local listeners
 * - Coordinates with AcousticEngineSubsystem
 */
UCLASS(ClassGroup = (Audio), meta = (BlueprintSpawnableComponent, DisplayName = "Acoustic Listener Manager"))
class ACOUSTICENGINE_API UAcousticListenerManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAcousticListenerManagerComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /** Use camera location (true) or actor location (false) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Listener")
    bool bUseCameraLocation = true;

    /** Listener index for this component */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Listener")
    int32 ListenerIndex = 0;

    /** Update rate for listener position (Hz) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Listener", meta = (ClampMin = "10.0", ClampMax = "120.0"))
    float UpdateRateHz = 60.0f;

    // ========================================================================
    // METHODS
    // ========================================================================

    /** Force update listener position */
    UFUNCTION(BlueprintCallable, Category = "Listener")
    void ForceUpdateListener();

    /** Get current listener data */
    UFUNCTION(BlueprintCallable, Category = "Listener")
    FAcousticListenerData GetListenerData() const;

    /** Check if this is the primary listener */
    UFUNCTION(BlueprintCallable, Category = "Listener")
    bool IsPrimaryListener() const { return ListenerIndex == 0; }

protected:
    /** Update listener position in acoustic engine */
    void UpdateListenerPosition();

    /** Calculate listener transform */
    void CalculateListenerTransform(FVector& OutLocation, FRotator& OutRotation) const;

    /** Previous listener location for velocity calculation */
    FVector PreviousLocation = FVector::ZeroVector;

    /** Update timer */
    float UpdateTimer = 0.0f;

    /** Is registered with acoustic engine */
    bool bIsRegistered = false;
};

// ============================================================================
// NETWORK AUDIO EVENT
// ============================================================================

/**
 * Network Audio Event
 *
 * Struct for sending audio events over the network.
 * Used for one-shot sounds that need network synchronization.
 */
USTRUCT(BlueprintType)
struct ACOUSTICENGINE_API FNetworkAudioEvent
{
    GENERATED_BODY()

    /** Location of the sound */
    UPROPERTY()
    FVector Location = FVector::ZeroVector;

    /** Sound asset reference */
    UPROPERTY()
    TSoftObjectPtr<USoundBase> Sound;

    /** Volume multiplier */
    UPROPERTY()
    float VolumeMultiplier = 1.0f;

    /** Pitch multiplier */
    UPROPERTY()
    float PitchMultiplier = 1.0f;

    /** Acoustic LOD for this event */
    UPROPERTY()
    EAcousticLOD AcousticLOD = EAcousticLOD::Basic;

    /** Is this a hero sound */
    UPROPERTY()
    bool bIsHeroSound = false;

    /** Timestamp for latency compensation */
    UPROPERTY()
    float ServerTimestamp = 0.0f;
};

/**
 * Network Audio Manager
 *
 * Handles network synchronization of audio events.
 * Provides utilities for playing networked sounds with
 * proper acoustic processing on each client.
 */
UCLASS()
class ACOUSTICENGINE_API UAcousticNetworkManager : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Play a one-shot sound at location, replicated to all clients
     * Each client will compute their own acoustic parameters
     */
    UFUNCTION(BlueprintCallable, Category = "Acoustic|Network", meta = (WorldContext = "WorldContextObject"))
    static void PlaySoundAtLocationReplicated(
        UObject* WorldContextObject,
        USoundBase* Sound,
        FVector Location,
        float VolumeMultiplier = 1.0f,
        float PitchMultiplier = 1.0f,
        EAcousticLOD AcousticLOD = EAcousticLOD::Basic
    );

    /**
     * Play a one-shot sound attached to actor, replicated to all clients
     */
    UFUNCTION(BlueprintCallable, Category = "Acoustic|Network", meta = (WorldContext = "WorldContextObject"))
    static void PlaySoundAttachedReplicated(
        UObject* WorldContextObject,
        USoundBase* Sound,
        AActor* AttachToActor,
        FName AttachPointName = NAME_None,
        float VolumeMultiplier = 1.0f,
        float PitchMultiplier = 1.0f,
        EAcousticLOD AcousticLOD = EAcousticLOD::Basic
    );

    /**
     * Check if we should process audio locally
     * (Always true - clients always do their own audio processing)
     */
    UFUNCTION(BlueprintCallable, Category = "Acoustic|Network", meta = (WorldContext = "WorldContextObject"))
    static bool ShouldProcessAudioLocally(UObject* WorldContextObject);
};
