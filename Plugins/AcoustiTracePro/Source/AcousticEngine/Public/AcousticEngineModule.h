// Copyright AcoustiTrace Pro. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAcousticEngine, Log, All);

/**
 * Acoustic Engine Module
 *
 * Main module for the AcoustiTrace Pro plugin.
 * Handles initialization, MetaSound node registration,
 * and global acoustic system setup.
 */
class FAcousticEngineModule : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    /** Get the module singleton */
    static FAcousticEngineModule& Get();

    /** Check if module is loaded */
    static bool IsAvailable();

private:
    /** Register MetaSound nodes */
    void RegisterMetaSoundNodes();

    /** Unregister MetaSound nodes */
    void UnregisterMetaSoundNodes();

    /** Register default material mappings */
    void RegisterDefaultMaterials();

    /** Register console commands */
    void RegisterConsoleCommands();

    /** Console command handles */
    TArray<IConsoleObject*> ConsoleCommands;
};
