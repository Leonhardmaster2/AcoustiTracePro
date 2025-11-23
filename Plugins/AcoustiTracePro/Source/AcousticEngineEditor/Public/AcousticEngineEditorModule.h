// Copyright AcoustiTrace Pro. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * Acoustic Engine Editor Module
 *
 * Provides editor-only functionality:
 * - Custom component visualizers
 * - Zone volume visualization
 * - Debug ray drawing
 * - Property customization
 * - Placement mode integration
 */
class FAcousticEngineEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    /** Register component visualizers */
    void RegisterVisualizers();

    /** Unregister component visualizers */
    void UnregisterVisualizers();

    /** Register custom property editors */
    void RegisterPropertyCustomizations();

    /** Register placement mode items */
    void RegisterPlacementModeItems();

    /** Visualizer registration handles */
    TArray<FName> RegisteredVisualizerNames;
};
