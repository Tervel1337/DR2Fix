#include "General.h"
#include "GPU.h"
#include "Shader.h"

bool General::IsOTR;

void General::Install() {
    auto Pattern = hook::pattern("F3 0F 11 1D ? ? ? ? F3 0F 10 1D ? ? ? ? F3 0F 59 CB"); // not using my wrapper because I explicitly want this to fail for OTR
    if (Pattern.empty()) IsOTR = true;

    Pattern = Utils::FindPattern("68 ? ? ? ? 68 ? ? ? ? 8B CB E8 ? ? ? ? ? ? 83 C6"); // cubemap res, 64x64 to match x360
    Patch<char>(Pattern.get_first(0x01), 64);
    Patch<char>(Pattern.get_first(0x06), 64);
    Patch<char>(Pattern.get_first(0x28), 64);
    Patch<char>(Pattern.get_first(0x2D), 64);

    if (!IsOTR) {
        auto Pattern = Utils::FindPattern("6A ? B9 ? ? ? ? E8 ? ? ? ? F3 0F 10 05"); // fix for the blur setting not applying on boot in DR2
        Nop(Pattern.get_first(), 12);
    }
}