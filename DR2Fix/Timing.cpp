#include "Timing.h"
#include "General.h"
#include "Utils.h"

static LARGE_INTEGER* Freq;
static float Frametime;
static const float TargetFT = 0.0333333f;

static double __cdecl GetTimeForTick(_LARGE_INTEGER Tick) {
    double Result = (double)Tick.QuadPart / (double)Freq->QuadPart;
    Frametime = Result;
    return Result;
}

static double __fastcall ComputePushableRotationSpeed(float *self, void *dummy, unsigned int *thing) {
    const auto value = thing[General::IsOTR ? 0x56 : 0x47];

    if (value > 0xF)
        return 0.0;

    return self[2 + value] * Frametime / TargetFT;
}

void Timing::Install() {
    auto Pattern = Utils::FindPattern("68 ? ? ? ? FF 15 ? ? ? ? 68 ? ? ? ? FF 15 ? ? ? ? 33 C0");
    Freq = *(LARGE_INTEGER**)(Pattern.get_first(0x01));

    Pattern = Utils::FindPattern("E8 ? ? ? ? ? ? ? ? 8B 0D ? ? ? ? 83 C4 ? 57");
    InjectHook(Pattern.get_first(), &GetTimeForTick, HookType::Call);

    // Fix pushable objects' turning speed being tied to frame-rate.
    Pattern = Utils::FindPattern("8B 44 24 04 8B 80 ? ? 00 00 83 F8 0F 77 07 D9 44 81 08 C2 04 00 D9 EE C2 04 00");
    InjectHook(Pattern.get_first(), &ComputePushableRotationSpeed, HookType::Jump);

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