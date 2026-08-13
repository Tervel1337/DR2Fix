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
            AlphaRSValue = MAKEFOURCC('A','2','M','1');
        }
        else if (Identifier.VendorId == 0x10DE) { // NVIDIA
            AlphaRS = D3DRS_ADAPTIVETESS_Y;
            AlphaRSValue = MAKEFOURCC('A','T','O','C');
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

static bool force_reverse_cull_mode;
static void (__fastcall *ReverseCullMode)(void *self);

static void (__fastcall *RenderMeshBatch)(void *self, void *dummy, int mesh_batch_index);

static void __fastcall RenderMeshBatchHijack(void *self, void *dummy, int mesh_batch_index) {
    const bool is_actor_icon = mesh_batch_index >= 42 && mesh_batch_index <= 50;

    force_reverse_cull_mode = is_actor_icon;
    RenderMeshBatch(self, dummy, mesh_batch_index);
    force_reverse_cull_mode = false;
}

static void (__fastcall *RenderMesh)(void *self, void *dummy, void *mesh_batch, void *mesh);

static void __fastcall RenderMeshHijack(void *self, void *dummy, void *mesh_batch, void *mesh) {
    const bool reverse_cull_mode = force_reverse_cull_mode ^ static_cast<bool*>(mesh)[0x36];

    if (reverse_cull_mode)
        ReverseCullMode(self);

    RenderMesh(self, dummy, mesh_batch, mesh);

    if (reverse_cull_mode)
        ReverseCullMode(self);
}

static void FixActorIconCullMode() {
    // The "actor icons" (survivor display pop-ups) are horizontally flipped, meaning the
    // culling mode needs to be reversed to account for the inverted vertex winding.
    auto Pattern = Utils::FindPattern("80 7E ? 00 74 ? 83 3E FE 74 ? 53 8B CF E8");
    InterceptCall(Pattern.get_first(14), RenderMeshBatch, &RenderMeshBatchHijack);

    Pattern = Utils::FindPattern("E8 ? ? ? ? 83 7D 00 FF 74 ? 83 BE ? 00 00 00 00 74");
    InterceptCall(Pattern.get_first(24), RenderMesh, &RenderMeshHijack);

    ReadCall(reinterpret_cast<uintptr_t>(RenderMesh) + (General::IsOTR ? 0x1E : 0x10), ReverseCullMode);

    // Disable the old calls to ReverseCullMode, now that we're responsible for calling it instead.
    Nop(reinterpret_cast<uintptr_t>(RenderMesh) + (General::IsOTR ? 0x1E : 0x10), 5);
    Nop(reinterpret_cast<uintptr_t>(RenderMesh) + (General::IsOTR ? 0xE5 : 0xF1), 5);
}

char* GPU::GetGraphicsInst() {
    return *GraphicsInst;
}

unsigned int GPU::GetDisplayWidth() {
    return *(unsigned int*)(GetGraphicsInst() + 0xC);
}

unsigned int GPU::GetDisplayHeight() {
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

    FixActorIconCullMode();
}
