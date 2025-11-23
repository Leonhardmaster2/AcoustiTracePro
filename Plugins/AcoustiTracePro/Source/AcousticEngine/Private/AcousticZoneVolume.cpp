// Copyright AcoustiTrace Pro. All Rights Reserved.

#include "AcousticZoneVolume.h"
#include "AcousticEngineSubsystem.h"
#include "AcousticEngineModule.h"
#include "Engine/World.h"
#include "Components/BrushComponent.h"
#include "Net/UnrealNetwork.h"

// ============================================================================
// ZONE PRESETS - Default values for each zone type
// ============================================================================

namespace ZonePresets
{
    void ApplyPreset(EAcousticZoneType Type, float& RT60, float& HFDecay, float& LFDecay,
        float& Density, float& Diffusion, float& EarlyReflectionLevel,
        float& LateReverbLevel, float& PreDelayMs, float& RoomSize, float& DefaultReverbSend)
    {
        switch (Type)
        {
            case EAcousticZoneType::SmallRoom:
                RT60 = 0.3f;
                HFDecay = 0.9f;
                LFDecay = 1.0f;
                Density = 0.7f;
                Diffusion = 0.6f;
                EarlyReflectionLevel = 1.2f;
                LateReverbLevel = 0.8f;
                PreDelayMs = 5.0f;
                RoomSize = 0.3f;
                DefaultReverbSend = 0.25f;
                break;

            case EAcousticZoneType::LargeRoom:
                RT60 = 0.8f;
                HFDecay = 0.8f;
                LFDecay = 1.0f;
                Density = 0.5f;
                Diffusion = 0.5f;
                EarlyReflectionLevel = 1.0f;
                LateReverbLevel = 1.0f;
                PreDelayMs = 15.0f;
                RoomSize = 1.0f;
                DefaultReverbSend = 0.35f;
                break;

            case EAcousticZoneType::Hallway:
                RT60 = 1.2f;
                HFDecay = 0.7f;
                LFDecay = 1.1f;
                Density = 0.3f;
                Diffusion = 0.3f;
                EarlyReflectionLevel = 1.5f;
                LateReverbLevel = 0.7f;
                PreDelayMs = 8.0f;
                RoomSize = 0.6f;
                DefaultReverbSend = 0.4f;
                break;

            case EAcousticZoneType::Cave:
                RT60 = 3.0f;
                HFDecay = 0.6f;
                LFDecay = 1.2f;
                Density = 0.8f;
                Diffusion = 0.7f;
                EarlyReflectionLevel = 1.3f;
                LateReverbLevel = 1.2f;
                PreDelayMs = 25.0f;
                RoomSize = 2.0f;
                DefaultReverbSend = 0.5f;
                break;

            case EAcousticZoneType::Cathedral:
                RT60 = 4.0f;
                HFDecay = 0.5f;
                LFDecay = 1.0f;
                Density = 0.6f;
                Diffusion = 0.8f;
                EarlyReflectionLevel = 0.8f;
                LateReverbLevel = 1.5f;
                PreDelayMs = 40.0f;
                RoomSize = 5.0f;
                DefaultReverbSend = 0.6f;
                break;

            case EAcousticZoneType::Forest:
                RT60 = 0.2f;
                HFDecay = 1.0f;
                LFDecay = 0.8f;
                Density = 0.2f;
                Diffusion = 0.9f;
                EarlyReflectionLevel = 0.5f;
                LateReverbLevel = 0.3f;
                PreDelayMs = 3.0f;
                RoomSize = 0.5f;
                DefaultReverbSend = 0.15f;
                break;

            case EAcousticZoneType::OpenAir:
                RT60 = 0.1f;
                HFDecay = 1.0f;
                LFDecay = 1.0f;
                Density = 0.1f;
                Diffusion = 0.5f;
                EarlyReflectionLevel = 0.2f;
                LateReverbLevel = 0.1f;
                PreDelayMs = 0.0f;
                RoomSize = 0.1f;
                DefaultReverbSend = 0.05f;
                break;

            case EAcousticZoneType::Underwater:
                RT60 = 0.5f;
                HFDecay = 0.3f;
                LFDecay = 1.5f;
                Density = 0.9f;
                Diffusion = 0.9f;
                EarlyReflectionLevel = 0.8f;
                LateReverbLevel = 1.0f;
                PreDelayMs = 10.0f;
                RoomSize = 1.0f;
                DefaultReverbSend = 0.7f;
                break;

            case EAcousticZoneType::Default:
            case EAcousticZoneType::Custom:
            default:
                RT60 = 1.0f;
                HFDecay = 1.0f;
                LFDecay = 1.0f;
                Density = 0.5f;
                Diffusion = 0.5f;
                EarlyReflectionLevel = 1.0f;
                LateReverbLevel = 1.0f;
                PreDelayMs = 10.0f;
                RoomSize = 1.0f;
                DefaultReverbSend = 0.3f;
                break;
        }
    }
}

// ============================================================================
// ACOUSTIC ZONE VOLUME
// ============================================================================

AAcousticZoneVolume::AAcousticZoneVolume()
{
    // Default settings
    ZoneName = FName("DefaultZone");
    ZoneType = EAcousticZoneType::Default;
    Priority = 0;
    BlendTime = 0.5f;
    bUseZoneTypePreset = true;

    // Default reverb settings
    RT60 = 1.0f;
    HFDecay = 1.0f;
    LFDecay = 1.0f;
    Density = 0.5f;
    Diffusion = 0.5f;
    EarlyReflectionLevel = 1.0f;
    LateReverbLevel = 1.0f;
    PreDelayMs = 10.0f;
    RoomSize = 1.0f;

    // Source behavior
    DefaultReverbSend = 0.3f;
    ReflectionDensityMod = 1.0f;
    TraceDistanceMod = 1.0f;

    // Zone ID
    static int32 NextZoneId = 1;
    ZoneId = NextZoneId++;
}

void AAcousticZoneVolume::BeginPlay()
{
    Super::BeginPlay();

    // Apply preset if enabled
    if (bUseZoneTypePreset)
    {
        ApplyZoneTypePreset();
    }

    // Cache the preset
    CachedPreset = GetZonePreset();

    // Register with the engine
    RegisterWithEngine();

    UE_LOG(LogAcousticEngine, Log, TEXT("Acoustic Zone '%s' activated (ID: %d, Type: %d)"),
        *ZoneName.ToString(), ZoneId, static_cast<int32>(ZoneType));
}

void AAcousticZoneVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnregisterFromEngine();
    Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void AAcousticZoneVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    FName PropertyName = PropertyChangedEvent.GetPropertyName();

    // Re-apply preset if zone type or use preset changed
    if (PropertyName == GET_MEMBER_NAME_CHECKED(AAcousticZoneVolume, ZoneType) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(AAcousticZoneVolume, bUseZoneTypePreset))
    {
        if (bUseZoneTypePreset)
        {
            ApplyZoneTypePreset();
        }
    }

    // Update cached preset
    CachedPreset = GetZonePreset();
}
#endif

void AAcousticZoneVolume::ApplyZoneTypePreset()
{
    ZonePresets::ApplyPreset(ZoneType, RT60, HFDecay, LFDecay, Density, Diffusion,
        EarlyReflectionLevel, LateReverbLevel, PreDelayMs, RoomSize, DefaultReverbSend);
}

FAcousticZonePreset AAcousticZoneVolume::GetZonePreset() const
{
    FAcousticZonePreset Preset;
    Preset.PresetName = ZoneName;
    Preset.ZoneType = ZoneType;
    Preset.RT60 = RT60;
    Preset.HFDecay = HFDecay;
    Preset.LFDecay = LFDecay;
    Preset.Density = Density;
    Preset.Diffusion = Diffusion;
    Preset.EarlyReflectionLevel = EarlyReflectionLevel;
    Preset.LateReverbLevel = LateReverbLevel;
    Preset.PreDelayMs = PreDelayMs;
    Preset.RoomSize = RoomSize;
    Preset.DefaultReverbSend = DefaultReverbSend;
    return Preset;
}

bool AAcousticZoneVolume::ContainsPoint(const FVector& Point) const
{
    // Use the brush component to check if point is inside
    if (UBrushComponent* BrushComp = GetBrushComponent())
    {
        return BrushComp->IsPointInside(Point);
    }

    // Fallback to bounding box check
    FVector Origin, Extent;
    GetActorBounds(false, Origin, Extent);
    FBox BoundingBox(Origin - Extent, Origin + Extent);
    return BoundingBox.IsInside(Point);
}

void AAcousticZoneVolume::RegisterWithEngine()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (UAcousticEngineSubsystem* Subsystem = World->GetSubsystem<UAcousticEngineSubsystem>())
    {
        Subsystem->RegisterZone(this);
    }
}

void AAcousticZoneVolume::UnregisterFromEngine()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (UAcousticEngineSubsystem* Subsystem = World->GetSubsystem<UAcousticEngineSubsystem>())
    {
        Subsystem->UnregisterZone(this);
    }
}

// ============================================================================
// ACOUSTIC PORTAL VOLUME
// ============================================================================

AAcousticPortalVolume::AAcousticPortalVolume()
{
    // Enable replication
    bReplicates = true;

    // Enable tick for transition animation
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    // Default settings
    PortalName = FName("DefaultPortal");
    bIsOpen = true;
    Openness = 1.0f;
    TargetOpenness = 1.0f;

    // Transmission properties
    ClosedTransmission = 0.1f;
    ClosedLFMultiplier = 1.5f;
    ClosedHFMultiplier = 0.3f;
    ClosedLPFCutoff = 800.0f;

    // Animation
    TransitionTime = 0.3f;

    // Portal ID
    static int32 NextPortalId = 1;
    PortalId = NextPortalId++;
}

void AAcousticPortalVolume::BeginPlay()
{
    Super::BeginPlay();

    // Auto-detect connected zones
    DetectConnectedZones();

    // Register with the engine
    RegisterWithEngine();

    UE_LOG(LogAcousticEngine, Log, TEXT("Acoustic Portal '%s' activated (ID: %d, Open: %s)"),
        *PortalName.ToString(), PortalId, bIsOpen ? TEXT("Yes") : TEXT("No"));
}

void AAcousticPortalVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnregisterFromEngine();
    Super::EndPlay(EndPlayReason);
}

void AAcousticPortalVolume::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Update transition animation
    UpdateTransition(DeltaTime);
}

void AAcousticPortalVolume::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AAcousticPortalVolume, bIsOpen);
    DOREPLIFETIME(AAcousticPortalVolume, Openness);
}

void AAcousticPortalVolume::SetOpen(bool bOpen)
{
    bIsOpen = bOpen;
    TargetOpenness = bOpen ? 1.0f : 0.0f;

    // Enable tick for transition
    if (TransitionTime > 0.0f)
    {
        SetActorTickEnabled(true);
    }
    else
    {
        Openness = TargetOpenness;
    }
}

void AAcousticPortalVolume::SetOpenness(float NewOpenness)
{
    Openness = FMath::Clamp(NewOpenness, 0.0f, 1.0f);
    TargetOpenness = Openness;
    bIsOpen = Openness > 0.5f;
}

float AAcousticPortalVolume::GetCurrentTransmission() const
{
    // Interpolate between full transmission (open) and closed transmission
    return FMath::Lerp(ClosedTransmission, 1.0f, Openness);
}

float AAcousticPortalVolume::GetCurrentLPFCutoff() const
{
    // Interpolate between closed cutoff and fully open (no filter)
    return FMath::Lerp(ClosedLPFCutoff, 20000.0f, Openness);
}

bool AAcousticPortalVolume::IsOnSoundPath(const FVector& SourceLocation, const FVector& ListenerLocation) const
{
    // Check if line between source and listener intersects this portal
    FVector PortalCenter = GetPortalCenter();
    FVector PortalNormal = GetPortalNormal();

    // Simple check: is the portal between source and listener?
    FVector ToSource = SourceLocation - PortalCenter;
    FVector ToListener = ListenerLocation - PortalCenter;

    // Check if source and listener are on opposite sides
    float DotSource = FVector::DotProduct(ToSource, PortalNormal);
    float DotListener = FVector::DotProduct(ToListener, PortalNormal);

    if (DotSource * DotListener >= 0)
    {
        // Same side, portal not on path
        return false;
    }

    // Check if line passes through the portal volume
    FVector Origin, Extent;
    GetActorBounds(false, Origin, Extent);

    FVector LineDir = (ListenerLocation - SourceLocation).GetSafeNormal();
    float LineLength = FVector::Dist(SourceLocation, ListenerLocation);

    // Ray-box intersection test
    FBox PortalBox(Origin - Extent, Origin + Extent);
    FVector HitLocation;
    FVector HitNormal;
    float HitTime;

    return FMath::LineBoxIntersection(PortalBox, SourceLocation, ListenerLocation, LineDir);
}

FVector AAcousticPortalVolume::GetPortalCenter() const
{
    return GetActorLocation();
}

FVector AAcousticPortalVolume::GetPortalNormal() const
{
    return GetActorForwardVector();
}

void AAcousticPortalVolume::RegisterWithEngine()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (UAcousticEngineSubsystem* Subsystem = World->GetSubsystem<UAcousticEngineSubsystem>())
    {
        Subsystem->RegisterPortal(this);
    }
}

void AAcousticPortalVolume::UnregisterFromEngine()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (UAcousticEngineSubsystem* Subsystem = World->GetSubsystem<UAcousticEngineSubsystem>())
    {
        Subsystem->UnregisterPortal(this);
    }
}

void AAcousticPortalVolume::DetectConnectedZones()
{
    if (ZoneA && ZoneB)
    {
        // Already set manually
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

    // Get portal center and normal
    FVector Center = GetPortalCenter();
    FVector Normal = GetPortalNormal();

    // Check slightly in front and behind the portal
    float ProbeDistance = 100.0f;
    FVector FrontPos = Center + Normal * ProbeDistance;
    FVector BackPos = Center - Normal * ProbeDistance;

    // Find zones at each position
    if (!ZoneA)
    {
        ZoneA = Subsystem->GetZoneAtLocation(FrontPos);
    }
    if (!ZoneB)
    {
        ZoneB = Subsystem->GetZoneAtLocation(BackPos);
    }

    if (ZoneA || ZoneB)
    {
        UE_LOG(LogAcousticEngine, Verbose, TEXT("Portal '%s' connected zones: %s <-> %s"),
            *PortalName.ToString(),
            ZoneA ? *ZoneA->ZoneName.ToString() : TEXT("None"),
            ZoneB ? *ZoneB->ZoneName.ToString() : TEXT("None"));
    }
}

void AAcousticPortalVolume::UpdateTransition(float DeltaTime)
{
    if (FMath::IsNearlyEqual(Openness, TargetOpenness, KINDA_SMALL_NUMBER))
    {
        Openness = TargetOpenness;
        SetActorTickEnabled(false);
        return;
    }

    // Calculate transition rate
    float Rate = TransitionTime > 0.0f ? (1.0f / TransitionTime) : 100.0f;

    // Move towards target
    if (Openness < TargetOpenness)
    {
        Openness = FMath::Min(Openness + Rate * DeltaTime, TargetOpenness);
    }
    else
    {
        Openness = FMath::Max(Openness - Rate * DeltaTime, TargetOpenness);
    }
}
