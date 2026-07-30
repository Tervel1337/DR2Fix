#include "General.h"
#include "GPU.h"
#include "Shader.h"
#include "Utils.h"

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

        Pattern = Utils::FindPattern("83 C4 08 8B F0 80 ? ? ? ? 00 00 74 18 6A 07"); // skip startup logos
        Nop(Pattern.get_first(12), 2);
    }
    else {
        auto Pattern = Utils::FindPattern("8B 70 ? 8B 0D ? ? ? ? 56");
        static const uintptr_t SkipAddr = (uintptr_t)Pattern.get_first(0x166);

        static auto FixAnimCrash = safetyhook::create_mid(Pattern.get_first(), [](SafetyHookContext& ctx) {
            if (!ctx.eax) ctx.eip = SkipAddr;
            });
        // the console version does this, this system was added in OTR and the PC version uses an earlier codebase I guess
    }
}