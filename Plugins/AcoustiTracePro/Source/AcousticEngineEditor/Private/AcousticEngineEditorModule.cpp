// Copyright AcoustiTrace Pro. All Rights Reserved.

#include "AcousticEngineEditorModule.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"

#define LOCTEXT_NAMESPACE "FAcousticEngineEditorModule"

void FAcousticEngineEditorModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("AcoustiTrace Pro - Editor Module Starting"));

    // Register visualizers
    RegisterVisualizers();

    // Register property customizations
    RegisterPropertyCustomizations();

    // Register placement mode items
    RegisterPlacementModeItems();

    UE_LOG(LogTemp, Log, TEXT("AcoustiTrace Pro - Editor Module Started"));
}

void FAcousticEngineEditorModule::ShutdownModule()
{
    UE_LOG(LogTemp, Log, TEXT("AcoustiTrace Pro - Editor Module Shutting Down"));

    // Unregister visualizers
    UnregisterVisualizers();

    UE_LOG(LogTemp, Log, TEXT("AcoustiTrace Pro - Editor Module Shut Down"));
}

void FAcousticEngineEditorModule::RegisterVisualizers()
{
    // Register custom component visualizers for acoustic sources and zones
    // This will draw debug shapes in the editor viewport
    // Implementation would register FComponentVisualizer subclasses
}

void FAcousticEngineEditorModule::UnregisterVisualizers()
{
    // Unregister all visualizers
    if (GUnrealEd)
    {
        for (const FName& Name : RegisteredVisualizerNames)
        {
            GUnrealEd->UnregisterComponentVisualizer(Name);
        }
    }
    RegisteredVisualizerNames.Empty();
}

void FAcousticEngineEditorModule::RegisterPropertyCustomizations()
{
    // Register custom property editors for acoustic types
    // This would customize how FAcousticMaterial, FAcousticSourceParams, etc. appear in details panels
}

void FAcousticEngineEditorModule::RegisterPlacementModeItems()
{
    // Register acoustic actors in the Place Actors panel under a custom "Audio" category
    // AAcousticZoneVolume, AAcousticPortalVolume
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAcousticEngineEditorModule, AcousticEngineEditor)
