// Copyright AcoustiTrace Pro. All Rights Reserved.

#include "AcousticSourceComponent.h"
#include "AcousticEngineSubsystem.h"
#include "AcousticSettings.h"
#include "AcousticEngineModule.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

UAcousticSourceComponent::UAcousticSourceComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.TickInterval = 0.0f; // Tick every frame

    bAutoActivate = true;

    // Default settings
    AcousticLOD = EAcousticLOD::Advanced;
    Importance = EAcousticImportance::Normal;
    SourceFlags = 0;
    BaseLoudness = 1.0f;
    PriorityOverride = -1.0f;
    BaseSpatialWidth = 0.0f;
    SourceRadius = 100.0f;
    ReverbSendOverride = -1.0f;
    bAutoCreateAudioComponent = false;
    bAutoPlay = false;
}

// ============================================================================
// COMPONENT LIFECYCLE
// ============================================================================

void UAcousticSourceComponent::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogAcousticEngine, Verbose, TEXT("AcousticSourceComponent BeginPlay: %s"),
        *GetOwner()->GetName());

    // Create audio component if needed
    CreateAudioComponentIfNeeded();

    // Find linked audio component
    FindLinkedAudioComponent();

    // Register with the engine
    RegisterWithEngine();

    // Auto-play if configured
    if (bAutoPlay && LinkedAudioComponent && Sound)
    {
        LinkedAudioComponent->SetSound(Sound);
        LinkedAudioComponent->Play();
    }
}

void UAcousticSourceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Unregister from the engine
    UnregisterFromEngine();

    Super::EndPlay(EndPlayReason);
}

void UAcousticSourceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Apply current params to audio if we have valid data
    if (bIsRegistered && CurrentParams.bIsValid)
    {
        ApplyParamsToAudio();
    }
}

#if WITH_EDITOR
void UAcousticSourceComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // Refresh linked audio component if changed
    FName PropertyName = PropertyChangedEvent.GetPropertyName();
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UAcousticSourceComponent, LinkedAudioComponent) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UAcousticSourceComponent, bAutoCreateAudioComponent))
    {
        FindLinkedAudioComponent();
    }
}
#endif

// ============================================================================
// REGISTRATION
// ============================================================================

void UAcousticSourceComponent::RegisterWithEngine()
{
    if (bIsRegistered)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    CachedSubsystem = World->GetSubsystem<UAcousticEngineSubsystem>();
    if (CachedSubsystem)
    {
        SourceId = CachedSubsystem->RegisterSource(this);
        bIsRegistered = (SourceId >= 0);

        if (bIsRegistered)
        {
            UE_LOG(LogAcousticEngine, Verbose, TEXT("Registered source %d: %s"),
                SourceId, *GetOwner()->GetName());
        }
    }
}

void UAcousticSourceComponent::UnregisterFromEngine()
{
    if (!bIsRegistered)
    {
        return;
    }

    if (CachedSubsystem)
    {
        CachedSubsystem->UnregisterSource(SourceId);
        UE_LOG(LogAcousticEngine, Verbose, TEXT("Unregistered source %d"), SourceId);
    }

    SourceId = -1;
    bIsRegistered = false;
    CachedSubsystem = nullptr;
}

// ============================================================================
// AUDIO COMPONENT MANAGEMENT
// ============================================================================

void UAcousticSourceComponent::FindLinkedAudioComponent()
{
    // If already linked, check if still valid
    if (LinkedAudioComponent && IsValid(LinkedAudioComponent))
    {
        return;
    }

    LinkedAudioComponent = nullptr;

    // Try to find an audio component on the same actor
    AActor* Owner = GetOwner();
    if (Owner)
    {
        LinkedAudioComponent = Owner->FindComponentByClass<UAudioComponent>();
    }
}

void UAcousticSourceComponent::CreateAudioComponentIfNeeded()
{
    if (!bAutoCreateAudioComponent || LinkedAudioComponent)
    {
        return;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    // Create a new audio component
    LinkedAudioComponent = NewObject<UAudioComponent>(Owner);
    if (LinkedAudioComponent)
    {
        LinkedAudioComponent->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
        LinkedAudioComponent->RegisterComponent();
        LinkedAudioComponent->bAutoActivate = false;

        if (Sound)
        {
            LinkedAudioComponent->SetSound(Sound);
        }

        UE_LOG(LogAcousticEngine, Verbose, TEXT("Created audio component for acoustic source: %s"),
            *Owner->GetName());
    }
}

void UAcousticSourceComponent::ApplyParamsToAudio()
{
    if (!LinkedAudioComponent || !CurrentParams.bIsValid)
    {
        return;
    }

    // Calculate effective values
    float EffectiveReverbSend = ReverbSendOverride >= 0.0f ?
        ReverbSendOverride : CurrentParams.ReverbSend;

    float EffectiveSpatialWidth = BaseSpatialWidth;
    if (HasFlag(EAcousticSourceFlags::LargeSource))
    {
        EffectiveSpatialWidth = FMath::Max(EffectiveSpatialWidth, 0.5f);
    }

    // Strong occlusion reduces spatial width
    if (CurrentParams.Occlusion > 0.5f)
    {
        EffectiveSpatialWidth = FMath::Lerp(EffectiveSpatialWidth, 0.0f,
            (CurrentParams.Occlusion - 0.5f) * 2.0f);
    }

    // Apply to audio component via instance parameters
    // These names match what MetaSound graphs expect

    // Set occlusion
    LinkedAudioComponent->SetFloatParameter(GetOcclusionParamName(), CurrentParams.Occlusion);

    // Set LPF cutoff
    LinkedAudioComponent->SetFloatParameter(GetLPFCutoffParamName(), CurrentParams.LowPassCutoff);

    // Set reverb send
    LinkedAudioComponent->SetFloatParameter(GetReverbSendParamName(), EffectiveReverbSend);

    // Set spatial width
    LinkedAudioComponent->SetFloatParameter(GetSpatialWidthParamName(),
        EffectiveSpatialWidth + CurrentParams.SpatialWidth);

    // Apply volume based on occlusion (direct attenuation)
    if (!HasFlag(EAcousticSourceFlags::NeverOcclude))
    {
        float VolumeMultiplier = CurrentParams.TransmissionGain;
        if (const UAcousticSettings* Settings = UAcousticSettings::Get())
        {
            // Apply minimum audibility
            float MinGain = FMath::Pow(10.0f, Settings->MinimumAudibilityDb / 20.0f);
            VolumeMultiplier = FMath::Max(VolumeMultiplier, MinGain);
        }
        LinkedAudioComponent->SetVolumeMultiplier(VolumeMultiplier);
    }

    // Apply LPF to the audio component's native filter if available
    // Note: This depends on the audio component's implementation
    LinkedAudioComponent->SetLowPassFilterEnabled(true);
    LinkedAudioComponent->SetLowPassFilterFrequency(CurrentParams.LowPassCutoff);
}

// ============================================================================
// BLUEPRINT CALLABLE
// ============================================================================

void UAcousticSourceComponent::SetAcousticLOD(EAcousticLOD NewLOD)
{
    AcousticLOD = NewLOD;
}

void UAcousticSourceComponent::SetImportance(EAcousticImportance NewImportance)
{
    Importance = NewImportance;
}

bool UAcousticSourceComponent::HasFlag(EAcousticSourceFlags Flag) const
{
    return (SourceFlags & static_cast<uint8>(Flag)) != 0;
}

void UAcousticSourceComponent::SetFlag(EAcousticSourceFlags Flag, bool bEnabled)
{
    if (bEnabled)
    {
        SourceFlags |= static_cast<uint8>(Flag);
    }
    else
    {
        SourceFlags &= ~static_cast<uint8>(Flag);
    }
}

void UAcousticSourceComponent::ForceUpdate()
{
    if (bIsRegistered && CachedSubsystem)
    {
        CachedSubsystem->ForceSourceUpdate(SourceId);
    }
}

bool UAcousticSourceComponent::IsAudible() const
{
    if (!bIsRegistered)
    {
        return false;
    }

    // Check if effectively off
    if (EffectiveLOD == EAcousticLOD::Off)
    {
        return false;
    }

    // Always audible flag
    if (HasFlag(EAcousticSourceFlags::AlwaysAudible))
    {
        return true;
    }

    // Check distance to listener
    if (CachedSubsystem)
    {
        FAcousticListenerData Listener = CachedSubsystem->GetListenerData(0);
        float Distance = FVector::Dist(GetAcousticLocation(), Listener.Location);

        if (const UAcousticSettings* Settings = UAcousticSettings::Get())
        {
            return Distance < Settings->OffLODDistance;
        }
    }

    return true;
}

// ============================================================================
// INTERNAL
// ============================================================================

void UAcousticSourceComponent::OnParamsUpdated(const FAcousticSourceParams& NewParams)
{
    CurrentParams = NewParams;
}

float UAcousticSourceComponent::ComputePriorityScore(const FVector& ListenerLocation) const
{
    // Custom priority override
    if (PriorityOverride >= 0.0f)
    {
        return PriorityOverride;
    }

    float Distance = FMath::Max(FVector::Dist(GetAcousticLocation(), ListenerLocation), 1.0f);

    // Base priority from loudness and distance
    float DistanceFactor = 1.0f / (Distance / 100.0f); // Normalize to meters
    float Priority = BaseLoudness * DistanceFactor;

    // Apply importance multiplier
    int32 ImportanceIndex = static_cast<int32>(Importance);
    if (ImportanceIndex >= 0 && ImportanceIndex < 4)
    {
        static const float ImportanceMultipliers[] = { 0.25f, 1.0f, 2.0f, 10.0f };
        Priority *= ImportanceMultipliers[ImportanceIndex];
    }

    // Hero sources get massive boost
    if (HasFlag(EAcousticSourceFlags::IsHero))
    {
        Priority *= 100.0f;
    }

    // Critical importance gets infinite priority
    if (Importance == EAcousticImportance::Critical)
    {
        Priority = FLT_MAX;
    }

    return Priority;
}

FVector UAcousticSourceComponent::GetAcousticLocation() const
{
    return GetComponentLocation();
}

// ============================================================================
// BLUEPRINT FUNCTION LIBRARY
// ============================================================================

UAcousticEngineSubsystem* UAcousticBlueprintLibrary::GetAcousticEngine(UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if (!World)
    {
        return nullptr;
    }

    return World->GetSubsystem<UAcousticEngineSubsystem>();
}

void UAcousticBlueprintLibrary::SetAudioOutputMode(UObject* WorldContextObject, EAudioOutputMode Mode)
{
    if (UAcousticEngineSubsystem* Subsystem = GetAcousticEngine(WorldContextObject))
    {
        Subsystem->SetAudioOutputMode(Mode);
    }
}

EAudioOutputMode UAcousticBlueprintLibrary::GetAudioOutputMode(UObject* WorldContextObject)
{
    if (UAcousticEngineSubsystem* Subsystem = GetAcousticEngine(WorldContextObject))
    {
        return Subsystem->GetAudioOutputMode();
    }
    return EAudioOutputMode::Speakers;
}

float UAcousticBlueprintLibrary::TraceOcclusion(UObject* WorldContextObject, FVector Start, FVector End)
{
    if (UAcousticEngineSubsystem* Subsystem = GetAcousticEngine(WorldContextObject))
    {
        FAcousticRayHit Hit;
        return Subsystem->TraceOcclusion(Start, End, Hit);
    }
    return 0.0f;
}

FAcousticMaterial UAcousticBlueprintLibrary::GetSurfaceMaterial(UObject* WorldContextObject, FVector Location, FVector Direction)
{
    FAcousticMaterial DefaultMaterial;

    if (!WorldContextObject)
    {
        return DefaultMaterial;
    }

    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if (!World)
    {
        return DefaultMaterial;
    }

    // Trace to find surface
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.bReturnPhysicalMaterial = true;

    bool bHit = World->LineTraceSingleByChannel(
        HitResult,
        Location,
        Location + Direction * 10000.0f,
        ECC_Visibility,
        QueryParams
    );

    if (bHit)
    {
        if (UAcousticEngineSubsystem* Subsystem = World->GetSubsystem<UAcousticEngineSubsystem>())
        {
            return Subsystem->GetAcousticMaterial(HitResult.PhysMaterial.Get());
        }
    }

    return DefaultMaterial;
}
