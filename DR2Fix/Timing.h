#pragma once
#include "Utils.h"

class Timing {
public:
    static LARGE_INTEGER* Freq;
    static float Frametime;
    const static float TargetFT;

    static double __cdecl GetTimeForTick(_LARGE_INTEGER Tick);
    static void Install();
};