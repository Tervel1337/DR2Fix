#include "GPU.h"
#include "Shader.h"
#include "General.h"
#include "Utils.h"

static constexpr int ShadowMapRes = 4096;

static IDirect3DDevice9* pDevice;
static int* DrawInst;
static short D3DDeviceOffset = 0x198;
static D3DRENDERSTATETYPE AlphaRS;
static DWORD AlphaRSValue;

int* GPU::GraphicsInst;
int GPU::ResX;
int GPU::ResY;

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
    ((void(*)())Shader::InitShaders)();
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
        /*unsigned int ShaderHash = *(unsigned int*)(*DrawInst + Shader::ShaderHashOffset);
        if (Shader::IsSkinnedShader(ShaderHash) && Desc.Width == ResX && Desc.Height == ResY) {
            pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
        }
        // no need to reset cullmode because the game will overwrite it anyhow
        else*/ if (Desc.Width == ShadowMapRes * 4 && Desc.Height == ShadowMapRes) pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        pRenderTarget->Release();
    }
}

void GPU::Install() {
    auto Pattern = Utils::FindPattern("E8 ? ? ? ? E8 ? ? ? ? 8B 3D ? ? ? ? 8B CF");
    InterceptCall(Pattern.get_first(0x5), Shader::InitShaders, &GetGPUVendor);

    Pattern = Utils::FindPattern("80 3D ? ? ? ? ? A1 ? ? ? ? 74");
    static SafetyHookMid RSFix = safetyhook::create_mid(Pattern.get_first(), &FixRenderStates);

    Pattern = Utils::FindPattern("A3 ? ? ? ? A3 ? ? ? ? C3 33 C0");
    GraphicsInst = *(int**)(Pattern.get_first(0x01));
    DrawInst = *(int**)(Pattern.get_first(0x06));

    Pattern = Utils::FindPattern("83 BE ? ? ? ? ? C7 86 ? ? ? ? ? ? ? ? C6 86");
    static auto ShadowRes = safetyhook::create_mid(Pattern.get_first(0x11), [](SafetyHookContext& ctx) {
        const short WidthOffset = General::IsOTR ? 0xD0 : 0xBC;
        *(int*)(ctx.esi + WidthOffset) = ShadowMapRes;
       });
}