#include "GPU.h"
#include "HD.h"
#include "Shader.h"
#include "General.h"
#include "Utils.h"

static IDirect3DDevice9* pDevice;
static char** GraphicsInst;
static int* DrawInst;
static short D3DDeviceOffset = 0x198;
static D3DRENDERSTATETYPE AlphaRS;
static DWORD AlphaRSValue;
static void (*InitShaders)();

static void GetGPUVendor() {
    if (General::IsOTR) D3DDeviceOffset += 0x100;

    pDevice = *(IDirect3DDevice9**)(GPU::GetGraphicsInst() + D3DDeviceOffset);
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
        const unsigned int ShadowMapRes = HD::GetScaledResolution(1024);
        const unsigned int SurvivorDisplayRes = HD::GetScaledResolution(64);
        /*unsigned int ShaderHash = *(unsigned int*)(*DrawInst + Shader::GetShaderHashOffset());
        if (Shader::IsSkinnedShader(ShaderHash) && Desc.Width == GPU::GetDisplayWidth() && Desc.Height == GPU::GetDisplayHeight()) {
            pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
        }
        // no need to reset cullmode because the game will overwrite it anyhow
        else*/ if (Desc.Width == ShadowMapRes * 4 && Desc.Height == ShadowMapRes) pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

        pRenderTarget->Release();
    }
}

char* GPU::GetGraphicsInst()
{
    return *GraphicsInst;
}

unsigned int GPU::GetDisplayWidth()
{
    return *(unsigned int*)(GetGraphicsInst() + 0xC);
}

unsigned int GPU::GetDisplayHeight()
{
    return *(unsigned int*)(GetGraphicsInst() + 0x10);
}

void GPU::Install() {
    auto Pattern = Utils::FindPattern("E8 ? ? ? ? E8 ? ? ? ? 8B 3D ? ? ? ? 8B CF");
    InterceptCall(Pattern.get_first(0x5), InitShaders, &GetGPUVendor);

    Pattern = Utils::FindPattern("80 3D ? ? ? ? ? A1 ? ? ? ? 74");
    static SafetyHookMid RSFix = safetyhook::create_mid(Pattern.get_first(), &FixRenderStates);

    Pattern = Utils::FindPattern("A3 ? ? ? ? A3 ? ? ? ? C3 33 C0");
    GraphicsInst = *(char***)(Pattern.get_first(0x01));
    DrawInst = *(int**)(Pattern.get_first(0x06));
}
