// Copyright AcoustiTrace Pro. All Rights Reserved.

#include "MetaSound/AcousticMetaSoundNodes.h"
#include "AcousticEngineModule.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundParamHelper.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundPrimitives.h"
#include "MetasoundAudioBuffer.h"

#define LOCTEXT_NAMESPACE "AcousticMetaSoundNodes"

namespace Metasound
{
    // ========================================================================
    // ACOUSTIC OCCLUSION FILTER OPERATOR
    // ========================================================================

    class FAcousticOcclusionFilterOperator : public TExecutableOperator<FAcousticOcclusionFilterOperator>
    {
    public:
        // Parameter names
        static const FVertexName& GetAudioInputName()
        {
            static FVertexName Name = TEXT("Audio");
            return Name;
        }

        static const FVertexName& GetOcclusionInputName()
        {
            static FVertexName Name = TEXT("Occlusion");
            return Name;
        }

        static const FVertexName& GetLPFCutoffInputName()
        {
            static FVertexName Name = TEXT("LPF Cutoff");
            return Name;
        }

        static const FVertexName& GetGainReductionInputName()
        {
            static FVertexName Name = TEXT("Gain Reduction");
            return Name;
        }

        static const FVertexName& GetAudioOutputName()
        {
            static FVertexName Name = TEXT("Audio Out");
            return Name;
        }

        static const FNodeClassMetadata& GetNodeInfo()
        {
            auto InitNodeInfo = []() -> FNodeClassMetadata
            {
                FNodeClassMetadata Info;
                Info.ClassName = { TEXT("UE"), TEXT("AcousticOcclusionFilter"), TEXT("Audio") };
                Info.MajorVersion = 1;
                Info.MinorVersion = 0;
                Info.DisplayName = LOCTEXT("AcousticOcclusionFilterDisplayName", "Acoustic Occlusion Filter");
                Info.Description = LOCTEXT("AcousticOcclusionFilterDescription", "Applies occlusion-based filtering and gain reduction");
                Info.Author = TEXT("AcoustiTrace Pro");
                Info.CategoryHierarchy = { LOCTEXT("AcousticCategory", "Acoustic") };
                Info.Keywords = { };
                return Info;
            };

            static const FNodeClassMetadata Info = InitNodeInfo();
            return Info;
        }

        static const FVertexInterface& GetVertexInterface()
        {
            static const FVertexInterface Interface(
                FInputVertexInterface(
                    TInputDataVertex<FAudioBuffer>(GetAudioInputName(), FDataVertexMetadata{ LOCTEXT("AudioInputTT", "Input audio signal") }),
                    TInputDataVertex<float>(GetOcclusionInputName(), FDataVertexMetadata{ LOCTEXT("OcclusionInputTT", "Occlusion amount (0-1)") }, 0.0f),
                    TInputDataVertex<float>(GetLPFCutoffInputName(), FDataVertexMetadata{ LOCTEXT("LPFCutoffInputTT", "Low-pass filter cutoff frequency (Hz)") }, 20000.0f),
                    TInputDataVertex<float>(GetGainReductionInputName(), FDataVertexMetadata{ LOCTEXT("GainReductionInputTT", "Additional gain reduction (dB)") }, 0.0f)
                ),
                FOutputVertexInterface(
                    TOutputDataVertex<FAudioBuffer>(GetAudioOutputName(), FDataVertexMetadata{ LOCTEXT("AudioOutputTT", "Filtered audio signal") })
                )
            );
            return Interface;
        }

        static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
        {
            const FInputVertexInterfaceData& InputData = InParams.InputData;

            FAudioBufferReadRef AudioIn = InputData.GetOrCreateDefaultDataReadReference<FAudioBuffer>(
                GetAudioInputName(), InParams.OperatorSettings);
            FFloatReadRef OcclusionIn = InputData.GetOrCreateDefaultDataReadReference<float>(
                GetOcclusionInputName(), InParams.OperatorSettings);
            FFloatReadRef LPFCutoffIn = InputData.GetOrCreateDefaultDataReadReference<float>(
                GetLPFCutoffInputName(), InParams.OperatorSettings);
            FFloatReadRef GainReductionIn = InputData.GetOrCreateDefaultDataReadReference<float>(
                GetGainReductionInputName(), InParams.OperatorSettings);

            return MakeUnique<FAcousticOcclusionFilterOperator>(
                InParams.OperatorSettings,
                AudioIn,
                OcclusionIn,
                LPFCutoffIn,
                GainReductionIn
            );
        }

        FAcousticOcclusionFilterOperator(
            const FOperatorSettings& InSettings,
            const FAudioBufferReadRef& InAudio,
            const FFloatReadRef& InOcclusion,
            const FFloatReadRef& InLPFCutoff,
            const FFloatReadRef& InGainReduction)
            : AudioInput(InAudio)
            , OcclusionInput(InOcclusion)
            , LPFCutoffInput(InLPFCutoff)
            , GainReductionInput(InGainReduction)
            , AudioOutput(FAudioBufferWriteRef::CreateNew(InSettings))
            , SampleRate(InSettings.GetSampleRate())
            , CurrentLPFCoeff(0.0f)
            , FilterState(0.0f)
            , SmoothedGain(1.0f)
        {
        }

        virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override
        {
            InOutVertexData.BindReadVertex(GetAudioInputName(), AudioInput);
            InOutVertexData.BindReadVertex(GetOcclusionInputName(), OcclusionInput);
            InOutVertexData.BindReadVertex(GetLPFCutoffInputName(), LPFCutoffInput);
            InOutVertexData.BindReadVertex(GetGainReductionInputName(), GainReductionInput);
        }

        virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
        {
            InOutVertexData.BindReadVertex(GetAudioOutputName(), AudioOutput);
        }

        void Execute()
        {
            const float* InputData = AudioInput->GetData();
            float* OutputData = AudioOutput->GetData();
            const int32 NumSamples = AudioInput->Num();

            // Get parameters
            const float Occlusion = FMath::Clamp(*OcclusionInput, 0.0f, 1.0f);
            const float LPFCutoff = FMath::Clamp(*LPFCutoffInput, 20.0f, 20000.0f);
            const float GainReductionDb = FMath::Clamp(*GainReductionInput, -60.0f, 0.0f);

            // Calculate LPF coefficient (simple one-pole)
            float TargetLPFCoeff = FMath::Exp(-2.0f * PI * LPFCutoff / SampleRate);

            // Calculate target gain
            float OcclusionGainDb = -Occlusion * 20.0f; // Up to -20dB from occlusion
            float TargetGain = FMath::Pow(10.0f, (OcclusionGainDb + GainReductionDb) / 20.0f);

            // Process samples with smoothing
            float LPFCoeffSmooth = 0.999f;
            float GainSmooth = 0.9995f;

            for (int32 i = 0; i < NumSamples; i++)
            {
                // Smooth parameters
                CurrentLPFCoeff = LPFCoeffSmooth * CurrentLPFCoeff + (1.0f - LPFCoeffSmooth) * TargetLPFCoeff;
                SmoothedGain = GainSmooth * SmoothedGain + (1.0f - GainSmooth) * TargetGain;

                // Apply one-pole LPF: y[n] = (1-a)*x[n] + a*y[n-1]
                float Input = InputData[i];
                FilterState = (1.0f - CurrentLPFCoeff) * Input + CurrentLPFCoeff * FilterState;

                // Apply gain
                OutputData[i] = FilterState * SmoothedGain;
            }
        }

        void Reset(const IOperator::FResetParams& InParams)
        {
            FilterState = 0.0f;
            SmoothedGain = 1.0f;
            CurrentLPFCoeff = 0.0f;
        }

    private:
        FAudioBufferReadRef AudioInput;
        FFloatReadRef OcclusionInput;
        FFloatReadRef LPFCutoffInput;
        FFloatReadRef GainReductionInput;

        FAudioBufferWriteRef AudioOutput;

        float SampleRate;
        float CurrentLPFCoeff;
        float FilterState;
        float SmoothedGain;
    };

    // ========================================================================
    // ACOUSTIC SPATIAL WIDTH OPERATOR
    // ========================================================================

    class FAcousticSpatialWidthOperator : public TExecutableOperator<FAcousticSpatialWidthOperator>
    {
    public:
        static const FVertexName& GetAudioLeftInputName()
        {
            static FVertexName Name = TEXT("Audio L");
            return Name;
        }

        static const FVertexName& GetAudioRightInputName()
        {
            static FVertexName Name = TEXT("Audio R");
            return Name;
        }

        static const FVertexName& GetWidthInputName()
        {
            static FVertexName Name = TEXT("Width");
            return Name;
        }

        static const FVertexName& GetDecorrelationInputName()
        {
            static FVertexName Name = TEXT("Decorrelation");
            return Name;
        }

        static const FVertexName& GetAudioLeftOutputName()
        {
            static FVertexName Name = TEXT("Audio Out L");
            return Name;
        }

        static const FVertexName& GetAudioRightOutputName()
        {
            static FVertexName Name = TEXT("Audio Out R");
            return Name;
        }

        static const FNodeClassMetadata& GetNodeInfo()
        {
            auto InitNodeInfo = []() -> FNodeClassMetadata
            {
                FNodeClassMetadata Info;
                Info.ClassName = { TEXT("UE"), TEXT("AcousticSpatialWidth"), TEXT("Audio") };
                Info.MajorVersion = 1;
                Info.MinorVersion = 0;
                Info.DisplayName = LOCTEXT("AcousticSpatialWidthDisplayName", "Acoustic Spatial Width");
                Info.Description = LOCTEXT("AcousticSpatialWidthDescription", "Controls spatial width from point source to diffuse");
                Info.Author = TEXT("AcoustiTrace Pro");
                Info.CategoryHierarchy = { LOCTEXT("AcousticCategory", "Acoustic") };
                return Info;
            };

            static const FNodeClassMetadata Info = InitNodeInfo();
            return Info;
        }

        static const FVertexInterface& GetVertexInterface()
        {
            static const FVertexInterface Interface(
                FInputVertexInterface(
                    TInputDataVertex<FAudioBuffer>(GetAudioLeftInputName(), FDataVertexMetadata{ LOCTEXT("AudioLInTT", "Left input") }),
                    TInputDataVertex<FAudioBuffer>(GetAudioRightInputName(), FDataVertexMetadata{ LOCTEXT("AudioRInTT", "Right input") }),
                    TInputDataVertex<float>(GetWidthInputName(), FDataVertexMetadata{ LOCTEXT("WidthInTT", "Spatial width (0=point, 1=diffuse)") }, 0.0f),
                    TInputDataVertex<float>(GetDecorrelationInputName(), FDataVertexMetadata{ LOCTEXT("DecorrInTT", "Decorrelation amount") }, 0.5f)
                ),
                FOutputVertexInterface(
                    TOutputDataVertex<FAudioBuffer>(GetAudioLeftOutputName(), FDataVertexMetadata{ LOCTEXT("AudioLOutTT", "Left output") }),
                    TOutputDataVertex<FAudioBuffer>(GetAudioRightOutputName(), FDataVertexMetadata{ LOCTEXT("AudioROutTT", "Right output") })
                )
            );
            return Interface;
        }

        static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutResults)
        {
            const FInputVertexInterfaceData& InputData = InParams.InputData;

            FAudioBufferReadRef AudioInL = InputData.GetOrCreateDefaultDataReadReference<FAudioBuffer>(
                GetAudioLeftInputName(), InParams.OperatorSettings);
            FAudioBufferReadRef AudioInR = InputData.GetOrCreateDefaultDataReadReference<FAudioBuffer>(
                GetAudioRightInputName(), InParams.OperatorSettings);
            FFloatReadRef WidthIn = InputData.GetOrCreateDefaultDataReadReference<float>(
                GetWidthInputName(), InParams.OperatorSettings);
            FFloatReadRef DecorrelationIn = InputData.GetOrCreateDefaultDataReadReference<float>(
                GetDecorrelationInputName(), InParams.OperatorSettings);

            return MakeUnique<FAcousticSpatialWidthOperator>(
                InParams.OperatorSettings,
                AudioInL,
                AudioInR,
                WidthIn,
                DecorrelationIn
            );
        }

        FAcousticSpatialWidthOperator(
            const FOperatorSettings& InSettings,
            const FAudioBufferReadRef& InAudioL,
            const FAudioBufferReadRef& InAudioR,
            const FFloatReadRef& InWidth,
            const FFloatReadRef& InDecorrelation)
            : AudioInputL(InAudioL)
            , AudioInputR(InAudioR)
            , WidthInput(InWidth)
            , DecorrelationInput(InDecorrelation)
            , AudioOutputL(FAudioBufferWriteRef::CreateNew(InSettings))
            , AudioOutputR(FAudioBufferWriteRef::CreateNew(InSettings))
            , SampleRate(InSettings.GetSampleRate())
            , DelayWriteIndex(0)
            , AllpassStateL(0.0f)
            , AllpassStateR(0.0f)
        {
            // Initialize decorrelation delay lines (~20ms max)
            int32 MaxDelaySamples = FMath::CeilToInt(SampleRate * 0.02f);
            DecorrelationDelayL.SetNumZeroed(MaxDelaySamples);
            DecorrelationDelayR.SetNumZeroed(MaxDelaySamples);
        }

        virtual void BindInputs(FInputVertexInterfaceData& InOutVertexData) override
        {
            InOutVertexData.BindReadVertex(GetAudioLeftInputName(), AudioInputL);
            InOutVertexData.BindReadVertex(GetAudioRightInputName(), AudioInputR);
            InOutVertexData.BindReadVertex(GetWidthInputName(), WidthInput);
            InOutVertexData.BindReadVertex(GetDecorrelationInputName(), DecorrelationInput);
        }

        virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexData) override
        {
            InOutVertexData.BindReadVertex(GetAudioLeftOutputName(), AudioOutputL);
            InOutVertexData.BindReadVertex(GetAudioRightOutputName(), AudioOutputR);
        }

        void Execute()
        {
            const float* InputL = AudioInputL->GetData();
            const float* InputR = AudioInputR->GetData();
            float* OutputL = AudioOutputL->GetData();
            float* OutputR = AudioOutputR->GetData();
            const int32 NumSamples = AudioInputL->Num();

            const float Width = FMath::Clamp(*WidthInput, 0.0f, 1.0f);
            const float Decorrelation = FMath::Clamp(*DecorrelationInput, 0.0f, 1.0f);

            // Delay times for decorrelation
            int32 DelayL = FMath::RoundToInt(7.3f * SampleRate / 1000.0f); // 7.3ms
            int32 DelayR = FMath::RoundToInt(11.7f * SampleRate / 1000.0f); // 11.7ms

            for (int32 i = 0; i < NumSamples; i++)
            {
                float InL = InputL[i];
                float InR = InputR[i];

                // Create mono sum for narrow sounds
                float Mono = (InL + InR) * 0.5f;

                // Write to delay lines
                DecorrelationDelayL[DelayWriteIndex] = InL;
                DecorrelationDelayR[DelayWriteIndex] = InR;

                // Read delayed and apply allpass for decorrelation
                int32 ReadIndexL = (DelayWriteIndex - DelayL + DecorrelationDelayL.Num()) % DecorrelationDelayL.Num();
                int32 ReadIndexR = (DelayWriteIndex - DelayR + DecorrelationDelayR.Num()) % DecorrelationDelayR.Num();

                float DelayedL = DecorrelationDelayL[ReadIndexL];
                float DelayedR = DecorrelationDelayR[ReadIndexR];

                // Simple allpass for phase dispersion
                float AllpassCoeff = 0.5f * Decorrelation;
                float AllpassOutL = -InL * AllpassCoeff + AllpassStateL + DelayedL * AllpassCoeff;
                float AllpassOutR = -InR * AllpassCoeff + AllpassStateR + DelayedR * AllpassCoeff;
                AllpassStateL = InL + AllpassOutL * AllpassCoeff;
                AllpassStateR = InR + AllpassOutR * AllpassCoeff;

                // Create decorrelated wide signal
                float WideL = AllpassOutL;
                float WideR = AllpassOutR;

                // Blend between mono (narrow) and decorrelated (wide) based on Width
                OutputL[i] = FMath::Lerp(Mono, WideL, Width);
                OutputR[i] = FMath::Lerp(Mono, WideR, Width);

                DelayWriteIndex = (DelayWriteIndex + 1) % DecorrelationDelayL.Num();
            }
        }

        void Reset(const IOperator::FResetParams& InParams)
        {
            FMemory::Memzero(DecorrelationDelayL.GetData(), DecorrelationDelayL.Num() * sizeof(float));
            FMemory::Memzero(DecorrelationDelayR.GetData(), DecorrelationDelayR.Num() * sizeof(float));
            DelayWriteIndex = 0;
            AllpassStateL = 0.0f;
            AllpassStateR = 0.0f;
        }

    private:
        FAudioBufferReadRef AudioInputL;
        FAudioBufferReadRef AudioInputR;
        FFloatReadRef WidthInput;
        FFloatReadRef DecorrelationInput;

        FAudioBufferWriteRef AudioOutputL;
        FAudioBufferWriteRef AudioOutputR;

        float SampleRate;
        TArray<float> DecorrelationDelayL;
        TArray<float> DecorrelationDelayR;
        int32 DelayWriteIndex;
        float AllpassStateL;
        float AllpassStateR;
    };

    // Register nodes
    METASOUND_REGISTER_NODE(FAcousticOcclusionFilterOperator);
    METASOUND_REGISTER_NODE(FAcousticSpatialWidthOperator);

} // namespace Metasound

// ============================================================================
// NODE REGISTRATION
// ============================================================================

namespace AcousticMetaSoundNodes
{
    void RegisterNodes()
    {
        UE_LOG(LogAcousticEngine, Log, TEXT("Registering Acoustic MetaSound nodes"));
        // Nodes are registered via METASOUND_REGISTER_NODE macros above
    }

    void UnregisterNodes()
    {
        UE_LOG(LogAcousticEngine, Log, TEXT("Unregistering Acoustic MetaSound nodes"));
        // MetaSound handles unregistration automatically
    }
}

#undef LOCTEXT_NAMESPACE
