#pragma once
#include "Utils.h"

class Font {
public:
    const static unsigned int ArialBlk46Hash;
    const static unsigned int ArialBlk18Hash;
    static int* FontRes;
    static int* FontData;
    static uintptr_t WatchFace;

    static bool IsAsianLang(int* This);
    static void FixWatchFont(float Scale);
    static float __fastcall FontScaleOverride(int* This, unsigned int FontHash, void* dummy);
    static void Install();
};