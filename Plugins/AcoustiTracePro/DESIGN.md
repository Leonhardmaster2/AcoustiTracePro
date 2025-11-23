# AcoustiTrace Pro - UE5.6 Acoustic Simulation Plugin

## Complete Design Document

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Core Components](#core-components)
4. [Signal Flow](#signal-flow)
5. [Ray Tracing System](#ray-tracing-system)
6. [MetaSound Integration](#metasound-integration)
7. [Zone & Portal System](#zone--portal-system)
8. [Multiplayer Architecture](#multiplayer-architecture)
9. [Configuration & Settings](#configuration--settings)
10. [Performance Optimization](#performance-optimization)
11. [API Reference](#api-reference)

---

## Overview

AcoustiTrace Pro is an advanced acoustic simulation engine for Unreal Engine 5.6 that provides:

- **Raytraced Occlusion**: Physically-based sound blocking through geometry
- **Early Reflections**: Per-source reflection taps computed from environment
- **Zone-Based Reverb**: Automatic reverb adjustment based on room characteristics
- **HRTF Spatialization**: Proper headphone 3D audio with left/right and elevation
- **Multiplayer Support**: Client-side acoustic processing with replicated events
- **Performance Budgeting**: Configurable ray budgets and LOD system

### Design Philosophy

```
Your Plugin = High-Level Controller that feeds UE5 with correct parameters
```

The plugin does NOT replace UE5's audio system. Instead, it:
1. Performs geometry queries (occlusion, reflections, room detection)
2. Outputs per-source parameters (Gain, LPF, Reverb Send, Spatial Width)
3. Feeds these into UE5's existing audio pipeline

---

## Architecture

### Module Structure

```
AcoustiTracePro/
├── AcoustiTracePro.uplugin
├── Source/
│   ├── AcousticEngine/           # Runtime module
│   │   ├── Public/
│   │   │   ├── AcousticTypes.h              # Core types and enums
│   │   │   ├── AcousticSettings.h           # Configuration
│   │   │   ├── AcousticEngineSubsystem.h    # Main engine
│   │   │   ├── AcousticSourceComponent.h    # Per-source component
│   │   │   ├── AcousticZoneVolume.h         # Zone/Portal volumes
│   │   │   ├── AcousticSubmixEffects.h      # Submix effects
│   │   │   ├── AcousticMultiplayer.h        # Multiplayer support
│   │   │   └── MetaSound/
│   │   │       └── AcousticMetaSoundNodes.h # Custom MetaSound nodes
│   │   └── Private/
│   │       └── [Implementation files]
│   └── AcousticEngineEditor/     # Editor module
│       ├── Public/
│       └── Private/
├── Content/
│   ├── MetaSounds/               # Template MetaSound graphs
│   ├── Presets/                  # Zone presets
│   └── Materials/                # Material presets
└── Config/
```

### System Layers

```
┌─────────────────────────────────────────────────────────────────┐
│                      Game / Level                                │
│  (Actors, Pawns, Sound Sources, Zone Volumes)                   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│              UAcousticEngineSubsystem                           │
│  - Manages all sources and listeners                            │
│  - Performs ray tracing within budget                           │
│  - Computes acoustic parameters                                 │
│  - Distributes params to components                             │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│              UAcousticSourceComponent                           │
│  - Receives params from subsystem                               │
│  - Pushes to MetaSound/Audio Component                          │
│  - Per-source LOD and flags                                     │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│              UE5 Audio System                                   │
│  - UAudioComponent                                              │
│  - MetaSound graphs                                             │
│  - Spatialization (HRTF)                                        │
│  - Submix routing                                               │
└─────────────────────────────────────────────────────────────────┘
```

---

## Core Components

### 1. UAcousticEngineSubsystem

World subsystem that manages all acoustic processing.

**Responsibilities:**
- Maintains list of active listeners and sources
- Enforces ray budget per frame
- Updates per-source occlusion & reflection params
- Manages zone transitions

**Key Methods:**
```cpp
// Source Management
int32 RegisterSource(UAcousticSourceComponent* Source);
void UnregisterSource(int32 SourceId);
bool GetSourceParams(int32 SourceId, FAcousticSourceParams& OutParams);

// Listener Management
void UpdateListener(int32 Index, const FAcousticListenerData& Data);
FAcousticListenerData GetListenerData(int32 Index);

// Zone Management
void RegisterZone(AAcousticZoneVolume* Zone);
AAcousticZoneVolume* GetZoneAtLocation(const FVector& Location);

// Runtime Queries
float TraceOcclusion(const FVector& Start, const FVector& End, FAcousticRayHit& Hit);
void SampleReflections(const FVector& Origin, int32 NumRays, TArray<FAcousticRayHit>& Hits);
```

### 2. UAcousticSourceComponent

Attaches to actors with audio sources.

**Configuration:**
```cpp
EAcousticLOD AcousticLOD;        // Off, Basic, Advanced, Hero
EAcousticImportance Importance;  // Background, Normal, Important, Critical
uint8 SourceFlags;               // IsHero, Environmental, UI, AlwaysAudible, etc.
float BaseLoudness;              // Affects priority (0-1)
float BaseSpatialWidth;          // Base spread (0=point, 1=diffuse)
```

**Runtime State:**
```cpp
FAcousticSourceParams CurrentParams;
EAcousticLOD EffectiveLOD;       // May differ from configured due to budget
int32 SourceId;
bool bIsRegistered;
```

### 3. FAcousticSourceParams

Complete acoustic state for a source:

```cpp
struct FAcousticSourceParams
{
    // Occlusion
    float Occlusion;           // 0-1, how blocked
    float LowPassCutoff;       // Hz, for muffling
    float HighPassCutoff;      // Hz
    float TransmissionGain;    // Through occluder

    // Reverb
    float ReverbSend;          // 0-1, send to reverb bus
    float DryGain;             // Direct signal level

    // Spatialization
    float SpatialWidth;        // 0=point, 1=diffuse
    float HRTFSpreadMultiplier;

    // Early Reflections
    FEarlyReflectionParams EarlyReflections;  // 8 taps

    // Distance
    float Distance;
    float PerceivedDistance;
};
```

---

## Signal Flow

### Per-Source Audio Path

```
Sound Source
     │
     ▼
┌─────────────────────────────┐
│    MetaSound Graph          │
│  ┌───────────────────────┐  │
│  │ Acoustic Processor    │  │
│  │ - Occlusion Filter    │  │
│  │ - Early Reflections   │  │
│  │ - Spatial Width       │  │
│  └───────────────────────┘  │
└─────────────────────────────┘
     │
     ▼
┌─────────────────────────────┐
│    Source Effects           │
│  (Additional DSP)           │
└─────────────────────────────┘
     │
     ▼
┌─────────────────────────────┐
│    Spatialization           │
│  (HRTF when Headphones)     │
└─────────────────────────────┘
     │
     ├──────────────────────┐
     ▼                      ▼
┌───────────┐      ┌───────────────────┐
│ Dry Bus   │      │ Reverb Submix     │
│ (Direct)  │      │ (Zone-based)      │
└───────────┘      └───────────────────┘
     │                      │
     └──────────┬───────────┘
                ▼
┌─────────────────────────────┐
│    Master Submix            │
│  - Headphone Crossfeed      │
│  - Final Limiting           │
└─────────────────────────────┘
     │
     ▼
   Output
```

### Submix Structure

```cpp
// Required submixes
MasterSubmix                    // Final output
├── EnvironmentReverbSubmix     // Zone-based reverb (controlled by plugin)
├── DrySubmix                   // Direct sounds (no reverb)
└── UISubmix                    // Non-diegetic (no acoustic processing)
```

---

## Ray Tracing System

### Geometry Source

Use dedicated collision channel for audio:
```cpp
ECC_AudioOcclusion    // For occlusion traces
ECC_AudioPortal       // For portal detection
```

Configure via Project Settings or per-mesh:
- Use simplified collision or dedicated Audio Proxy meshes
- Reuse NavMesh or simplified collision where appropriate

### Per-Frame Acoustic Query

**1. Direct Occlusion Ray**
```cpp
// For each listener + relevant source pair:
LineTraceSingleByChannel(Listener, Source, ECC_AudioOcclusion)

if (Hit)
{
    Material = GetAcousticMaterial(Hit.PhysMaterial);
    OcclusionFactor = ComputeOcclusion(Material);
    LPFCutoff = ComputeLPF(Material, OcclusionFactor);
}
```

**2. Reflections (from Listener)**
```cpp
// Fire N rays in hemisphere from listener
for (int i = 0; i < NumRays; i++)
{
    Direction = GetHemisphereDirection(ListenerForward, i, NumRays);
    LineTrace(Listener, Listener + Direction * MaxDistance);

    if (Hit)
    {
        PathLength = Hit.Distance;
        DelayMs = PathLength / SpeedOfSound * 1000;
        Absorption = GetMaterialAbsorption(Hit.Material);
        // Store for clustering
    }
}

// Cluster to 4-8 taps
ClusterReflections(Hits, EarlyReflections);
```

**3. Zone Detection**
```cpp
// Find current zone for listener
Zone = GetZoneAtLocation(ListenerLocation);
if (Zone != PreviousZone)
{
    // Blend reverb settings
    BlendToZone(Zone, BlendTime);
}
```

### Cost Control

**Ray Budget System:**
```cpp
MaxRaysPerFrame = 200;  // Total budget

// Per frame:
// 1. Sort sources by priority
// 2. Allocate rays to top N sources
// 3. For others: reuse cached data for several frames
```

**LOD-Based Allocation:**
| LOD | Occlusion | Reflections | Update Rate |
|-----|-----------|-------------|-------------|
| Off | None | None | N/A |
| Basic | 1 ray | Zone only | 30 Hz |
| Advanced | 1 ray | 8-16 rays | 15 Hz |
| Hero | 1 ray | 24-32 rays | 30 Hz |

---

## MetaSound Integration

### Custom MetaSound Nodes

**1. Acoustic Occlusion Filter**
```
Inputs:
- Audio (Buffer)
- Occlusion (Float, 0-1)
- LPF Cutoff (Float, Hz)
- Gain Reduction (Float, dB)

Outputs:
- Audio (Buffer)
```

**2. Acoustic Early Reflections**
```
Inputs:
- Audio (Buffer)
- Tap[0-7] Delay (Float, ms)
- Tap[0-7] Gain (Float, 0-1)
- Tap[0-7] LPF (Float, Hz)
- Wet/Dry (Float)

Outputs:
- Audio (Buffer)
```

**3. Acoustic Spatial Width**
```
Inputs:
- Audio L/R (Buffer)
- Width (Float, 0-1)
- Decorrelation (Float)

Outputs:
- Audio L/R (Buffer)
```

**4. Acoustic Processor (All-in-One)**
```
Inputs:
- Audio (Buffer)
- Occlusion (Float)
- LPF Cutoff (Float)
- Reverb Send (Float)
- Spatial Width (Float)
- Wet/Dry (Float)

Outputs:
- Audio L/R (Buffer)
- Reverb Send (Float)
```

### Parameter Names (for binding)

```cpp
static FName GetOcclusionParamName()    { return "Acoustic_Occlusion"; }
static FName GetLPFCutoffParamName()    { return "Acoustic_LPFCutoff"; }
static FName GetReverbSendParamName()   { return "Acoustic_ReverbSend"; }
static FName GetSpatialWidthParamName() { return "Acoustic_SpatialWidth"; }
```

---

## Zone & Portal System

### AAcousticZoneVolume

Place in levels to define acoustic regions.

**Properties:**
```cpp
FName ZoneName;
EAcousticZoneType ZoneType;  // SmallRoom, Cave, Cathedral, Forest, etc.
int32 Priority;               // For overlapping zones
float BlendTime;              // Transition duration

// Reverb
float RT60;          // Reverb time
float HFDecay;       // High freq decay
float Density;       // Reflection density
float Diffusion;     // Echo smoothness
float PreDelayMs;    // Initial delay
float RoomSize;      // Size factor

// Source behavior
float DefaultReverbSend;
float ReflectionDensityMod;
float TraceDistanceMod;
```

### Zone Type Presets

| Type | RT60 | HF Decay | Density | Character |
|------|------|----------|---------|-----------|
| SmallRoom | 0.3s | 0.9 | 0.7 | Tight, clear |
| LargeRoom | 0.8s | 0.8 | 0.5 | Spacious |
| Hallway | 1.2s | 0.7 | 0.3 | Elongated echoes |
| Cave | 3.0s | 0.6 | 0.8 | Dense, dark |
| Cathedral | 4.0s | 0.5 | 0.6 | Massive, ethereal |
| Forest | 0.2s | 1.0 | 0.2 | Natural absorption |
| OpenAir | 0.1s | 1.0 | 0.1 | Minimal reverb |

### AAcousticPortalVolume

Defines openings between zones (doors, windows, vents).

**Properties:**
```cpp
bool bIsOpen;              // Current state
float Openness;            // 0-1 for partial opening
float ClosedTransmission;  // Sound through when closed
float ClosedLPFCutoff;     // Muffling when closed
float TransitionTime;      // Animation duration
AAcousticZoneVolume* ZoneA, ZoneB;  // Connected zones
```

**Use Cases:**
- Doors: Toggle open/closed with replicated state
- Windows: Always partially open
- Vents: Small transmission, heavy filtering

---

## Multiplayer Architecture

### Key Rule

**Server replicates events and transforms, clients do ALL acoustic work locally.**

### What IS Replicated

```cpp
// Per Actor
- Transform (position, rotation)
- Velocity (for Doppler)
- Audio state (playing/stopped, looping)

// Per Source
- BaseLoudness
- AcousticLOD
- IsHeroSource
- Importance

// Environment
- Portal states (open/closed)
- Zone changes (collapse, flood, etc.)
```

### What is NOT Replicated

```
- Ray trace results
- Filter parameters
- Reflection data
- Computed acoustic params
```

### Per-Client Processing

Each client:
1. Has own listener position (camera)
2. Runs listener-source raytraces
3. Performs acoustic LOD selection
4. Updates params into MetaSounds/submixes

**Result: Each player gets correct headphone mix from their POV.**

### Replication Components

**UAcousticReplicationComponent:**
```cpp
// Server functions
Server_PlaySound();
Server_StopSound();
Server_SetAcousticLOD(EAcousticLOD NewLOD);

// Replicated state
FReplicatedAcousticState ReplicatedState;
```

**UAcousticPortalReplicationComponent:**
```cpp
// Server functions
Server_SetOpen(bool bOpen);
Server_SetOpenness(float NewOpenness);

// Replicated state
bool bIsOpen;
float Openness;
```

---

## Configuration & Settings

### UAcousticSettings (Project Settings)

```cpp
// Ray Tracing Budget
int32 MaxRaysPerFrame = 200;
int32 MaxReflectionsPerSource = 8;
int32 MaxBounces = 1;
float MaxTraceDistance = 10000.0f;  // 100m

// Update Rates
float OcclusionUpdateRateHz = 30.0f;
float ReflectionUpdateRateHz = 15.0f;
float ZoneUpdateRateHz = 20.0f;

// LOD Thresholds
float BasicLODDistance = 3000.0f;   // 30m
float OffLODDistance = 8000.0f;     // 80m
int32 MaxAdvancedSources = 8;
int32 MaxHeroSources = 2;

// Realism vs Game Mix
float RealismFactor = 0.7f;         // 0=game-friendly, 1=realistic
float MinimumAudibilityDb = -50.0f;
float MaxOcclusionDb = -30.0f;

// Headphone Mode
bool bForceHRTFInHeadphones = true;
float HeadphoneReverbBoost = 1.2f;
float HeadphoneDistanceDamp = 0.85f;
float HeadphoneCrossfeed = 0.15f;

// Collision
ECollisionChannel AudioOcclusionChannel = ECC_Visibility;
bool bUseComplexCollision = false;

// Debug
bool bEnableDebugVisualization = false;
```

### Console Commands

```
Acoustic.Debug.Enable    - Enable debug visualization
Acoustic.Debug.Disable   - Disable debug visualization
Acoustic.Stats           - Print engine statistics
Acoustic.SetHeadphones   - Switch to headphone mode
Acoustic.SetSpeakers     - Switch to speaker mode
```

---

## Performance Optimization

### LOD System

| LOD | Use Case | Cost |
|-----|----------|------|
| Off | Distant/unimportant | None |
| Basic | Most sounds | 1 ray, zone reverb |
| Advanced | Important sounds | 1 + 16 rays |
| Hero | Critical sounds | 1 + 32 rays, 2 bounces |

### Budget Enforcement

```cpp
// Priority calculation
Priority = BaseLoudness * ImportanceMultiplier * (1.0 / Distance);

// Per frame
1. Sort all sources by priority
2. Top sources get full LOD
3. Others are downgraded or use cached data
```

### Caching Strategy

- Occlusion: Cache for 5 frames (configurable)
- Reflections: Cache for 10 frames
- Zone: Update every frame (cheap)

### Performance Targets

| Platform | Max Rays/Frame | Max Sources |
|----------|---------------|-------------|
| PC High | 200 | 32 |
| PC Low | 100 | 16 |
| Console | 150 | 24 |
| Mobile | 50 | 8 |

---

## API Reference

### Blueprint Functions

```cpp
// Get acoustic engine
UAcousticEngineSubsystem* GetAcousticEngine(WorldContext);

// Audio mode
void SetAudioOutputMode(WorldContext, EAudioOutputMode Mode);
EAudioOutputMode GetAudioOutputMode(WorldContext);

// Queries
float TraceOcclusion(WorldContext, Start, End);
FAcousticMaterial GetSurfaceMaterial(WorldContext, Location, Direction);

// Source component
void SetAcousticLOD(EAcousticLOD NewLOD);
void SetImportance(EAcousticImportance NewImportance);
bool HasFlag(EAcousticSourceFlags Flag);
void SetFlag(EAcousticSourceFlags Flag, bool bEnabled);
void ForceUpdate();

// Zone volume
FAcousticZonePreset GetZonePreset();
bool ContainsPoint(FVector Point);

// Portal volume
void SetOpen(bool bOpen);
void SetOpenness(float NewOpenness);
float GetCurrentTransmission();
```

### Events

```cpp
// Subsystem events
FOnAcousticParamsUpdated OnAcousticParamsUpdated;
FOnAcousticZoneChanged OnAcousticZoneChanged;
```

---

## Quick Start Guide

### 1. Enable the Plugin

Enable AcoustiTrace Pro in Project Settings → Plugins.

### 2. Configure Settings

Edit Project Settings → Plugins → AcoustiTrace Pro.

### 3. Add Zone Volumes

Place `AAcousticZoneVolume` actors in your level to define acoustic regions.

### 4. Add Source Components

Add `UAcousticSourceComponent` to actors with audio:
```cpp
// In Blueprint or C++
AActor* MyActor = ...;
UAcousticSourceComponent* AcousticSource =
    MyActor->AddComponent<UAcousticSourceComponent>();
AcousticSource->AcousticLOD = EAcousticLOD::Advanced;
AcousticSource->Importance = EAcousticImportance::Important;
```

### 5. Setup MetaSound

Use the Acoustic Processor node in your MetaSound graphs, binding parameters:
- `Acoustic_Occlusion`
- `Acoustic_LPFCutoff`
- `Acoustic_ReverbSend`
- `Acoustic_SpatialWidth`

### 6. Configure Submixes

Create the required submix structure and route audio appropriately.

---

## Version History

- **v1.0.0** - Initial release for UE5.6
  - Core acoustic engine
  - Raytraced occlusion
  - Early reflections
  - Zone-based reverb
  - HRTF headphone support
  - Multiplayer architecture
  - MetaSound integration
