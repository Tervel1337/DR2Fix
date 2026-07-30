#include "Ultrawide.h"
#include "GPU.h"
#include "General.h"
#include "Utils.h"

static float AspectRatio;
static const float AR16x9 = 1.78f;
static const float AR32x9 = 3.55f;

static float __fastcall GetUIAspectRatio(int* thisptr, void* Dummy) {
    GPU::ResX = *(int*)(*GPU::GraphicsInst + 0xC);
    GPU::ResY = *(int*)(*GPU::GraphicsInst + 0x10);
    AspectRatio = (float)GPU::ResX / (float)GPU::ResY;
    return AspectRatio > AR16x9 ? AR16x9 : AspectRatio;
}

static bool __stdcall FixupRes(int* Width, int* Height, tagRECT* DestRect) {
    return (float)*Width / *Height < AR16x9;
}

void Ultrawide::Install() {
    auto Pattern = Utils::FindPattern("E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 76 ? F3 0F 10 44 24 ? F3 0F 59 05");
    void* AspectRatioF;

    ReadCall(Pattern.get_first(), AspectRatioF);
    InjectHook(AspectRatioF, &GetUIAspectRatio, HookType::Jump);

    Pattern = Utils::FindPattern("83 EC ? ? ? ? ? ? ? 53 8B 5C 24 ? ? ? ? ? ? ? ? ? ? ? 55");
    InjectHook(Pattern.get_first(), &FixupRes, HookType::Jump);

    Pattern = Utils::FindPattern("EB ? F3 0F 10 81 ? ? ? ? F3 0F 10 1D ? ? ? ? F3 0F 11 44 24 ? F3 0F 10 41"); 
    static auto Fix3DAR = safetyhook::create_mid(Pattern.get_first(), [](SafetyHookContext& ctx) {
        if (AspectRatio > AR16x9) ctx.xmm0.f32[0] = AspectRatio;
        });

    Pattern = Utils::FindPattern("? ? ? ? ? ? ? ? DF E0 F6 C4 ? 75 ? ? ? ? ? ? DF E0 F6 C4 ? 7B ? 80 7C 24");
    static auto Fix3DTo2DProj = safetyhook::create_mid(Pattern.get_first(), [](SafetyHookContext& ctx) {
        if (AspectRatio > AR16x9) *(float*)ctx.esi *= AspectRatio * (9.0f / 16.0f);
        });

    Pattern = Utils::FindPattern("F3 0F 10 4C 24 ? F3 0F 10 54 24 ? F3 0F 10 2D");
    static auto ShadowCoverageHack = safetyhook::create_mid(Pattern.get_first(), [](SafetyHookContext& ctx) {
        if (AspectRatio > AR16x9) ctx.xmm0.f32[0] = (9.0f / AR16x9) * (AspectRatio - AR16x9);
        // this is 100% not correct but it works for increasing the coverage in a way that looks fine at least
        });
}
