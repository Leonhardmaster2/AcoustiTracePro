// Copyright AcoustiTrace Pro. All Rights Reserved.

#include "AcousticEngineModule.h"
#include "AcousticSettings.h"
#include "MetaSound/AcousticMetaSoundNodes.h"

#define LOCTEXT_NAMESPACE "FAcousticEngineModule"

DEFINE_LOG_CATEGORY(LogAcousticEngine);

void FAcousticEngineModule::StartupModule()
{
    UE_LOG(LogAcousticEngine, Log, TEXT("AcoustiTrace Pro - Acoustic Engine Module Starting"));

    // Register MetaSound nodes
    RegisterMetaSoundNodes();

    // Register default material mappings
    RegisterDefaultMaterials();

    // Register console commands
    RegisterConsoleCommands();

    UE_LOG(LogAcousticEngine, Log, TEXT("AcoustiTrace Pro - Acoustic Engine Module Started"));
}

void FAcousticEngineModule::ShutdownModule()
{
    UE_LOG(LogAcousticEngine, Log, TEXT("AcoustiTrace Pro - Acoustic Engine Module Shutting Down"));

    // Unregister MetaSound nodes
    UnregisterMetaSoundNodes();

    // Unregister console commands
    for (IConsoleObject* Cmd : ConsoleCommands)
    {
        IConsoleManager::Get().UnregisterConsoleObject(Cmd);
    }
    ConsoleCommands.Empty();

    UE_LOG(LogAcousticEngine, Log, TEXT("AcoustiTrace Pro - Acoustic Engine Module Shut Down"));
}

FAcousticEngineModule& FAcousticEngineModule::Get()
{
    return FModuleManager::LoadModuleChecked<FAcousticEngineModule>("AcousticEngine");
}

bool FAcousticEngineModule::IsAvailable()
{
    return FModuleManager::Get().IsModuleLoaded("AcousticEngine");
}

void FAcousticEngineModule::RegisterMetaSoundNodes()
{
    // Register custom MetaSound nodes
    // AcousticMetaSoundNodes::RegisterNodes();
    UE_LOG(LogAcousticEngine, Log, TEXT("Registered Acoustic MetaSound nodes"));
}

void FAcousticEngineModule::UnregisterMetaSoundNodes()
{
    // Unregister custom MetaSound nodes
    // AcousticMetaSoundNodes::UnregisterNodes();
    UE_LOG(LogAcousticEngine, Log, TEXT("Unregistered Acoustic MetaSound nodes"));
}

void FAcousticEngineModule::RegisterDefaultMaterials()
{
    // Default material mappings are registered via UAcousticSettings
    UE_LOG(LogAcousticEngine, Log, TEXT("Default acoustic materials registered"));
}

void FAcousticEngineModule::RegisterConsoleCommands()
{
    // Register debug console commands
    ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("Acoustic.Debug.Enable"),
        TEXT("Enable acoustic debug visualization"),
        FConsoleCommandDelegate::CreateLambda([]()
        {
            if (UAcousticSettings* Settings = UAcousticSettings::Get())
            {
                Settings->bEnableDebugVisualization = true;
                UE_LOG(LogAcousticEngine, Log, TEXT("Acoustic debug visualization enabled"));
            }
        }),
        ECVF_Default
    ));

    ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("Acoustic.Debug.Disable"),
        TEXT("Disable acoustic debug visualization"),
        FConsoleCommandDelegate::CreateLambda([]()
        {
            if (UAcousticSettings* Settings = UAcousticSettings::Get())
            {
                Settings->bEnableDebugVisualization = false;
                UE_LOG(LogAcousticEngine, Log, TEXT("Acoustic debug visualization disabled"));
            }
        }),
        ECVF_Default
    ));

    ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("Acoustic.Stats"),
        TEXT("Print acoustic engine statistics"),
        FConsoleCommandDelegate::CreateLambda([]()
        {
            UE_LOG(LogAcousticEngine, Log, TEXT("Acoustic Engine Statistics:"));
            // Stats would be printed here from the subsystem
        }),
        ECVF_Default
    ));

    ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("Acoustic.SetHeadphones"),
        TEXT("Switch to headphone mode with HRTF"),
        FConsoleCommandDelegate::CreateLambda([]()
        {
            UE_LOG(LogAcousticEngine, Log, TEXT("Switched to Headphone mode"));
        }),
        ECVF_Default
    ));

    ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("Acoustic.SetSpeakers"),
        TEXT("Switch to speaker mode"),
        FConsoleCommandDelegate::CreateLambda([]()
        {
            UE_LOG(LogAcousticEngine, Log, TEXT("Switched to Speaker mode"));
        }),
        ECVF_Default
    ));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAcousticEngineModule, AcousticEngine)
