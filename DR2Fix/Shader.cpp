#include "Shader.h"
#include "General.h"
#include "Utils.h"

static short ShaderHashOffset = 0x414;
static const char* BasePath = "data/shaders/deadrising-ps-base.big";
static const char* OTRPath = "data/shaders/deadrising-ps-otr.big";

static bool IsSkinnedShader(unsigned int Hash) {
    // these are the name hashes so if you mod those shaders they won't change and also I don't see the point in computing them
    if (Hash == 0xB9CB5CE0 // ProtoSkinnedEye
        || Hash == 0xEC541DC0 // ProtoSkinnedSkin
        || Hash == 0x4635A600 // ProtoSkinnedSkinRGB
        || Hash == 0xA4CF76A0 // ProtoSkinnedCloth
        || Hash == 0xC85A1340 // ProtoSkinnedClothReflective
        || Hash == 0x5CEF4D20) // ProtoSkinnedClothRGB
    {
        return true;
    }
    return false;
}

void Shader::Install() {
    if (General::IsOTR) ShaderHashOffset += 0x104;

    auto Pattern = Utils::FindPattern("8B 0D ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68");
    Patch<const char*>(Pattern.get_first(0x11), General::IsOTR ? OTRPath :BasePath);
}