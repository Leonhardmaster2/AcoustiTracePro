// Copyright AcoustiTrace Pro. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "MetasoundParamHelper.h"
#include "MetasoundPrimitives.h"
#include "MetasoundAudioBuffer.h"
#include "MetasoundVertex.h"
#include "AcousticTypes.h"

namespace Metasound
{
    // ========================================================================
    // ACOUSTIC PARAMETER DATA TYPE
    // ========================================================================

    /**
     * MetaSound data type for acoustic parameters
     * Allows passing full acoustic state through MetaSound graphs
     */
    DECLARE_METASOUND_DATA_REFERENCE_TYPES(
        FAcousticSourceParams,
        ACOUSTICENGINE_API,
        FAcousticSourceParamsTypeInfo,
        FAcousticSourceParamsReadRef,
        FAcousticSourceParamsWriteRef
    );

    // ========================================================================
    // OCCLUSION FILTER NODE
    // ========================================================================

    /**
     * Acoustic Occlusion Filter
     *
     * Applies low-pass filtering and gain reduction based on occlusion
     * parameters from the acoustic engine.
     *
     * Inputs:
     * - Audio: Input audio signal
     * - Occlusion: Occlusion amount (0-1)
     * - LPF Cutoff: Low-pass filter cutoff frequency
     * - Gain Reduction: Additional gain reduction in dB
     *
     * Outputs:
     * - Audio: Filtered audio signal
     */
    class ACOUSTICENGINE_API FAcousticOcclusionFilterOperator : public TExecutableOperator<FAcousticOcclusionFilterOperator>
    {
    public:
        static const FNodeClassMetadata& GetNodeInfo();
        static const FVertexInterface& GetVertexInterface();
        static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);

        FAcousticOcclusionFilterOperator(
            const FOperatorSettings& InSettings,
            const FAudioBufferReadRef& InAudio,
            const FFloatReadRef& InOcclusion,
            const FFloatReadRef& InLPFCutoff,
            const FFloatReadRef& InGainReduction
        );

        virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override;
        virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override;
        void Execute();
        void Reset(const IOperator::FResetParams& InParams);

    private:
        // Inputs
        FAudioBufferReadRef AudioInput;
        FFloatReadRef OcclusionInput;
        FFloatReadRef LPFCutoffInput;
        FFloatReadRef GainReductionInput;

        // Outputs
        FAudioBufferWriteRef AudioOutput;

        // DSP state
        float SampleRate;
        float CurrentLPFCoeff;
        float FilterState;
        float SmoothedGain;
    };

    // ========================================================================
    // EARLY REFLECTIONS NODE
    // ========================================================================

    /**
     * Acoustic Early Reflections
     *
     * Generates early reflections using a multi-tap delay line.
     * Tap parameters are driven by the acoustic engine's reflection analysis.
     *
     * Inputs:
     * - Audio: Input audio signal
     * - Tap 0-7 Delay: Delay time in ms for each tap
     * - Tap 0-7 Gain: Gain for each tap
     * - Tap 0-7 LPF: LPF cutoff for each tap
     * - Wet/Dry: Mix amount
     *
     * Outputs:
     * - Audio: Audio with early reflections
     */
    class ACOUSTICENGINE_API FAcousticEarlyReflectionsOperator : public TExecutableOperator<FAcousticEarlyReflectionsOperator>
    {
    public:
        static const FNodeClassMetadata& GetNodeInfo();
        static const FVertexInterface& GetVertexInterface();
        static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);

        FAcousticEarlyReflectionsOperator(
            const FOperatorSettings& InSettings,
            const FAudioBufferReadRef& InAudio,
            const TArray<FFloatReadRef>& InDelays,
            const TArray<FFloatReadRef>& InGains,
            const TArray<FFloatReadRef>& InLPFs,
            const FFloatReadRef& InWetDry
        );

        virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override;
        virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override;
        void Execute();
        void Reset(const IOperator::FResetParams& InParams);

    private:
        static constexpr int32 NumTaps = 8;
        static constexpr int32 MaxDelayMs = 500;

        // Inputs
        FAudioBufferReadRef AudioInput;
        TArray<FFloatReadRef> DelayInputs;
        TArray<FFloatReadRef> GainInputs;
        TArray<FFloatReadRef> LPFInputs;
        FFloatReadRef WetDryInput;

        // Outputs
        FAudioBufferWriteRef AudioOutput;

        // DSP state
        float SampleRate;
        TArray<float> DelayBuffer;
        int32 WriteIndex;
        TArray<float> TapFilterStates;
        TArray<float> SmoothedGains;
    };

    // ========================================================================
    // SPATIAL WIDTH NODE
    // ========================================================================

    /**
     * Acoustic Spatial Width
     *
     * Controls the perceived width of a sound source by blending
     * between mono (point source) and decorrelated stereo.
     *
     * Inputs:
     * - Audio L/R: Input stereo signal
     * - Width: Spatial width (0 = point, 1 = diffuse)
     * - Decorrelation: Amount of decorrelation for wide sounds
     *
     * Outputs:
     * - Audio L/R: Processed stereo signal
     */
    class ACOUSTICENGINE_API FAcousticSpatialWidthOperator : public TExecutableOperator<FAcousticSpatialWidthOperator>
    {
    public:
        static const FNodeClassMetadata& GetNodeInfo();
        static const FVertexInterface& GetVertexInterface();
        static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);

        FAcousticSpatialWidthOperator(
            const FOperatorSettings& InSettings,
            const FAudioBufferReadRef& InAudioL,
            const FAudioBufferReadRef& InAudioR,
            const FFloatReadRef& InWidth,
            const FFloatReadRef& InDecorrelation
        );

        virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override;
        virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override;
        void Execute();
        void Reset(const IOperator::FResetParams& InParams);

    private:
        // Inputs
        FAudioBufferReadRef AudioInputL;
        FAudioBufferReadRef AudioInputR;
        FFloatReadRef WidthInput;
        FFloatReadRef DecorrelationInput;

        // Outputs
        FAudioBufferWriteRef AudioOutputL;
        FAudioBufferWriteRef AudioOutputR;

        // DSP state
        float SampleRate;
        TArray<float> DecorrelationDelayL;
        TArray<float> DecorrelationDelayR;
        int32 DelayWriteIndex;
        float AllpassStateL;
        float AllpassStateR;
    };

    // ========================================================================
    // REVERB SEND CALCULATOR NODE
    // ========================================================================

    /**
     * Acoustic Reverb Send Calculator
     *
     * Calculates appropriate reverb send levels based on
     * distance, occlusion, and zone parameters.
     *
     * Inputs:
     * - Distance: Distance to listener
     * - Occlusion: Current occlusion amount
     * - Zone Reverb Send: Base reverb send from zone
     * - Reflection Density: Detected reflection density
     *
     * Outputs:
     * - Reverb Send: Computed reverb send level
     * - Early Reflection Level: Level for early reflections
     */
    class ACOUSTICENGINE_API FAcousticReverbSendOperator : public TExecutableOperator<FAcousticReverbSendOperator>
    {
    public:
        static const FNodeClassMetadata& GetNodeInfo();
        static const FVertexInterface& GetVertexInterface();
        static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);

        FAcousticReverbSendOperator(
            const FOperatorSettings& InSettings,
            const FFloatReadRef& InDistance,
            const FFloatReadRef& InOcclusion,
            const FFloatReadRef& InZoneReverbSend,
            const FFloatReadRef& InReflectionDensity
        );

        virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override;
        virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override;
        void Execute();
        void Reset(const IOperator::FResetParams& InParams);

    private:
        // Inputs
        FFloatReadRef DistanceInput;
        FFloatReadRef OcclusionInput;
        FFloatReadRef ZoneReverbSendInput;
        FFloatReadRef ReflectionDensityInput;

        // Outputs
        FFloatWriteRef ReverbSendOutput;
        FFloatWriteRef EarlyReflectionLevelOutput;
    };

    // ========================================================================
    // ACOUSTIC PROCESSOR NODE (ALL-IN-ONE)
    // ========================================================================

    /**
     * Acoustic Processor
     *
     * All-in-one acoustic processing node that applies:
     * - Occlusion filtering
     * - Early reflections
     * - Spatial width adjustment
     * - Reverb send calculation
     *
     * This is a convenience node that combines all acoustic
     * processing into a single graph node.
     *
     * Inputs:
     * - Audio: Input mono audio
     * - Acoustic Params: Full acoustic parameter struct
     * - Wet/Dry: Global wet/dry mix
     *
     * Outputs:
     * - Audio L/R: Processed stereo audio
     * - Reverb Send: Computed reverb send amount
     */
    class ACOUSTICENGINE_API FAcousticProcessorOperator : public TExecutableOperator<FAcousticProcessorOperator>
    {
    public:
        static const FNodeClassMetadata& GetNodeInfo();
        static const FVertexInterface& GetVertexInterface();
        static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults);

        FAcousticProcessorOperator(
            const FOperatorSettings& InSettings,
            const FAudioBufferReadRef& InAudio,
            const FFloatReadRef& InOcclusion,
            const FFloatReadRef& InLPFCutoff,
            const FFloatReadRef& InReverbSend,
            const FFloatReadRef& InSpatialWidth,
            const FFloatReadRef& InWetDry
        );

        virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override;
        virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override;
        void Execute();
        void Reset(const IOperator::FResetParams& InParams);

    private:
        // Inputs
        FAudioBufferReadRef AudioInput;
        FFloatReadRef OcclusionInput;
        FFloatReadRef LPFCutoffInput;
        FFloatReadRef ReverbSendInput;
        FFloatReadRef SpatialWidthInput;
        FFloatReadRef WetDryInput;

        // Outputs
        FAudioBufferWriteRef AudioOutputL;
        FAudioBufferWriteRef AudioOutputR;
        FFloatWriteRef ReverbSendOutput;

        // DSP state
        float SampleRate;

        // Occlusion filter state
        float LPFCoeff;
        float LPFState;
        float SmoothedOcclusionGain;

        // Early reflection state
        static constexpr int32 NumTaps = 8;
        TArray<float> ReflectionDelayBuffer;
        int32 ReflectionWriteIndex;
        TArray<float> ReflectionFilterStates;

        // Spatial width state
        TArray<float> DecorrelationDelayL;
        TArray<float> DecorrelationDelayR;
        int32 WidthDelayWriteIndex;
        float AllpassStateL;
        float AllpassStateR;
    };

} // namespace Metasound

// ============================================================================
// NODE REGISTRATION
// ============================================================================

namespace AcousticMetaSoundNodes
{
    /** Register all acoustic MetaSound nodes */
    ACOUSTICENGINE_API void RegisterNodes();

    /** Unregister all acoustic MetaSound nodes */
    ACOUSTICENGINE_API void UnregisterNodes();
}
