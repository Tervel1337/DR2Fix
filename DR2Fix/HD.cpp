#include "HD.h"

#include "General.h"
#include "GPU.h"
#include "Settings.h"
#include "Utils.h"

enum TEXTUREFORMAT : DWORD
{
    TEXTUREFORMAT_A8R8G8B8 = 0,
    TEXTUREFORMAT_X8R8G8B8 = 1,
    TEXTUREFORMAT_A4R4G4B4 = 2,
    TEXTUREFORMAT_R5G6B5 = 3,
    TEXTUREFORMAT_A1R5G5B5 = 4,
    TEXTUREFORMAT_P8 = 5,
    TEXTUREFORMAT_L8 = 7,
    TEXTUREFORMAT_DXT1 = 8,
    TEXTUREFORMAT_DXT3 = 9,
    TEXTUREFORMAT_DXT5 = 10,
    TEXTUREFORMAT_A16B16G16R16 = 16,
    TEXTUREFORMAT_R16F = 17,
    TEXTUREFORMAT_G16R16F = 18,
    TEXTUREFORMAT_A16B16G16R16F = 19,
    TEXTUREFORMAT_R32F = 20,
    TEXTUREFORMAT_A32B32G32R32F = 22,
    TEXTUREFORMAT_D16 = 23,
    TEXTUREFORMAT_D24X8 = 24,
    TEXTUREFORMAT_D24S8 = 25,
    TEXTUREFORMAT_DXT5_2 = 31,
    TEXTUREFORMAT_DXT5_3 = 32,
    TEXTUREFORMAT_A8 = 33,
    TEXTUREFORMAT_UNSUPPORTED = 34
};

enum MULTISAMPLETYPE : DWORD
{
    MULTISAMPLETYPE_NONE,
    MULTISAMPLETYPE_2_SAMPLES,
    MULTISAMPLETYPE_3_SAMPLES,
    MULTISAMPLETYPE_4_SAMPLES,
    MULTISAMPLETYPE_5_SAMPLES,
    MULTISAMPLETYPE_6_SAMPLES,
    MULTISAMPLETYPE_7_SAMPLES,
    MULTISAMPLETYPE_8_SAMPLES,
    MULTISAMPLETYPE_9_SAMPLES,
    MULTISAMPLETYPE_10_SAMPLES,
    MULTISAMPLETYPE_11_SAMPLES,
    MULTISAMPLETYPE_12_SAMPLES,
    MULTISAMPLETYPE_13_SAMPLES,
    MULTISAMPLETYPE_14_SAMPLES,
    MULTISAMPLETYPE_15_SAMPLES,
    MULTISAMPLETYPE_16_SAMPLES
};

enum class ResolutionQuality
{
    LOW,
    MEDIUM,
    HIGH
};

static ResolutionQuality GetResolutionQuality()
{
    const int screen_width = GPU::GetDisplayWidth();

    if (screen_width >= 2560)
        return ResolutionQuality::HIGH;
    else if (screen_width >= 1920)
        return ResolutionQuality::MEDIUM;
    else
        return ResolutionQuality::LOW;
}

unsigned int HD::GetScaledResolution(unsigned int resolution)
{
    // We scale based on the resolution so that we do not overwhelm low-end platforms with massive textures.
    // 1280x720:  1x
    // 1920x1080: 2x
    // 2560x1440: 4x
    // There is no 3x because we need to maintain powers of two.
    switch (GetResolutionQuality())
    {
        case ResolutionQuality::HIGH:
            resolution <<= 2;
            break;

        case ResolutionQuality::MEDIUM:
            resolution <<= 1;
            break;

        case ResolutionQuality::LOW:
            resolution <<= 0;
            break;
    }

    return resolution;
}

static TEXTUREFORMAT CorrectTextureFormat(TEXTUREFORMAT format)
{
    if (format == TEXTUREFORMAT_R5G6B5)
        format = TEXTUREFORMAT_X8R8G8B8;

    return format;
}

static int (__fastcall *CreateTextureDepthStencil)(void *self, void* dummy, int width, int height, TEXTUREFORMAT format, int a5, int a6, MULTISAMPLETYPE multisample_type, const char *label);
static int (__fastcall *CreateTextureRenderTarget)(void *self, void* dummy, int width, int height, TEXTUREFORMAT format, int a5, unsigned int a6, MULTISAMPLETYPE multisample_type);

static int __fastcall CreateTextureDepthStencilHD(void *self, void* dummy, int width, int height, TEXTUREFORMAT format, int a5, int a6, MULTISAMPLETYPE multisample_type, const char *label)
{
    return CreateTextureDepthStencil(self, dummy, HD::GetScaledResolution(width), HD::GetScaledResolution(height), CorrectTextureFormat(format), a5, a6, multisample_type, label);
}

static int __fastcall CreateTextureRenderTargetHD(void *self, void* dummy, int width, int height, TEXTUREFORMAT format, int a5, unsigned int a6, MULTISAMPLETYPE multisample_type)
{
    return CreateTextureRenderTarget(self, dummy, HD::GetScaledResolution(width), HD::GetScaledResolution(height), CorrectTextureFormat(format), a5, a6, multisample_type);
}

static MULTISAMPLETYPE GetMultisampleType()
{
    // Use whatever level of MSAA the user has selected in the settings.
    return *static_cast<MULTISAMPLETYPE*>(Settings::GetValue(Settings::Value::MSAA_TYPE));
}

static int __fastcall CreateTextureDepthStencilHDMSAA(void *self, void* dummy, int width, int height, TEXTUREFORMAT format, int a5, int a6, MULTISAMPLETYPE multisample_type, const char *label)
{
    return CreateTextureDepthStencil(self, dummy, HD::GetScaledResolution(width), HD::GetScaledResolution(height), CorrectTextureFormat(format), a5, a6, GetMultisampleType(), label);
}

static int __fastcall CreateTextureRenderTargetHDMSAA(void *self, void* dummy, int width, int height, TEXTUREFORMAT format, int a5, unsigned int a6, MULTISAMPLETYPE multisample_type)
{
    return CreateTextureRenderTarget(self, dummy, HD::GetScaledResolution(width), HD::GetScaledResolution(height), CorrectTextureFormat(format), a5, a6, GetMultisampleType());
}

void HD::Install()
{
    // Increase resolution of shadow map.
    if (General::IsOTR)
    {
        auto Pattern = Utils::FindPattern("6A 00 03 C0 6A 19 03 C0 52");
        InterceptCall(Pattern.get_first(0x18), CreateTextureDepthStencil, &CreateTextureDepthStencilHD);
        InterceptCall(Pattern.get_first(0x81), CreateTextureRenderTarget, &CreateTextureRenderTargetHD);
    }
    else
    {
        auto Pattern = Utils::FindPattern("6A 00 83 C0 01 6A 19 53 89 0C 85");
        InterceptCall(Pattern.get_first(0x22), CreateTextureDepthStencil, &CreateTextureDepthStencilHD);
        InterceptCall(Pattern.get_first(0x50), CreateTextureRenderTarget, &CreateTextureRenderTargetHD);
    }

    // Increase resolution of mirror reflection textures.
    // Also applies MSAA to eliminate aliasing.
    // Also changes format from RGB565 to XRGB8888, to eliminate colour-banding.
    auto Pattern = Utils::FindPattern("6A 19 C7 86 ? ? ? ? 00 00 00 00 8B 3D ? ? ? ? 68 00 01 00 00 68 00 04 00 00 8B CF E8");
    InjectHook(Pattern.get_first(0x1E), &CreateTextureDepthStencilHDMSAA);
    InjectHook(Pattern.get_first(0x70), &CreateTextureRenderTargetHDMSAA);

    // Increase resolution of survivor displays.
    // Also applies MSAA to eliminate aliasing.
    Pattern = Utils::FindPattern("6A 00 F3 0F 11 86 ? ? ? ? F3 0F 11 86 ? ? ? ? F3 0F 11 86 ? ? ? ? 8B 1D");
    InjectHook(Pattern.get_first(0x2A), &CreateTextureRenderTargetHDMSAA);
}
