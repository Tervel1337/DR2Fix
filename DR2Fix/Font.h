#pragma once
#include "Utils.h"

class Font {
public:
    const static unsigned int ArialBlk46Hash;
    const static unsigned int ArialBlk18Hash;
    const static int FontRes;
    static int* FontData;

    static int BaseFontRes();
    static bool IsAsianLang(int* This);
    static float __fastcall FontScaleOverride(int* This, unsigned int FontHash, void* dummy);
    static void Install();
};