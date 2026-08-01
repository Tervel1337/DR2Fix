#include "GPU.h"
#include "Shader.h"
#include "General.h"
#include "Utils.h"

static IDirect3DDevice9* pDevice;
static int* DrawInst;
static short D3DDeviceOffset = 0x198;
static D3DRENDERSTATETYPE AlphaRS;
static DWORD AlphaRSValue;
static void (*InitShaders)();

int* GPU::GraphicsInst;
int GPU::ResX;
int GPU::ResY;

static unsigned int ScaleResolution(unsigned int resolution)
{
    const int screen_width = *(int*)(*GPU::GraphicsInst + 0xC);

    // We scale based on the resolution so that we do not overwhelm low-end platforms with massive textures.
    // 1280x720:  1x
    // 1920x1080: 2x
    // 2560x1440: 4x
    // There is no 3x because we need to maintain powers of two.
    if (screen_width >= 2560)
        resolution <<= 2;
    else if (screen_width >= 1920)
        resolution <<= 1;

    return resolution;
}

static unsigned int CorrectTextureFormat(unsigned int format)
{
    const int screen_width = *(int*)(*GPU::GraphicsInst + 0xC);

    if (screen_width >= 1920)
        if (format == 3) // RGB565
            format = 1;  // XRGB8888

    return format;
}

static void GetGPUVendor() {
    if (General::IsOTR) D3DDeviceOffset += 0x100;

    pDevice = *(IDirect3DDevice9**)(void*)(*GPU::GraphicsInst + D3DDeviceOffset);
    if (pDevice) {
        IDirect3D9* pD3D;
        pDevice->GetDirect3D(&pD3D);
        D3DDEVICE_CREATION_PARAMETERS Params = {};
        D3DADAPTER_IDENTIFIER9 Identifier = {};
        pD3D->GetAdapterIdentifier(Params.AdapterOrdinal, 0, &Identifier);
        if (Identifier.VendorId == 0x1002) { // AMD
            AlphaRS = D3DRS_POINTSIZE;
            AlphaRSValue = 0x314D3241; // ATOC
        }
        else if (Identifier.VendorId == 0x10DE) { // NVIDIA
            AlphaRS = D3DRS_ADAPTIVETESS_Y;
            AlphaRSValue = 0x434F5441; // A2M1
        }
    }
    InitShaders();
}

static void FixRenderStates(safetyhook::Context32& ctx) {
    if (!pDevice) return;

    DWORD AlphaRef, AlphaTest;
    pDevice->GetRenderState(D3DRS_ALPHAREF, &AlphaRef);
    pDevice->GetRenderState(D3DRS_ALPHATESTENABLE, &AlphaTest);
    if (AlphaTest == 1 && AlphaRef != 0) pDevice->SetRenderState(AlphaRS, AlphaRSValue);
    else pDevice->SetRenderState(AlphaRS, 0); // this needs manual resetting because it's a state the game doesn't set

    LPDIRECT3DSURFACE9 pRenderTarget = nullptr;
    pDevice->GetRenderTarget(0, &pRenderTarget);
    if (pRenderTarget) {
        D3DSURFACE_DESC Desc;
        pRenderTarget->GetDesc(&Desc);
	const unsigned int ShadowMapRes = ScaleResolution(1024);
        /*unsigned int ShaderHash = *(unsigned int*)(*DrawInst + Shader::ShaderHashOffset);
        if (Shader::IsSkinnedShader(ShaderHash) && Desc.Width == ResX && Desc.Height == ResY) {
            pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
        }
        // no need to reset cullmode because the game will overwrite it anyhow
        else*/ if (Desc.Width == ShadowMapRes * 4 && Desc.Height == ShadowMapRes) pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        pRenderTarget->Release();
    }
}

static int (__fastcall *CreateTexture)(void *self, void* dummy, int width, int height, int format, int a5, int a6, int multisample_type, const char *label);

static int __fastcall CreateTextureHijack(void *self, void* dummy, int width, int height, int format, int a5, int a6, int multisample_type, const char *label)
{
    return CreateTexture(self, dummy, ScaleResolution(width), ScaleResolution(height), CorrectTextureFormat(format), a5, a6, multisample_type, label);
}

static int (__fastcall *CreateTextureRenderTarget)(void *self, void* dummy, int width, int height, int format, int a5, unsigned int a6, int multisample_type);

static int __fastcall CreateTextureRenderTargetHijack(void *self, void* dummy, int width, int height, int format, int a5, unsigned int a6, int multisample_type)
{
    return CreateTextureRenderTarget(self, dummy, ScaleResolution(width), ScaleResolution(height), CorrectTextureFormat(format), a5, a6, multisample_type);
}

void GPU::Install() {
    auto Pattern = Utils::FindPattern("E8 ? ? ? ? E8 ? ? ? ? 8B 3D ? ? ? ? 8B CF");
    InterceptCall(Pattern.get_first(0x5), InitShaders, &GetGPUVendor);

    Pattern = Utils::FindPattern("80 3D ? ? ? ? ? A1 ? ? ? ? 74");
    static SafetyHookMid RSFix = safetyhook::create_mid(Pattern.get_first(), &FixRenderStates);

    Pattern = Utils::FindPattern("A3 ? ? ? ? A3 ? ? ? ? C3 33 C0");
    GraphicsInst = *(int**)(Pattern.get_first(0x01));
    DrawInst = *(int**)(Pattern.get_first(0x06));

    // Increase resolution of shadow map.
    Pattern = Utils::FindPattern("6A 00 83 C0 01 6A 19 53 89 0C 85");
    InterceptCall(Pattern.get_first(0x22), CreateTexture, &CreateTextureHijack);
    InterceptCall(Pattern.get_first(0x50), CreateTextureRenderTarget, &CreateTextureRenderTargetHijack);

    // Increase resolution of mirror reflection textures.
    // Also changes format from RGB565 to XRGB8888, to eliminate colour-banding.
    Pattern = Utils::FindPattern("6A 19 C7 86 ? ? ? ? 00 00 00 00 8B 3D ? ? ? ? 68 00 01 00 00 68 00 04 00 00 8B CF E8");
    InjectHook(Pattern.get_first(0x1E), &CreateTextureHijack);
    InjectHook(Pattern.get_first(0x70), &CreateTextureRenderTargetHijack);
}
