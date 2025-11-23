// Copyright AcoustiTrace Pro. All Rights Reserved.

#include "AcousticEngineSubsystem.h"
#include "AcousticSourceComponent.h"
#include "AcousticZoneVolume.h"
#include "AcousticSettings.h"
#include "AcousticEngineModule.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "CollisionQueryParams.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "DrawDebugHelpers.h"

// ============================================================================
// CONSTANTS
// ============================================================================

namespace AcousticConstants
{
    // Speed of sound in cm/s (343 m/s = 34300 cm/s)
    constexpr float SpeedOfSound = 34300.0f;

    // Minimum distance for acoustic calculations
    constexpr float MinDistance = 1.0f;

    // Default LPF when no occlusion
    constexpr float DefaultLPFCutoff = 20000.0f;

    // Fully occluded LPF
    constexpr float OccludedLPFCutoff = 500.0f;

    // Priority multipliers by importance
    constexpr float ImportanceMultipliers[] = { 0.25f, 1.0f, 2.0f, 10.0f };
}

// ============================================================================
// CONSTRUCTOR
// ============================================================================

UAcousticEngineSubsystem::UAcousticEngineSubsystem()
{
    // Default initialization
}

// ============================================================================
// SUBSYSTEM LIFECYCLE
// ============================================================================

void UAcousticEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogAcousticEngine, Log, TEXT("AcousticEngineSubsystem initializing for world: %s"),
        *GetWorld()->GetName());

    // Get settings reference
    Settings = UAcousticSettings::Get();

    // Initialize listener array with at least one listener
    ListenerDataArray.SetNum(1);

    // Initialize ray budget
    CurrentBudget.TotalRaysBudget = Settings ? Settings->MaxRaysPerFrame : 200;

    bIsInitialized = true;
}

void UAcousticEngineSubsystem::Deinitialize()
{
    UE_LOG(LogAcousticEngine, Log, TEXT("AcousticEngineSubsystem deinitializing"));

    // Clear all registered sources
    RegisteredSources.Empty();
    RegisteredZones.Empty();
    RegisteredPortals.Empty();
    ListenerDataArray.Empty();

    // Remove tick delegate
    if (TickDelegateHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
        TickDelegateHandle.Reset();
    }

    bIsInitialized = false;

    Super::Deinitialize();
}

bool UAcousticEngineSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    // Create for all game worlds, not for editor preview worlds
    if (UWorld* World = Cast<UWorld>(Outer))
    {
        return World->IsGameWorld();
    }
    return false;
}

void UAcousticEngineSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    UE_LOG(LogAcousticEngine, Log, TEXT("AcousticEngineSubsystem - World Begin Play"));

    // Register tick function
    TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UAcousticEngineSubsystem::TickSubsystem),
        0.0f // Tick every frame
    );
}

bool UAcousticEngineSubsystem::TickSubsystem(float DeltaTime)
{
    Tick(DeltaTime);
    return true; // Continue ticking
}

void UAcousticEngineSubsystem::Tick(float DeltaTime)
{
    if (!bIsInitialized || !Settings)
    {
        return;
    }

    // Update listener positions from player controllers
    UpdateListenersFromPlayers();

    // Process acoustic updates
    ProcessAcousticUpdate(DeltaTime);

    // Debug visualization
    if (Settings->bEnableDebugVisualization)
    {
        DrawDebugVisualization();
    }
}

// ============================================================================
// SOURCE MANAGEMENT
// ============================================================================

int32 UAcousticEngineSubsystem::RegisterSource(UAcousticSourceComponent* Source)
{
    if (!Source)
    {
        return -1;
    }

    // Check if already registered
    for (const auto& Pair : RegisteredSources)
    {
        if (Pair.Value.SourceComponent.Get() == Source)
        {
            return Pair.Key;
        }
    }

    // Create new entry
    FAcousticSourceEntry Entry;
    Entry.SourceComponent = Source;
    Entry.SourceId = NextSourceId++;
    Entry.CurrentParams.Reset();
    Entry.PreviousParams.Reset();
    Entry.EffectiveLOD = Source->AcousticLOD;
    Entry.bIsAudible = true;

    RegisteredSources.Add(Entry.SourceId, Entry);

    UE_LOG(LogAcousticEngine, Verbose, TEXT("Registered acoustic source %d: %s"),
        Entry.SourceId, *Source->GetOwner()->GetName());

    return Entry.SourceId;
}

void UAcousticEngineSubsystem::UnregisterSource(int32 SourceId)
{
    if (RegisteredSources.Remove(SourceId) > 0)
    {
        UE_LOG(LogAcousticEngine, Verbose, TEXT("Unregistered acoustic source %d"), SourceId);
    }
}

bool UAcousticEngineSubsystem::GetSourceParams(int32 SourceId, FAcousticSourceParams& OutParams) const
{
    if (const FAcousticSourceEntry* Entry = RegisteredSources.Find(SourceId))
    {
        OutParams = Entry->CurrentParams;
        return true;
    }
    return false;
}

void UAcousticEngineSubsystem::ForceSourceUpdate(int32 SourceId)
{
    if (FAcousticSourceEntry* Entry = RegisteredSources.Find(SourceId))
    {
        Entry->LastOcclusionUpdateTime = 0.0;
        Entry->LastReflectionUpdateTime = 0.0;
    }
}

// ============================================================================
// LISTENER MANAGEMENT
// ============================================================================

void UAcousticEngineSubsystem::UpdateListener(int32 ListenerIndex, const FAcousticListenerData& ListenerData)
{
    if (ListenerIndex >= 0)
    {
        if (ListenerIndex >= ListenerDataArray.Num())
        {
            ListenerDataArray.SetNum(ListenerIndex + 1);
        }
        ListenerDataArray[ListenerIndex] = ListenerData;
    }
}

FAcousticListenerData UAcousticEngineSubsystem::GetListenerData(int32 ListenerIndex) const
{
    if (ListenerIndex >= 0 && ListenerIndex < ListenerDataArray.Num())
    {
        return ListenerDataArray[ListenerIndex];
    }
    return FAcousticListenerData();
}

void UAcousticEngineSubsystem::UpdateListenersFromPlayers()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    int32 ListenerIndex = 0;
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        if (APlayerController* PC = It->Get())
        {
            if (PC->IsLocalController())
            {
                FAcousticListenerData ListenerData;

                // Get listener location from camera or pawn
                FVector ViewLocation;
                FRotator ViewRotation;
                PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

                ListenerData.Location = ViewLocation;
                ListenerData.Forward = ViewRotation.Vector();
                ListenerData.Right = FRotationMatrix(ViewRotation).GetScaledAxis(EAxis::Y);
                ListenerData.Up = FRotationMatrix(ViewRotation).GetScaledAxis(EAxis::Z);
                ListenerData.PlayerIndex = ListenerIndex;

                // Calculate velocity from previous frame
                if (ListenerIndex < ListenerDataArray.Num())
                {
                    FVector PrevLocation = ListenerDataArray[ListenerIndex].Location;
                    float DeltaTime = World->GetDeltaSeconds();
                    if (DeltaTime > KINDA_SMALL_NUMBER)
                    {
                        ListenerData.Velocity = (ViewLocation - PrevLocation) / DeltaTime;
                    }
                }

                UpdateListener(ListenerIndex, ListenerData);
                ListenerIndex++;
            }
        }
    }
}

// ============================================================================
// ZONE MANAGEMENT
// ============================================================================

void UAcousticEngineSubsystem::RegisterZone(AAcousticZoneVolume* Zone)
{
    if (Zone && !RegisteredZones.Contains(Zone))
    {
        RegisteredZones.Add(Zone);
        UE_LOG(LogAcousticEngine, Verbose, TEXT("Registered acoustic zone: %s"), *Zone->ZoneName.ToString());
    }
}

void UAcousticEngineSubsystem::UnregisterZone(AAcousticZoneVolume* Zone)
{
    RegisteredZones.Remove(Zone);
}

void UAcousticEngineSubsystem::RegisterPortal(AAcousticPortalVolume* Portal)
{
    if (Portal && !RegisteredPortals.Contains(Portal))
    {
        RegisteredPortals.Add(Portal);
        UE_LOG(LogAcousticEngine, Verbose, TEXT("Registered acoustic portal: %s"), *Portal->PortalName.ToString());
    }
}

void UAcousticEngineSubsystem::UnregisterPortal(AAcousticPortalVolume* Portal)
{
    RegisteredPortals.Remove(Portal);
}

AAcousticZoneVolume* UAcousticEngineSubsystem::GetZoneAtLocation(const FVector& Location) const
{
    AAcousticZoneVolume* BestZone = nullptr;
    int32 BestPriority = INT_MIN;

    for (const TWeakObjectPtr<AAcousticZoneVolume>& ZonePtr : RegisteredZones)
    {
        if (AAcousticZoneVolume* Zone = ZonePtr.Get())
        {
            if (Zone->ContainsPoint(Location) && Zone->Priority > BestPriority)
            {
                BestZone = Zone;
                BestPriority = Zone->Priority;
            }
        }
    }

    return BestZone;
}

FAcousticZonePreset UAcousticEngineSubsystem::GetCurrentZonePreset(int32 ListenerIndex) const
{
    if (ListenerIndex >= 0 && ListenerIndex < ListenerDataArray.Num())
    {
        const FAcousticListenerData& Listener = ListenerDataArray[ListenerIndex];
        if (AAcousticZoneVolume* Zone = GetZoneAtLocation(Listener.Location))
        {
            return Zone->GetZonePreset();
        }
    }

    // Return default preset
    FAcousticZonePreset Default;
    Default.PresetName = FName("Default");
    Default.ZoneType = EAcousticZoneType::Default;
    return Default;
}

// ============================================================================
// AUDIO MODE
// ============================================================================

void UAcousticEngineSubsystem::SetAudioOutputMode(EAudioOutputMode NewMode)
{
    if (CurrentOutputMode != NewMode)
    {
        CurrentOutputMode = NewMode;
        UE_LOG(LogAcousticEngine, Log, TEXT("Audio output mode changed to: %s"),
            NewMode == EAudioOutputMode::Headphones ? TEXT("Headphones") : TEXT("Speakers"));

        // Notify all sources of mode change
        for (auto& Pair : RegisteredSources)
        {
            if (UAcousticSourceComponent* Source = Pair.Value.SourceComponent.Get())
            {
                Source->ForceUpdate();
            }
        }
    }
}

// ============================================================================
// MATERIAL SYSTEM
// ============================================================================

FAcousticMaterial UAcousticEngineSubsystem::GetAcousticMaterial(UPhysicalMaterial* PhysMat) const
{
    if (PhysMat)
    {
        FName PhysMatName = PhysMat->GetFName();
        if (const FAcousticMaterial* Found = MaterialMappings.Find(PhysMatName))
        {
            return *Found;
        }
    }

    // Return default material
    FAcousticMaterial Default;
    Default.MaterialType = EAcousticMaterialType::Default;
    Default.LowAbsorption = 0.1f;
    Default.MidAbsorption = 0.2f;
    Default.HighAbsorption = 0.3f;
    Default.Transmission = 0.0f;
    Default.Scattering = 0.1f;
    return Default;
}

void UAcousticEngineSubsystem::RegisterMaterialMapping(FName PhysMatName, const FAcousticMaterial& AcousticMat)
{
    MaterialMappings.Add(PhysMatName, AcousticMat);
}

// ============================================================================
// RUNTIME QUERIES
// ============================================================================

float UAcousticEngineSubsystem::TraceOcclusion(const FVector& Start, const FVector& End, FAcousticRayHit& OutHit) const
{
    UWorld* World = GetWorld();
    if (!World || !Settings)
    {
        return 0.0f;
    }

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.bTraceComplex = Settings->bUseComplexCollision;
    QueryParams.bReturnPhysicalMaterial = true;

    bool bHit = World->LineTraceSingleByChannel(
        HitResult,
        Start,
        End,
        Settings->AudioOcclusionChannel,
        QueryParams
    );

    if (bHit)
    {
        OutHit.bIsValidHit = true;
        OutHit.HitLocation = HitResult.ImpactPoint;
        OutHit.HitNormal = HitResult.ImpactNormal;
        OutHit.Distance = HitResult.Distance;

        if (HitResult.PhysMaterial.IsValid())
        {
            OutHit.PhysicalMaterialName = HitResult.PhysMaterial->GetFName();
            OutHit.Material = GetAcousticMaterial(HitResult.PhysMaterial.Get());
        }
        else
        {
            OutHit.Material = GetAcousticMaterial(nullptr);
        }

        return ComputeOcclusionFactor(OutHit);
    }

    OutHit.bIsValidHit = false;
    return 0.0f;
}

void UAcousticEngineSubsystem::SampleReflections(const FVector& Origin, const FVector& Forward, int32 NumRays, TArray<FAcousticRayHit>& OutHits) const
{
    UWorld* World = GetWorld();
    if (!World || !Settings)
    {
        return;
    }

    OutHits.Reset();
    OutHits.Reserve(NumRays);

    // Generate hemisphere directions
    TArray<FVector> Directions;
    GenerateHemisphereRays(Forward, NumRays, Directions);

    FCollisionQueryParams QueryParams;
    QueryParams.bTraceComplex = Settings->bUseComplexCollision;
    QueryParams.bReturnPhysicalMaterial = true;

    for (const FVector& Direction : Directions)
    {
        FHitResult HitResult;
        FVector End = Origin + Direction * Settings->MaxTraceDistance;

        bool bHit = World->LineTraceSingleByChannel(
            HitResult,
            Origin,
            End,
            Settings->AudioOcclusionChannel,
            QueryParams
        );

        if (bHit)
        {
            FAcousticRayHit RayHit;
            RayHit.bIsValidHit = true;
            RayHit.HitLocation = HitResult.ImpactPoint;
            RayHit.HitNormal = HitResult.ImpactNormal;
            RayHit.Distance = HitResult.Distance;

            if (HitResult.PhysMaterial.IsValid())
            {
                RayHit.PhysicalMaterialName = HitResult.PhysMaterial->GetFName();
                RayHit.Material = GetAcousticMaterial(HitResult.PhysMaterial.Get());
            }
            else
            {
                RayHit.Material = GetAcousticMaterial(nullptr);
            }

            OutHits.Add(RayHit);
        }
    }
}

// ============================================================================
// STATS & DEBUG
// ============================================================================

int32 UAcousticEngineSubsystem::GetNumActiveSources() const
{
    int32 Count = 0;
    for (const auto& Pair : RegisteredSources)
    {
        if (Pair.Value.bIsAudible)
        {
            Count++;
        }
    }
    return Count;
}

// ============================================================================
// INTERNAL PROCESSING
// ============================================================================

void UAcousticEngineSubsystem::ProcessAcousticUpdate(float DeltaTime)
{
    // Reset ray budget
    CurrentBudget.OcclusionRays = 0;
    CurrentBudget.ReflectionRays = 0;
    CurrentBudget.TotalRaysUsed = 0;
    CurrentBudget.TotalRaysBudget = Settings->MaxRaysPerFrame;

    // Update accumulators
    OcclusionUpdateAccumulator += DeltaTime;
    ReflectionUpdateAccumulator += DeltaTime;
    ZoneUpdateAccumulator += DeltaTime;

    // Calculate update intervals
    float OcclusionInterval = 1.0f / Settings->OcclusionUpdateRateHz;
    float ReflectionInterval = 1.0f / Settings->ReflectionUpdateRateHz;
    float ZoneInterval = 1.0f / Settings->ZoneUpdateRateHz;

    // Update source priorities
    UpdateSourcePriorities();

    // Update listener zones if needed
    if (ZoneUpdateAccumulator >= ZoneInterval)
    {
        UpdateListenerZones();
        ZoneUpdateAccumulator = 0.0f;
    }

    // Process occlusion if needed
    if (OcclusionUpdateAccumulator >= OcclusionInterval)
    {
        ProcessOcclusion(DeltaTime);
        OcclusionUpdateAccumulator = 0.0f;
    }

    // Process reflections if needed
    if (ReflectionUpdateAccumulator >= ReflectionInterval)
    {
        ProcessReflections(DeltaTime);
        ReflectionUpdateAccumulator = 0.0f;
    }

    // Apply parameters to sources
    ApplyParamsToSources();
}

void UAcousticEngineSubsystem::UpdateSourcePriorities()
{
    if (ListenerDataArray.Num() == 0)
    {
        return;
    }

    const FVector& ListenerLocation = ListenerDataArray[0].Location;
    int32 AdvancedCount = 0;
    int32 HeroCount = 0;

    // Calculate priorities and sort
    TArray<TPair<int32, float>> SortedSources;

    for (auto& Pair : RegisteredSources)
    {
        FAcousticSourceEntry& Entry = Pair.Value;
        UAcousticSourceComponent* Source = Entry.SourceComponent.Get();

        if (!Source)
        {
            continue;
        }

        // Calculate distance
        FVector SourceLocation = Source->GetAcousticLocation();
        float Distance = FMath::Max(FVector::Dist(SourceLocation, ListenerLocation), AcousticConstants::MinDistance);
        Entry.CurrentParams.Distance = Distance;

        // Calculate priority score
        float Priority = Source->ComputePriorityScore(ListenerLocation);
        Entry.PriorityScore = Priority;

        SortedSources.Add(TPair<int32, float>(Pair.Key, Priority));
    }

    // Sort by priority (highest first)
    SortedSources.Sort([](const TPair<int32, float>& A, const TPair<int32, float>& B) {
        return A.Value > B.Value;
    });

    // Assign effective LODs based on budget
    for (const auto& SortedPair : SortedSources)
    {
        FAcousticSourceEntry* Entry = RegisteredSources.Find(SortedPair.Key);
        if (!Entry || !Entry->SourceComponent.IsValid())
        {
            continue;
        }

        UAcousticSourceComponent* Source = Entry->SourceComponent.Get();
        EAcousticLOD DesiredLOD = Source->AcousticLOD;

        // Distance-based downgrade
        float Distance = Entry->CurrentParams.Distance;
        if (Distance > Settings->OffLODDistance)
        {
            DesiredLOD = EAcousticLOD::Off;
        }
        else if (Distance > Settings->BasicLODDistance && DesiredLOD > EAcousticLOD::Basic)
        {
            DesiredLOD = EAcousticLOD::Basic;
        }

        // Budget-based downgrade
        if (DesiredLOD == EAcousticLOD::Hero)
        {
            if (HeroCount >= Settings->MaxHeroSources)
            {
                DesiredLOD = EAcousticLOD::Advanced;
            }
            else
            {
                HeroCount++;
            }
        }

        if (DesiredLOD == EAcousticLOD::Advanced)
        {
            if (AdvancedCount >= Settings->MaxAdvancedSources)
            {
                DesiredLOD = EAcousticLOD::Basic;
            }
            else
            {
                AdvancedCount++;
            }
        }

        Entry->EffectiveLOD = DesiredLOD;
        Entry->bIsAudible = (DesiredLOD != EAcousticLOD::Off);
    }
}

void UAcousticEngineSubsystem::ProcessOcclusion(float DeltaTime)
{
    if (ListenerDataArray.Num() == 0)
    {
        return;
    }

    const FVector& ListenerLocation = ListenerDataArray[0].Location;
    double CurrentTime = FPlatformTime::Seconds();

    for (auto& Pair : RegisteredSources)
    {
        FAcousticSourceEntry& Entry = Pair.Value;

        if (Entry.EffectiveLOD == EAcousticLOD::Off)
        {
            continue;
        }

        // Check ray budget
        if (CurrentBudget.TotalRaysUsed >= CurrentBudget.TotalRaysBudget)
        {
            break;
        }

        UAcousticSourceComponent* Source = Entry.SourceComponent.Get();
        if (!Source)
        {
            continue;
        }

        // Check if we should use cached data
        double TimeSinceUpdate = CurrentTime - Entry.LastOcclusionUpdateTime;
        float CacheTime = Settings->OcclusionCacheFrames / 60.0f;

        if (TimeSinceUpdate < CacheTime && Entry.CurrentParams.bIsValid)
        {
            continue; // Use cached data
        }

        // Trace occlusion
        FVector SourceLocation = Source->GetAcousticLocation();
        FAcousticRayHit OcclusionHit;
        float Occlusion = TraceOcclusion(ListenerLocation, SourceLocation, OcclusionHit);

        // Store previous params for interpolation
        Entry.PreviousParams = Entry.CurrentParams;

        // Update occlusion params
        Entry.CurrentParams.Occlusion = Occlusion;
        Entry.CurrentParams.LowPassCutoff = ComputeLPFFromOcclusion(Occlusion, OcclusionHit.Material);
        Entry.CurrentParams.TransmissionGain = OcclusionHit.bIsValidHit ?
            (1.0f - Occlusion) + (Occlusion * OcclusionHit.Material.Transmission) : 1.0f;
        Entry.CurrentParams.bIsValid = true;

        Entry.LastOcclusionUpdateTime = CurrentTime;
        CurrentBudget.OcclusionRays++;
        CurrentBudget.TotalRaysUsed++;
    }
}

void UAcousticEngineSubsystem::ProcessReflections(float DeltaTime)
{
    if (ListenerDataArray.Num() == 0)
    {
        return;
    }

    const FAcousticListenerData& Listener = ListenerDataArray[0];
    double CurrentTime = FPlatformTime::Seconds();

    for (auto& Pair : RegisteredSources)
    {
        FAcousticSourceEntry& Entry = Pair.Value;

        // Only process Advanced and Hero LODs
        if (Entry.EffectiveLOD != EAcousticLOD::Advanced && Entry.EffectiveLOD != EAcousticLOD::Hero)
        {
            // For Basic LOD, just use zone-based reverb
            if (Entry.EffectiveLOD == EAcousticLOD::Basic)
            {
                FAcousticZonePreset ZonePreset = GetCurrentZonePreset(0);
                Entry.CurrentParams.ReverbSend = ZonePreset.DefaultReverbSend;
                Entry.CurrentParams.EarlyReflections.Reset();
            }
            continue;
        }

        // Check ray budget
        int32 NumRays = (Entry.EffectiveLOD == EAcousticLOD::Hero) ?
            Settings->HeroReflectionRays : Settings->AdvancedReflectionRays;

        if (CurrentBudget.TotalRaysUsed + NumRays > CurrentBudget.TotalRaysBudget)
        {
            continue;
        }

        UAcousticSourceComponent* Source = Entry.SourceComponent.Get();
        if (!Source)
        {
            continue;
        }

        // Sample reflections from listener position
        TArray<FAcousticRayHit> ReflectionHits;
        SampleReflections(Listener.Location, Listener.Forward, NumRays, ReflectionHits);

        // Cluster reflections into taps
        ClusterReflections(ReflectionHits, Entry.CurrentParams.EarlyReflections);

        // Compute reverb send based on reflections
        float ReflectionDensity = Entry.CurrentParams.EarlyReflections.ReflectionDensity;
        FAcousticZonePreset ZonePreset = GetCurrentZonePreset(0);
        Entry.CurrentParams.ReverbSend = FMath::Lerp(
            ZonePreset.DefaultReverbSend,
            ZonePreset.DefaultReverbSend * 1.5f,
            ReflectionDensity
        );

        Entry.LastReflectionUpdateTime = CurrentTime;
        CurrentBudget.ReflectionRays += NumRays;
        CurrentBudget.TotalRaysUsed += NumRays;
    }
}

void UAcousticEngineSubsystem::UpdateListenerZones()
{
    for (int32 i = 0; i < ListenerDataArray.Num(); i++)
    {
        FAcousticListenerData& Listener = ListenerDataArray[i];
        AAcousticZoneVolume* CurrentZone = GetZoneAtLocation(Listener.Location);
        int32 NewZoneId = CurrentZone ? CurrentZone->GetZoneId() : -1;

        if (NewZoneId != Listener.CurrentZoneId)
        {
            int32 OldZoneId = Listener.CurrentZoneId;
            Listener.CurrentZoneId = NewZoneId;

            // Broadcast zone change
            OnAcousticZoneChanged.Broadcast(i, OldZoneId, NewZoneId);

            UE_LOG(LogAcousticEngine, Verbose, TEXT("Listener %d changed zone from %d to %d"),
                i, OldZoneId, NewZoneId);
        }
    }
}

void UAcousticEngineSubsystem::ApplyParamsToSources()
{
    for (auto& Pair : RegisteredSources)
    {
        FAcousticSourceEntry& Entry = Pair.Value;
        UAcousticSourceComponent* Source = Entry.SourceComponent.Get();

        if (Source && Entry.CurrentParams.bIsValid)
        {
            Source->OnParamsUpdated(Entry.CurrentParams);
            OnAcousticParamsUpdated.Broadcast(Pair.Key, Entry.CurrentParams);
        }
    }
}

void UAcousticEngineSubsystem::ClusterReflections(const TArray<FAcousticRayHit>& Hits, FEarlyReflectionParams& OutParams)
{
    OutParams.Reset();

    if (Hits.Num() == 0)
    {
        return;
    }

    // Sort hits by distance
    TArray<FAcousticRayHit> SortedHits = Hits;
    SortedHits.Sort([](const FAcousticRayHit& A, const FAcousticRayHit& B) {
        return A.Distance < B.Distance;
    });

    // Calculate density
    OutParams.ReflectionDensity = FMath::Clamp(Hits.Num() / 20.0f, 0.0f, 1.0f);

    // Create up to 8 taps from the hits
    int32 NumTaps = FMath::Min(SortedHits.Num(), FEarlyReflectionParams::MaxTaps);
    float TotalDelay = 0.0f;

    for (int32 i = 0; i < NumTaps; i++)
    {
        const FAcousticRayHit& Hit = SortedHits[i];
        FReflectionTap& Tap = OutParams.Taps[i];

        // Calculate delay from path length
        Tap.DelayMs = (Hit.Distance / AcousticConstants::SpeedOfSound) * 1000.0f;
        TotalDelay += Tap.DelayMs;

        // Calculate gain based on distance and material
        float DistanceAtten = 1.0f / FMath::Max(Hit.Distance / 100.0f, 1.0f);
        float MaterialAtten = 1.0f - Hit.Material.GetAverageAbsorption();
        Tap.Gain = FMath::Clamp(DistanceAtten * MaterialAtten * 0.5f, 0.0f, 1.0f);

        // Calculate LPF from material
        Tap.LPFCutoff = FMath::Lerp(
            AcousticConstants::DefaultLPFCutoff,
            3000.0f,
            Hit.Material.HighAbsorption
        );

        // Calculate direction for spatial positioning (simplified)
        // In a full implementation, this would compute azimuth/elevation from hit location
        Tap.Azimuth = 0.0f;
        Tap.Elevation = 0.0f;

        Tap.bIsValid = true;
    }

    OutParams.ValidTapCount = NumTaps;
    OutParams.AverageDelayMs = NumTaps > 0 ? TotalDelay / NumTaps : 0.0f;
}

float UAcousticEngineSubsystem::ComputeOcclusionFactor(const FAcousticRayHit& Hit) const
{
    if (!Hit.bIsValidHit)
    {
        return 0.0f;
    }

    // Base occlusion from transmission (0 transmission = full occlusion)
    float BaseOcclusion = 1.0f - Hit.Material.Transmission;

    // Apply realism factor
    float RealismFactor = Settings ? Settings->RealismFactor : 0.7f;
    float Occlusion = BaseOcclusion * RealismFactor;

    return FMath::Clamp(Occlusion, 0.0f, 1.0f);
}

float UAcousticEngineSubsystem::ComputeLPFFromOcclusion(float OcclusionFactor, const FAcousticMaterial& Material) const
{
    // Higher occlusion = lower cutoff frequency
    float MinCutoff = AcousticConstants::OccludedLPFCutoff;
    float MaxCutoff = AcousticConstants::DefaultLPFCutoff;

    // Material high absorption makes the cutoff even lower
    float MaterialFactor = 1.0f - (Material.HighAbsorption * 0.5f);
    MinCutoff *= MaterialFactor;

    return FMath::Lerp(MaxCutoff, MinCutoff, OcclusionFactor);
}

void UAcousticEngineSubsystem::GenerateHemisphereRays(const FVector& Normal, int32 NumRays, TArray<FVector>& OutDirections) const
{
    OutDirections.Reset();
    OutDirections.Reserve(NumRays);

    // Use golden ratio for even distribution
    const float GoldenRatio = (1.0f + FMath::Sqrt(5.0f)) / 2.0f;
    const float AngleIncrement = PI * 2.0f * GoldenRatio;

    // Create rotation from up to normal
    FQuat Rotation = FQuat::FindBetweenNormals(FVector::UpVector, Normal);

    for (int32 i = 0; i < NumRays; i++)
    {
        float T = (float)i / (float)NumRays;
        float Inclination = FMath::Acos(1.0f - T); // 0 to PI/2 for hemisphere
        float Azimuth = AngleIncrement * i;

        FVector Direction(
            FMath::Sin(Inclination) * FMath::Cos(Azimuth),
            FMath::Sin(Inclination) * FMath::Sin(Azimuth),
            FMath::Cos(Inclination)
        );

        // Rotate to align with normal
        Direction = Rotation.RotateVector(Direction);
        OutDirections.Add(Direction);
    }
}

void UAcousticEngineSubsystem::DrawDebugVisualization()
{
#if ENABLE_DRAW_DEBUG
    UWorld* World = GetWorld();
    if (!World || !Settings)
    {
        return;
    }

    // Draw listener position
    for (const FAcousticListenerData& Listener : ListenerDataArray)
    {
        DrawDebugSphere(World, Listener.Location, 25.0f, 8, FColor::Green, false, -1.0f);
        DrawDebugDirectionalArrow(World, Listener.Location,
            Listener.Location + Listener.Forward * 100.0f, 20.0f, FColor::Blue, false, -1.0f);
    }

    // Draw source positions
    for (const auto& Pair : RegisteredSources)
    {
        const FAcousticSourceEntry& Entry = Pair.Value;
        if (UAcousticSourceComponent* Source = Entry.SourceComponent.Get())
        {
            FVector Location = Source->GetAcousticLocation();
            FColor Color = Entry.bIsAudible ? FColor::Yellow : FColor::Red;

            DrawDebugSphere(World, Location, 15.0f, 6, Color, false, -1.0f);

            // Draw occlusion line
            if (Settings->bDrawOcclusionRays && ListenerDataArray.Num() > 0)
            {
                FColor OcclusionColor = FColor::MakeRedToGreenColorFromScalar(1.0f - Entry.CurrentParams.Occlusion);
                DrawDebugLine(World, ListenerDataArray[0].Location, Location, OcclusionColor, false, -1.0f);
            }
        }
    }

    // Draw zone boundaries
    if (Settings->bDrawZoneBoundaries)
    {
        for (const TWeakObjectPtr<AAcousticZoneVolume>& ZonePtr : RegisteredZones)
        {
            if (AAcousticZoneVolume* Zone = ZonePtr.Get())
            {
                FVector Origin, Extent;
                Zone->GetActorBounds(false, Origin, Extent);
                DrawDebugBox(World, Origin, Extent, FColor::Cyan, false, -1.0f);
            }
        }
    }
#endif
}
