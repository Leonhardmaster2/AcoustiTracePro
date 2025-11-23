// Copyright AcoustiTrace Pro. All Rights Reserved.

#include "AcousticSubmixEffects.h"
#include "AcousticEngineModule.h"
#include "DSP/FloatArrayMath.h"

// ============================================================================
// ZONE REVERB SETTINGS
// ============================================================================

void FAcousticZoneReverbSettings::InitFromZonePreset(const FAcousticZonePreset& Preset)
{
    RT60 = Preset.RT60;
    PreDelayMs = Preset.PreDelayMs;
    HFDecay = Preset.HFDecay;
    LFDecay = Preset.LFDecay;
    Density = Preset.Density;
    Diffusion = Preset.Diffusion;
    EarlyLevel = Preset.EarlyReflectionLevel;
    LateLevel = Preset.LateReverbLevel;
    RoomSize = Preset.RoomSize;
    WetLevel = Preset.DefaultReverbSend;
}

// ============================================================================
// ZONE REVERB PRESET
// ============================================================================

void UAcousticZoneReverbPreset::SetSettings(const FAcousticZoneReverbSettings& InSettings)
{
    UpdateSettings(InSettings);
}

void UAcousticZoneReverbPreset::SetFromZonePreset(const FAcousticZonePreset& ZonePreset)
{
    FAcousticZoneReverbSettings NewSettings;
    NewSettings.InitFromZonePreset(ZonePreset);
    UpdateSettings(NewSettings);
}

// ============================================================================
// ZONE REVERB EFFECT
// ============================================================================

void FAcousticZoneReverbEffect::Init(const FSoundEffectSubmixInitData& InitData)
{
    SampleRate = InitData.SampleRate;
    NumChannels = InitData.NumOutputChannels;

    InitializeDSP();
}

void FAcousticZoneReverbEffect::OnPresetChanged()
{
    UAcousticZoneReverbPreset* Preset = CastChecked<UAcousticZoneReverbPreset>(GetPreset());
    TargetSettings = Preset->Settings;

    if (FMath::IsNearlyZero(CurrentSettings.RT60))
    {
        // First time - apply immediately
        CurrentSettings = TargetSettings;
    }
    else
    {
        // Start blending
        bIsBlending = true;
        BlendProgress = 0.0f;
    }
}

void FAcousticZoneReverbEffect::SetTargetSettings(const FAcousticZoneReverbSettings& InSettings)
{
    TargetSettings = InSettings;
    bIsBlending = true;
    BlendProgress = 0.0f;
}

void FAcousticZoneReverbEffect::InitializeDSP()
{
    // Initialize pre-delay buffer (up to 100ms)
    int32 MaxPreDelaySamples = FMath::CeilToInt(SampleRate * 0.1f);
    PreDelayBuffer.SetNumZeroed(MaxPreDelaySamples * NumChannels);
    PreDelayWriteIndex = 0;

    // Initialize early reflection taps
    EarlyTaps.SetNum(8);
    float TapDelays[] = { 5.3f, 7.9f, 11.2f, 16.4f, 22.1f, 29.7f, 38.5f, 49.8f };
    float TapGains[] = { 0.841f, 0.782f, 0.733f, 0.691f, 0.632f, 0.578f, 0.501f, 0.422f };
    float TapPans[] = { -0.5f, 0.5f, -0.3f, 0.3f, -0.7f, 0.7f, -0.2f, 0.2f };

    for (int32 i = 0; i < 8; i++)
    {
        EarlyTaps[i].DelayMs = TapDelays[i];
        EarlyTaps[i].Gain = TapGains[i];
        EarlyTaps[i].Pan = TapPans[i];
    }

    // Initialize allpass diffusers
    int32 DiffuserDelays[] = { 142, 107, 379, 277 };
    Diffusers.SetNum(4);
    for (int32 i = 0; i < 4; i++)
    {
        Diffusers[i].Buffer.SetNumZeroed(DiffuserDelays[i] * NumChannels);
        Diffusers[i].WriteIndex = 0;
        Diffusers[i].Feedback = 0.5f;
    }

    // Initialize FDN tanks (4 tanks for simple reverb)
    int32 TankDelays[] = { 1557, 1617, 1491, 1422 };
    FDNTanks.SetNum(4);
    for (int32 i = 0; i < 4; i++)
    {
        int32 DelaySize = FMath::CeilToInt(TankDelays[i] * (SampleRate / 44100.0f));
        FDNTanks[i].Buffer.SetNumZeroed(DelaySize * NumChannels);
        FDNTanks[i].WriteIndex = 0;
        FDNTanks[i].Feedback = 0.0f; // Will be calculated from RT60
        FDNTanks[i].LPFState = 0.0f;
        FDNTanks[i].HPFState = 0.0f;
    }
}

void FAcousticZoneReverbEffect::OnProcessAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData)
{
    const float* InBuffer = InData.AudioBuffer->GetData();
    float* OutBuffer = OutData.AudioBuffer->GetData();
    const int32 NumFrames = InData.NumFrames;
    const int32 NumChannels = InData.NumChannels;

    // Update blend if active
    if (bIsBlending)
    {
        UpdateBlend(NumFrames);
    }

    // Get current settings (interpolated if blending)
    FAcousticZoneReverbSettings ActiveSettings = bIsBlending ?
        InterpolateSettings(BlendProgress) : CurrentSettings;

    // Calculate derived parameters
    float DryMix = 1.0f - ActiveSettings.WetLevel;
    float WetMix = ActiveSettings.WetLevel;

    // Process each frame
    for (int32 Frame = 0; Frame < NumFrames; Frame++)
    {
        // Get input sample (sum to mono for reverb input)
        float InputL = InBuffer[Frame * NumChannels];
        float InputR = NumChannels > 1 ? InBuffer[Frame * NumChannels + 1] : InputL;
        float MonoInput = (InputL + InputR) * 0.5f;

        // Early reflections
        float EarlyL = 0.0f;
        float EarlyR = 0.0f;
        ProcessEarlyReflectionsSample(MonoInput, EarlyL, EarlyR, ActiveSettings);

        // Late reverb via FDN
        float LateL = 0.0f;
        float LateR = 0.0f;
        ProcessLateReverbSample(MonoInput + EarlyL * 0.3f, LateL, LateR, ActiveSettings);

        // Mix wet signals
        float WetL = (EarlyL * ActiveSettings.EarlyLevel + LateL * ActiveSettings.LateLevel);
        float WetR = (EarlyR * ActiveSettings.EarlyLevel + LateR * ActiveSettings.LateLevel);

        // Apply stereo width
        float Mid = (WetL + WetR) * 0.5f;
        float Side = (WetL - WetR) * 0.5f * ActiveSettings.StereoWidth;
        WetL = Mid + Side;
        WetR = Mid - Side;

        // Final mix
        OutBuffer[Frame * NumChannels] = InputL * DryMix + WetL * WetMix;
        if (NumChannels > 1)
        {
            OutBuffer[Frame * NumChannels + 1] = InputR * DryMix + WetR * WetMix;
        }
    }
}

void FAcousticZoneReverbEffect::ProcessEarlyReflections(const float* InBuffer, float* OutBuffer, int32 NumFrames)
{
    // Batch processing version - simplified for clarity
    FMemory::Memcpy(OutBuffer, InBuffer, NumFrames * NumChannels * sizeof(float));
}

void FAcousticZoneReverbEffect::ProcessEarlyReflectionsSample(float Input, float& OutL, float& OutR, const FAcousticZoneReverbSettings& Settings)
{
    OutL = 0.0f;
    OutR = 0.0f;

    // Write to pre-delay buffer
    int32 PreDelaySamples = FMath::RoundToInt(Settings.PreDelayMs * SampleRate / 1000.0f);
    PreDelaySamples = FMath::Clamp(PreDelaySamples, 1, PreDelayBuffer.Num() - 1);

    PreDelayBuffer[PreDelayWriteIndex] = Input;
    int32 ReadIndex = (PreDelayWriteIndex - PreDelaySamples + PreDelayBuffer.Num()) % PreDelayBuffer.Num();
    float DelayedInput = PreDelayBuffer[ReadIndex];
    PreDelayWriteIndex = (PreDelayWriteIndex + 1) % PreDelayBuffer.Num();

    // Sum early reflection taps
    for (const FEarlyTap& Tap : EarlyTaps)
    {
        int32 TapDelaySamples = FMath::RoundToInt(Tap.DelayMs * Settings.RoomSize * SampleRate / 1000.0f);
        TapDelaySamples = FMath::Clamp(TapDelaySamples, 1, PreDelayBuffer.Num() - 1);

        int32 TapReadIndex = (PreDelayWriteIndex - TapDelaySamples + PreDelayBuffer.Num()) % PreDelayBuffer.Num();
        float TapSample = PreDelayBuffer[TapReadIndex] * Tap.Gain * Settings.Density;

        // Pan
        float LeftGain = 0.5f - Tap.Pan * 0.5f;
        float RightGain = 0.5f + Tap.Pan * 0.5f;

        OutL += TapSample * LeftGain;
        OutR += TapSample * RightGain;
    }
}

void FAcousticZoneReverbEffect::ProcessLateReverb(const float* InBuffer, float* OutBuffer, int32 NumFrames)
{
    // Batch processing version
    FMemory::Memcpy(OutBuffer, InBuffer, NumFrames * NumChannels * sizeof(float));
}

void FAcousticZoneReverbEffect::ProcessLateReverbSample(float Input, float& OutL, float& OutR, const FAcousticZoneReverbSettings& Settings)
{
    // Calculate feedback from RT60
    // Feedback = 10^(-3 * DelayTime / RT60)
    float AvgDelayTime = 0.035f * Settings.RoomSize; // ~35ms average delay scaled by room
    float Feedback = FMath::Pow(10.0f, -3.0f * AvgDelayTime / FMath::Max(Settings.RT60, 0.1f));
    Feedback = FMath::Clamp(Feedback, 0.0f, 0.99f);

    // Calculate filter coefficients from HF/LF decay
    float HFDamping = 1.0f - Settings.HFDecay * 0.5f;
    float LFDamping = 1.0f - Settings.LFDecay * 0.5f;

    // Apply input through diffusers
    float DiffusedInput = Input;
    for (FAllpassDiffuser& Diffuser : Diffusers)
    {
        int32 BufferSize = Diffuser.Buffer.Num();
        if (BufferSize > 0)
        {
            int32 ReadIndex = (Diffuser.WriteIndex + 1) % BufferSize;
            float DelayedSample = Diffuser.Buffer[ReadIndex];

            float OutputSample = -DiffusedInput * Diffuser.Feedback + DelayedSample;
            Diffuser.Buffer[Diffuser.WriteIndex] = DiffusedInput + DelayedSample * Diffuser.Feedback;
            Diffuser.WriteIndex = (Diffuser.WriteIndex + 1) % BufferSize;

            DiffusedInput = OutputSample * Settings.Diffusion + DiffusedInput * (1.0f - Settings.Diffusion);
        }
    }

    // Process through FDN tanks
    float TankOutputs[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    for (int32 i = 0; i < 4; i++)
    {
        FFDNTank& Tank = FDNTanks[i];
        int32 BufferSize = Tank.Buffer.Num();
        if (BufferSize > 0)
        {
            // Read from delay
            int32 ReadIndex = (Tank.WriteIndex + 1) % BufferSize;
            float DelayedSample = Tank.Buffer[ReadIndex];

            // Apply damping filters
            // Simple one-pole LPF: y[n] = (1-a)*x[n] + a*y[n-1]
            Tank.LPFState = (1.0f - HFDamping) * DelayedSample + HFDamping * Tank.LPFState;
            // Simple one-pole HPF: y[n] = a*(y[n-1] + x[n] - x[n-1])
            float HPFInput = Tank.LPFState;
            float HPFOutput = LFDamping * (Tank.HPFState + HPFInput);
            Tank.HPFState = HPFOutput - HPFInput;

            TankOutputs[i] = HPFOutput;

            // Write to delay with feedback
            float FeedbackSum = DiffusedInput;
            // Hadamard-like mixing (simplified)
            FeedbackSum += TankOutputs[(i + 1) % 4] * 0.5f;
            FeedbackSum += TankOutputs[(i + 2) % 4] * -0.5f;
            FeedbackSum += TankOutputs[(i + 3) % 4] * 0.5f;

            Tank.Buffer[Tank.WriteIndex] = FeedbackSum * Feedback;
            Tank.WriteIndex = (Tank.WriteIndex + 1) % BufferSize;
        }
    }

    // Sum tank outputs to stereo (decorrelated)
    OutL = (TankOutputs[0] + TankOutputs[2]) * 0.5f;
    OutR = (TankOutputs[1] + TankOutputs[3]) * 0.5f;
}

void FAcousticZoneReverbEffect::UpdateBlend(int32 NumFrames)
{
    if (!bIsBlending)
    {
        return;
    }

    float BlendTime = FMath::Max(CurrentSettings.BlendTime, 0.01f);
    float FrameTime = NumFrames / SampleRate;
    BlendProgress += FrameTime / BlendTime;

    if (BlendProgress >= 1.0f)
    {
        BlendProgress = 1.0f;
        CurrentSettings = TargetSettings;
        bIsBlending = false;
    }
}

FAcousticZoneReverbSettings FAcousticZoneReverbEffect::InterpolateSettings(float Alpha) const
{
    FAcousticZoneReverbSettings Result;

    Result.RT60 = FMath::Lerp(CurrentSettings.RT60, TargetSettings.RT60, Alpha);
    Result.PreDelayMs = FMath::Lerp(CurrentSettings.PreDelayMs, TargetSettings.PreDelayMs, Alpha);
    Result.HFDecay = FMath::Lerp(CurrentSettings.HFDecay, TargetSettings.HFDecay, Alpha);
    Result.LFDecay = FMath::Lerp(CurrentSettings.LFDecay, TargetSettings.LFDecay, Alpha);
    Result.Density = FMath::Lerp(CurrentSettings.Density, TargetSettings.Density, Alpha);
    Result.Diffusion = FMath::Lerp(CurrentSettings.Diffusion, TargetSettings.Diffusion, Alpha);
    Result.EarlyLevel = FMath::Lerp(CurrentSettings.EarlyLevel, TargetSettings.EarlyLevel, Alpha);
    Result.LateLevel = FMath::Lerp(CurrentSettings.LateLevel, TargetSettings.LateLevel, Alpha);
    Result.WetLevel = FMath::Lerp(CurrentSettings.WetLevel, TargetSettings.WetLevel, Alpha);
    Result.RoomSize = FMath::Lerp(CurrentSettings.RoomSize, TargetSettings.RoomSize, Alpha);
    Result.StereoWidth = FMath::Lerp(CurrentSettings.StereoWidth, TargetSettings.StereoWidth, Alpha);
    Result.BlendTime = CurrentSettings.BlendTime; // Use current blend time

    return Result;
}

// ============================================================================
// HEADPHONE CROSSFEED EFFECT
// ============================================================================

void FHeadphoneCrossfeedEffect::Init(const FSoundEffectSubmixInitData& InitData)
{
    SampleRate = InitData.SampleRate;

    // Initialize delay lines for crossfeed (~300us max)
    int32 MaxDelaySamples = FMath::CeilToInt(SampleRate * 0.0005f);
    CrossfeedDelayL.SetNumZeroed(MaxDelaySamples);
    CrossfeedDelayR.SetNumZeroed(MaxDelaySamples);
    DelayWriteIndex = 0;
}

void FHeadphoneCrossfeedEffect::OnPresetChanged()
{
    UHeadphoneCrossfeedPreset* Preset = CastChecked<UHeadphoneCrossfeedPreset>(GetPreset());
    CurrentSettings = Preset->Settings;
}

void FHeadphoneCrossfeedEffect::OnProcessAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData)
{
    if (!CurrentSettings.bEnabled)
    {
        // Pass through
        FMemory::Memcpy(OutData.AudioBuffer->GetData(), InData.AudioBuffer->GetData(),
            InData.NumFrames * InData.NumChannels * sizeof(float));
        return;
    }

    const float* InBuffer = InData.AudioBuffer->GetData();
    float* OutBuffer = OutData.AudioBuffer->GetData();
    const int32 NumFrames = InData.NumFrames;
    const int32 NumChannels = InData.NumChannels;

    if (NumChannels < 2)
    {
        // Mono - pass through
        FMemory::Memcpy(OutBuffer, InBuffer, NumFrames * sizeof(float));
        return;
    }

    // Calculate delay in samples from microseconds
    int32 DelaySamples = FMath::RoundToInt(CurrentSettings.CrossfeedDelayUs * SampleRate / 1000000.0f);
    DelaySamples = FMath::Clamp(DelaySamples, 1, CrossfeedDelayL.Num() - 1);

    // Calculate LPF coefficient
    float LPFFreq = CurrentSettings.CrossfeedLPFHz;
    float LPFCoeff = FMath::Exp(-2.0f * PI * LPFFreq / SampleRate);

    // Bass boost (simple shelf approximation)
    float BassBoostLinear = FMath::Pow(10.0f, CurrentSettings.BassBoostDb / 20.0f);

    for (int32 Frame = 0; Frame < NumFrames; Frame++)
    {
        float InL = InBuffer[Frame * NumChannels];
        float InR = InBuffer[Frame * NumChannels + 1];

        // Write to delay lines
        CrossfeedDelayL[DelayWriteIndex] = InL;
        CrossfeedDelayR[DelayWriteIndex] = InR;

        // Read delayed samples for crossfeed
        int32 ReadIndex = (DelayWriteIndex - DelaySamples + CrossfeedDelayL.Num()) % CrossfeedDelayL.Num();
        float DelayedL = CrossfeedDelayL[ReadIndex];
        float DelayedR = CrossfeedDelayR[ReadIndex];

        // Apply LPF to crossfeed
        CrossfeedLPFStateL = LPFCoeff * CrossfeedLPFStateL + (1.0f - LPFCoeff) * DelayedR;
        CrossfeedLPFStateR = LPFCoeff * CrossfeedLPFStateR + (1.0f - LPFCoeff) * DelayedL;

        // Mix crossfeed with main signal
        float CrossfeedAmount = CurrentSettings.CrossfeedAmount;
        OutBuffer[Frame * NumChannels] = InL * (1.0f - CrossfeedAmount * 0.5f) +
            CrossfeedLPFStateL * CrossfeedAmount;
        OutBuffer[Frame * NumChannels + 1] = InR * (1.0f - CrossfeedAmount * 0.5f) +
            CrossfeedLPFStateR * CrossfeedAmount;

        DelayWriteIndex = (DelayWriteIndex + 1) % CrossfeedDelayL.Num();
    }
}

// ============================================================================
// ACOUSTIC MASTER EFFECT
// ============================================================================

void FAcousticMasterEffect::Init(const FSoundEffectSubmixInitData& InitData)
{
    SampleRate = InitData.SampleRate;
    LimiterEnvelope = 0.0f;
    LimiterGain = 1.0f;
    SmoothedOutputGain = 1.0f;
}

void FAcousticMasterEffect::OnPresetChanged()
{
    UAcousticMasterPreset* Preset = CastChecked<UAcousticMasterPreset>(GetPreset());
    CurrentSettings = Preset->Settings;
}

void FAcousticMasterEffect::OnProcessAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData)
{
    if (!CurrentSettings.bEnabled)
    {
        FMemory::Memcpy(OutData.AudioBuffer->GetData(), InData.AudioBuffer->GetData(),
            InData.NumFrames * InData.NumChannels * sizeof(float));
        return;
    }

    const float* InBuffer = InData.AudioBuffer->GetData();
    float* OutBuffer = OutData.AudioBuffer->GetData();
    const int32 NumFrames = InData.NumFrames;
    const int32 NumChannels = InData.NumChannels;

    // Calculate target gain from dB
    float TargetGain = FMath::Pow(10.0f, CurrentSettings.OutputGainDb / 20.0f);

    // Limiter threshold
    float LimiterThreshold = FMath::Pow(10.0f, CurrentSettings.LimiterThresholdDb / 20.0f);

    // Limiter attack/release
    float AttackCoeff = FMath::Exp(-1.0f / (SampleRate * 0.001f)); // 1ms attack
    float ReleaseCoeff = FMath::Exp(-1.0f / (SampleRate * 0.1f));  // 100ms release

    // Gain smoothing
    float GainSmoothCoeff = FMath::Exp(-1.0f / (SampleRate * 0.01f)); // 10ms smoothing

    for (int32 Frame = 0; Frame < NumFrames; Frame++)
    {
        // Smooth output gain
        SmoothedOutputGain = GainSmoothCoeff * SmoothedOutputGain + (1.0f - GainSmoothCoeff) * TargetGain;

        // Get max absolute value across channels
        float MaxAbs = 0.0f;
        for (int32 Ch = 0; Ch < NumChannels; Ch++)
        {
            MaxAbs = FMath::Max(MaxAbs, FMath::Abs(InBuffer[Frame * NumChannels + Ch] * SmoothedOutputGain));
        }

        // Envelope follower
        if (MaxAbs > LimiterEnvelope)
        {
            LimiterEnvelope = AttackCoeff * LimiterEnvelope + (1.0f - AttackCoeff) * MaxAbs;
        }
        else
        {
            LimiterEnvelope = ReleaseCoeff * LimiterEnvelope + (1.0f - ReleaseCoeff) * MaxAbs;
        }

        // Calculate gain reduction
        if (LimiterEnvelope > LimiterThreshold)
        {
            LimiterGain = LimiterThreshold / LimiterEnvelope;
        }
        else
        {
            LimiterGain = 1.0f;
        }

        // Apply to output
        float FinalGain = SmoothedOutputGain * LimiterGain;
        for (int32 Ch = 0; Ch < NumChannels; Ch++)
        {
            OutBuffer[Frame * NumChannels + Ch] = InBuffer[Frame * NumChannels + Ch] * FinalGain;
        }
    }
}
