// Copyright AcoustiTrace Pro. All Rights Reserved.

#include "AcousticMultiplayer.h"
#include "AcousticSourceComponent.h"
#include "AcousticZoneVolume.h"
#include "AcousticEngineSubsystem.h"
#include "AcousticEngineModule.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

// ============================================================================
// ACOUSTIC REPLICATION COMPONENT
// ============================================================================

UAcousticReplicationComponent::UAcousticReplicationComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.TickInterval = 0.1f; // 10 Hz update rate
}

void UAcousticReplicationComponent::BeginPlay()
{
    Super::BeginPlay();

    // Find linked acoustic source
    FindLinkedSource();

    // Initialize replicated state from source
    if (GetOwner()->HasAuthority() && LinkedSource)
    {
        UpdateReplicatedStateFromSource();
    }
}

void UAcousticReplicationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Server updates replicated state
    if (GetOwner()->HasAuthority() && LinkedSource)
    {
        UpdateReplicatedStateFromSource();
    }
}

void UAcousticReplicationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UAcousticReplicationComponent, ReplicatedState);
}

void UAcousticReplicationComponent::OnRep_AudioState()
{
    // Apply replicated state locally
    ApplyReplicatedState();
}

void UAcousticReplicationComponent::Server_PlaySound_Implementation()
{
    ReplicatedState.bIsPlaying = true;
    ReplicatedState.PlaybackPosition = 0.0f;

    // Locally controlled sound starts here
    if (LinkedSource && LinkedSource->LinkedAudioComponent)
    {
        LinkedSource->LinkedAudioComponent->Play();
    }
}

void UAcousticReplicationComponent::Server_StopSound_Implementation()
{
    ReplicatedState.bIsPlaying = false;

    if (LinkedSource && LinkedSource->LinkedAudioComponent)
    {
        LinkedSource->LinkedAudioComponent->Stop();
    }
}

void UAcousticReplicationComponent::Server_SetAcousticLOD_Implementation(EAcousticLOD NewLOD)
{
    ReplicatedState.AcousticLOD = NewLOD;

    if (LinkedSource)
    {
        LinkedSource->SetAcousticLOD(NewLOD);
    }
}

void UAcousticReplicationComponent::Server_SetHeroSource_Implementation(bool bIsHero)
{
    ReplicatedState.bIsHeroSource = bIsHero;

    if (LinkedSource)
    {
        LinkedSource->SetFlag(EAcousticSourceFlags::IsHero, bIsHero);
    }
}

void UAcousticReplicationComponent::Multicast_PlayOneShotSound_Implementation()
{
    // Each client plays the sound locally and computes their own acoustics
    if (LinkedSource && LinkedSource->LinkedAudioComponent)
    {
        LinkedSource->LinkedAudioComponent->Play();
    }
}

void UAcousticReplicationComponent::ApplyReplicatedState()
{
    if (!LinkedSource)
    {
        FindLinkedSource();
    }

    if (!LinkedSource)
    {
        return;
    }

    // Apply LOD
    LinkedSource->SetAcousticLOD(ReplicatedState.AcousticLOD);

    // Apply hero flag
    LinkedSource->SetFlag(EAcousticSourceFlags::IsHero, ReplicatedState.bIsHeroSource);

    // Apply importance
    LinkedSource->SetImportance(ReplicatedState.Importance);

    // Apply base loudness
    LinkedSource->BaseLoudness = ReplicatedState.BaseLoudness;

    // Apply playback state
    if (LinkedSource->LinkedAudioComponent)
    {
        if (ReplicatedState.bIsPlaying && !LinkedSource->LinkedAudioComponent->IsPlaying())
        {
            LinkedSource->LinkedAudioComponent->Play();
        }
        else if (!ReplicatedState.bIsPlaying && LinkedSource->LinkedAudioComponent->IsPlaying())
        {
            LinkedSource->LinkedAudioComponent->Stop();
        }
    }
}

bool UAcousticReplicationComponent::IsLocallyControlled() const
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return false;
    }

    // Check if owned by local player
    if (APawn* Pawn = Cast<APawn>(Owner))
    {
        return Pawn->IsLocallyControlled();
    }

    // Check owner chain
    AActor* TopOwner = Owner->GetOwner();
    while (TopOwner)
    {
        if (APawn* Pawn = Cast<APawn>(TopOwner))
        {
            return Pawn->IsLocallyControlled();
        }
        TopOwner = TopOwner->GetOwner();
    }

    return GetOwner()->HasAuthority();
}

void UAcousticReplicationComponent::FindLinkedSource()
{
    if (LinkedSource)
    {
        return;
    }

    AActor* Owner = GetOwner();
    if (Owner)
    {
        LinkedSource = Owner->FindComponentByClass<UAcousticSourceComponent>();
    }
}

void UAcousticReplicationComponent::UpdateReplicatedStateFromSource()
{
    if (!LinkedSource)
    {
        return;
    }

    ReplicatedState.AcousticLOD = LinkedSource->AcousticLOD;
    ReplicatedState.Importance = LinkedSource->Importance;
    ReplicatedState.BaseLoudness = LinkedSource->BaseLoudness;
    ReplicatedState.bIsHeroSource = LinkedSource->HasFlag(EAcousticSourceFlags::IsHero);

    if (LinkedSource->LinkedAudioComponent)
    {
        ReplicatedState.bIsPlaying = LinkedSource->LinkedAudioComponent->IsPlaying();
    }
}

// ============================================================================
// ACOUSTIC PORTAL REPLICATION COMPONENT
// ============================================================================

UAcousticPortalReplicationComponent::UAcousticPortalReplicationComponent()
{
    SetIsReplicatedByDefault(true);
}

void UAcousticPortalReplicationComponent::BeginPlay()
{
    Super::BeginPlay();
    FindLinkedPortal();
}

void UAcousticPortalReplicationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UAcousticPortalReplicationComponent, bIsOpen);
    DOREPLIFETIME(UAcousticPortalReplicationComponent, Openness);
}

void UAcousticPortalReplicationComponent::OnRep_PortalState()
{
    ApplyStateToPortal();
}

void UAcousticPortalReplicationComponent::Server_SetOpen_Implementation(bool bOpen)
{
    bIsOpen = bOpen;
    Openness = bOpen ? 1.0f : 0.0f;
    ApplyStateToPortal();
}

void UAcousticPortalReplicationComponent::Server_SetOpenness_Implementation(float NewOpenness)
{
    Openness = FMath::Clamp(NewOpenness, 0.0f, 1.0f);
    bIsOpen = Openness > 0.5f;
    ApplyStateToPortal();
}

void UAcousticPortalReplicationComponent::FindLinkedPortal()
{
    AActor* Owner = GetOwner();
    if (Owner)
    {
        LinkedPortal = Cast<AAcousticPortalVolume>(Owner);
    }
}

void UAcousticPortalReplicationComponent::ApplyStateToPortal()
{
    if (!LinkedPortal)
    {
        FindLinkedPortal();
    }

    if (LinkedPortal)
    {
        LinkedPortal->SetOpenness(Openness);
    }
}

// ============================================================================
// ACOUSTIC LISTENER MANAGER COMPONENT
// ============================================================================

UAcousticListenerManagerComponent::UAcousticListenerManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

    bUseCameraLocation = true;
    ListenerIndex = 0;
    UpdateRateHz = 60.0f;
}

void UAcousticListenerManagerComponent::BeginPlay()
{
    Super::BeginPlay();

    // Register listener with acoustic engine
    UWorld* World = GetWorld();
    if (World)
    {
        if (UAcousticEngineSubsystem* Subsystem = World->GetSubsystem<UAcousticEngineSubsystem>())
        {
            bIsRegistered = true;
            ForceUpdateListener();
        }
    }
}

void UAcousticListenerManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bIsRegistered = false;
    Super::EndPlay(EndPlayReason);
}

void UAcousticListenerManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Update at configured rate
    float UpdateInterval = 1.0f / UpdateRateHz;
    UpdateTimer += DeltaTime;

    if (UpdateTimer >= UpdateInterval)
    {
        UpdateListenerPosition();
        UpdateTimer = 0.0f;
    }
}

void UAcousticListenerManagerComponent::ForceUpdateListener()
{
    UpdateListenerPosition();
}

FAcousticListenerData UAcousticListenerManagerComponent::GetListenerData() const
{
    FAcousticListenerData Data;

    FVector Location;
    FRotator Rotation;
    CalculateListenerTransform(Location, Rotation);

    Data.Location = Location;
    Data.Forward = Rotation.Vector();
    Data.Right = FRotationMatrix(Rotation).GetScaledAxis(EAxis::Y);
    Data.Up = FRotationMatrix(Rotation).GetScaledAxis(EAxis::Z);
    Data.PlayerIndex = ListenerIndex;

    return Data;
}

void UAcousticListenerManagerComponent::UpdateListenerPosition()
{
    if (!bIsRegistered)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UAcousticEngineSubsystem* Subsystem = World->GetSubsystem<UAcousticEngineSubsystem>();
    if (!Subsystem)
    {
        return;
    }

    FVector Location;
    FRotator Rotation;
    CalculateListenerTransform(Location, Rotation);

    FAcousticListenerData ListenerData;
    ListenerData.Location = Location;
    ListenerData.Forward = Rotation.Vector();
    ListenerData.Right = FRotationMatrix(Rotation).GetScaledAxis(EAxis::Y);
    ListenerData.Up = FRotationMatrix(Rotation).GetScaledAxis(EAxis::Z);
    ListenerData.PlayerIndex = ListenerIndex;

    // Calculate velocity
    float DeltaTime = 1.0f / UpdateRateHz;
    if (DeltaTime > KINDA_SMALL_NUMBER && !PreviousLocation.IsZero())
    {
        ListenerData.Velocity = (Location - PreviousLocation) / DeltaTime;
    }
    PreviousLocation = Location;

    Subsystem->UpdateListener(ListenerIndex, ListenerData);
}

void UAcousticListenerManagerComponent::CalculateListenerTransform(FVector& OutLocation, FRotator& OutRotation) const
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        OutLocation = FVector::ZeroVector;
        OutRotation = FRotator::ZeroRotator;
        return;
    }

    if (bUseCameraLocation)
    {
        // Try to get camera view point from player controller
        if (APlayerController* PC = Cast<APlayerController>(Owner))
        {
            PC->GetPlayerViewPoint(OutLocation, OutRotation);
            return;
        }

        // Try from pawn's controller
        if (APawn* Pawn = Cast<APawn>(Owner))
        {
            if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
            {
                PC->GetPlayerViewPoint(OutLocation, OutRotation);
                return;
            }
        }
    }

    // Fallback to actor transform
    OutLocation = Owner->GetActorLocation();
    OutRotation = Owner->GetActorRotation();
}

// ============================================================================
// ACOUSTIC NETWORK MANAGER
// ============================================================================

void UAcousticNetworkManager::PlaySoundAtLocationReplicated(
    UObject* WorldContextObject,
    USoundBase* Sound,
    FVector Location,
    float VolumeMultiplier,
    float PitchMultiplier,
    EAcousticLOD AcousticLOD)
{
    if (!WorldContextObject || !Sound)
    {
        return;
    }

    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if (!World)
    {
        return;
    }

    // Play sound locally - acoustic processing happens automatically
    // via the acoustic engine subsystem

    UAudioComponent* AudioComp = UGameplayStatics::SpawnSoundAtLocation(
        WorldContextObject,
        Sound,
        Location,
        FRotator::ZeroRotator,
        VolumeMultiplier,
        PitchMultiplier
    );

    if (AudioComp)
    {
        // Set acoustic parameters via sound instance parameters
        // These will be picked up by the acoustic system
        AudioComp->SetFloatParameter(FName("Acoustic_LOD"), static_cast<float>(AcousticLOD));
    }
}

void UAcousticNetworkManager::PlaySoundAttachedReplicated(
    UObject* WorldContextObject,
    USoundBase* Sound,
    AActor* AttachToActor,
    FName AttachPointName,
    float VolumeMultiplier,
    float PitchMultiplier,
    EAcousticLOD AcousticLOD)
{
    if (!WorldContextObject || !Sound || !AttachToActor)
    {
        return;
    }

    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if (!World)
    {
        return;
    }

    // Spawn attached sound
    UAudioComponent* AudioComp = UGameplayStatics::SpawnSoundAttached(
        Sound,
        AttachToActor->GetRootComponent(),
        AttachPointName,
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        EAttachLocation::SnapToTarget,
        false, // bStopWhenAttachedToDestroyed
        VolumeMultiplier,
        PitchMultiplier
    );

    if (AudioComp)
    {
        AudioComp->SetFloatParameter(FName("Acoustic_LOD"), static_cast<float>(AcousticLOD));
        AudioComp->Play();
    }
}

bool UAcousticNetworkManager::ShouldProcessAudioLocally(UObject* WorldContextObject)
{
    // Audio is ALWAYS processed locally in the acoustic system
    // This is a core design principle - no acoustic data is replicated
    return true;
}
