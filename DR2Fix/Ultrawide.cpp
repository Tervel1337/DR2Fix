#include "Ultrawide.h"
#include "GPU.h"
#include "General.h"
#include "Utils.h"

float Ultrawide::AspectRatio;
const float Ultrawide::AR16x9 = 1.78f;
const float Ultrawide::AR32x9 = 3.55f;
int GPU::ShadowMapRes = 1024;

float __fastcall Ultrawide::GetUIAspectRatio(int* thisptr, void* Dummy) {
    GPU::ResX = *(int*)(*GPU::GraphicsInst + 0xC);
    GPU::ResY = *(int*)(*GPU::GraphicsInst + 0x10);
    AspectRatio = (float)GPU::ResX / (float)GPU::ResY;
    return AspectRatio > AR16x9 ? AR16x9 : AspectRatio;
}

char __stdcall Ultrawide::FixupRes(int* Width, int* Height, tagRECT* DestRect) {
    static float AspectRatios[] = { 1.33f, AR16x9, 1.6f, 1.25f, 2.37f, 2.38f, AR32x9 };

    for (int i = 0; i < ARRAY_SIZE(AspectRatios); ++i) {
        if (fabs((*Width / (float)(*Height)) - AspectRatios[i]) < 0.01f) return 0;
    }

    int NewWidth = (*Width * 9 > *Height * 16) ? (*Height * 16) / 9 : *Width;
    int NewHeight = (*Width * 9 > *Height * 16) ? *Height : (*Width * 9) / 16;

    DestRect->left = (*Width - NewWidth) / 2;
    DestRect->top = (*Height - NewHeight) / 2;
    DestRect->right = DestRect->left + NewWidth;
    DestRect->bottom = DestRect->top + NewHeight;

    *Width = NewWidth;
    *Height = NewHeight;

    return 1;
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
        ctx.xmm0.f32[0] = AspectRatio;
        });

    Pattern = Utils::FindPattern("? ? ? ? ? ? ? ? DF E0 F6 C4 ? 75 ? ? ? ? ? ? DF E0 F6 C4 ? 7B ? 80 7C 24");
    static auto Fix3DTo2DProj = safetyhook::create_mid(Pattern.get_first(), [](SafetyHookContext& ctx) {
        if (AspectRatio > AR16x9) *(float*)ctx.esi *= AspectRatio * 0.5625f;
        });

    Pattern = Utils::FindPattern("F3 0F 10 4C 24 ? F3 0F 10 54 24 ? F3 0F 10 2D");
    static auto ShadowCoverageHack = safetyhook::create_mid(Pattern.get_first(), [](SafetyHookContext& ctx) {
        if (AspectRatio > AR16x9) ctx.xmm0.f32[0] = (9.0f / AR16x9) * (AspectRatio - AR16x9);
        // this is 100% not correct but it works for increasing the coverage in a way that looks fine at least
        });

    Pattern = Utils::FindPattern("83 BE ? ? ? ? ? C7 86 ? ? ? ? ? ? ? ? C6 86");
    static auto ShadowRes = safetyhook::create_mid(Pattern.get_first(0x11), [](SafetyHookContext& ctx) {
        const short WidthOffset = General::IsOTR ? 0xD0 : 0xBC;
        if (AspectRatio > AR16x9) {
            GPU::ShadowMapRes = AspectRatio < AR32x9 ? 2048 : 4096;
            *(int*)(ctx.esi + WidthOffset) = AspectRatio < AR32x9 ? 2048 : 4096;
        }
        else GPU::ShadowMapRes = *(int*)(ctx.esi + WidthOffset);
       });

}
