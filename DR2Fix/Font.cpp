#include "Font.h"
#include "GPU.h"
#include "General.h"
#include "Structs.h"

const unsigned int Font::ArialBlk46Hash = 0xD401C410;
const unsigned int Font::ArialBlk18Hash = 0xD401C3FB;
int* Font::FontRes;
int* Font::FontData;
uintptr_t Font::WatchFace;
bool Font::IsAsianLang(int* This) {
    if (General::IsOTR) {
        return *(bool*)This;
    }
    else {
        int FontID = *(int*)(*(int*)(*FontData + 0xBB0) + 0x88);
        return (FontID == 6 || FontID == 7);
    }
}

void Font::FixWatchFont(float Scale) {
    if (WatchFace) { // the game never destroys the watch face but it first is created when you open it
        cFEText* Date = *(cFEText**)(WatchFace + 0x0C);
        cFEText* Time = *(cFEText**)(WatchFace + 0x90);
        Date->ScaleX = 0.70000f / Scale;
        Date->ScaleY = 0.30000f / Scale;
        Time->ScaleX = 0.80000f / Scale;
        Time->ScaleY = 0.40000f / Scale;
    }
}

float __fastcall Font::FontScaleOverride(int* This, unsigned int FontHash, void* Dummy) {
    float Scale = (GPU::ResY) / float(*FontRes);
    if (!General::IsOTR) FixWatchFont(Scale);
    if (IsAsianLang(This)) {
        if (FontHash == ArialBlk46Hash) {
            Scale = Scale * 1.9166666;
        }
        else if (FontHash == ArialBlk18Hash) {
            Scale = Scale * 0.83333331;
        }
        return *FontRes * 0.0013888889f * Scale;
    }
    return Scale;
}

void Font::Install() {
    auto Pattern = Utils::FindPattern("A3 ? ? ? ? 74 ? 83 FD");
    FontRes = *(int**)Pattern.get_first(0x01);

    Pattern = Utils::FindPattern("E8 ? ? ? ? F6 86 ? ? ? ? ? 5E");
    void* OverrideFunc;
    ReadCall(Pattern.get_first(), OverrideFunc);
    InjectHook(OverrideFunc, &FontScaleOverride, HookType::Jump);

    if (!General::IsOTR) {
        Pattern = Utils::FindPattern("A1 ? ? ? ? 8B 54 24 ? 8B 88 ? ? ? ? 52");
        FontData = *(int**)(Pattern.get_first(0x01));

        Pattern = Utils::FindPattern("8B 4E ? 57 E8 ? ? ? ? 8B 4E ? 8B 69");
        static auto GetWatchFace = safetyhook::create_mid(Pattern.get_first(), [](SafetyHookContext& ctx) {
            WatchFace = ctx.esi;
            });
    }
}