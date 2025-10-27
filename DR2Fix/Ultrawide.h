#pragma once
#include "Utils.h"

class Ultrawide {
public:
    static float AspectRatio;
    const static float AR16x9;
    const static float AR32x9;

    static float __fastcall GetUIAspectRatio(int* thisptr, void* Dummy);
    static char __stdcall FixupRes(int* Width, int* Height, tagRECT* DestRect);
    static void Install();
};