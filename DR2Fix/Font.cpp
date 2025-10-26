#include "Font.h"
#include "GPU.h"
#include "General.h"

const unsigned int Font::ArialBlk46Hash = 0xD401C410;
const unsigned int Font::ArialBlk18Hash = 0xD401C3FB;
const int Font::FontRes = 1200;
int* Font::FontData;

int Font::BaseFontRes() {
    return FontRes;
}

bool Font::IsAsianLang(int* This) {
    if (General::IsOTR) {
        return *(bool*)This;
    }
    else {
        int FontID = *(int*)(*(int*)(*FontData + 0xBB0) + 0x88);
        return (FontID == 6 || FontID == 7);
    }
}

float __fastcall Font::FontScaleOverride(int* This, unsigned int FontHash, void* Dummy) {
    float Scale = GPU::ResY / float(FontRes);
    if (IsAsianLang(This)) {
        if (FontHash == ArialBlk46Hash) {
            Scale = Scale * 1.9166666;
        }
        else if (FontHash == ArialBlk18Hash) {
            Scale = Scale * 0.83333331;
        }
        return FontRes * 0.0013888889f * Scale;
    }
    return Scale;
}

void Font::Install() {
    auto Pattern = Utils::FindPattern("A1 ? ? ? ? 8B 40 ? 3D");
    InjectHook(Pattern.get_first(), &BaseFontRes, HookType::Jump);

    Pattern = Utils::FindPattern("E8 ? ? ? ? F6 86 ? ? ? ? ? 5E");
    void* OverrideFunc;
    ReadCall(Pattern.get_first(), OverrideFunc);
    InjectHook(OverrideFunc, &FontScaleOverride, HookType::Jump);

    if (!General::IsOTR) {
        Pattern = Utils::FindPattern("A1 ? ? ? ? 8B 54 24 ? 8B 88 ? ? ? ? 52");
        FontData = *(int**)(Pattern.get_first(0x01));
    }
}