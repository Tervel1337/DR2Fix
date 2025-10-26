#include "Timing.h"
#include "General.h"

LARGE_INTEGER* Timing::Freq;
float Timing::Frametime;
const float Timing::TargetFT = 0.0333333f;

double __cdecl Timing::GetTimeForTick(_LARGE_INTEGER Tick) {
    double Result = (double)Tick.QuadPart / (double)Freq->QuadPart;
    Frametime = Result;
    return Result;
}

void Timing::Install() {
    auto Pattern = Utils::FindPattern("68 ? ? ? ? FF 15 ? ? ? ? 68 ? ? ? ? FF 15 ? ? ? ? 33 C0");
    Freq = *(LARGE_INTEGER**)(Pattern.get_first(0x01));

    Pattern = Utils::FindPattern("E8 ? ? ? ? ? ? ? ? 8B 0D ? ? ? ? 83 C4 ? 57");
    InjectHook(Pattern.get_first(), &GetTimeForTick, HookType::Call);

    Pattern = Utils::FindPattern("F3 0F 11 90 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? F3 0F 10 82");
    static auto ExposureFix = safetyhook::create_mid(Pattern.get_first(-0x40), [](SafetyHookContext& ctx) {
        static short DeltaROffset = General::IsOTR ? 0x1C0 : 0x1B4;
        float* DeltaRatio = (float*)(ctx.eax + DeltaROffset);
        *DeltaRatio = *DeltaRatio * (Frametime / TargetFT);
        });

    if (!General::IsOTR) {
        Pattern = Utils::FindPattern("F3 0F 10 05 ? ? ? ? 88 41 ? F3 0F 11 41 ? C3 F3 0F 10 05");
        static int eip = (int)Pattern.get_first(0x08);
        static auto MBStrengthFix = safetyhook::create_mid(Pattern.get_first(), [](SafetyHookContext& ctx) {
            static float BaseStrength = 0.20;
            ctx.xmm0.f32[0] = BaseStrength * (TargetFT / Frametime);
            ctx.eip = eip;
            });
    }
}