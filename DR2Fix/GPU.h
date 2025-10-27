#pragma once
#include "Utils.h"

class GPU {
public:
    static IDirect3DDevice9* pDevice;
    static short D3DDeviceOffset;
    static D3DRENDERSTATETYPE AlphaRS;
    static DWORD AlphaRSValue;
    static int* GraphicsInst;
    static int* DrawInst;
    static int ResX;
    static int ResY;
    static int ShadowMapRes;

    static void GetGPUVendor();
    static void FixRenderStates(safetyhook::Context32& ctx);
    static void Install();
};