#pragma once
#include "stdafx.h"
#include <MemoryMgr.h>
#include <safetyhook.hpp>
#include <d3d9.h>
#include <Hooking.Patterns.h>
#define ARRAY_SIZE(v) (sizeof(v) / sizeof(v[0]))
using namespace Memory::VP;

class Utils {
public:
    static hook::pattern FindPattern(std::string_view PatternStr) {
        hook::pattern Pattern(PatternStr);
        if (Pattern.empty()) {
            char Buffer[512];
            sprintf_s(Buffer, sizeof(Buffer), "Failed to find: %s", PatternStr.data());
            MessageBoxA(nullptr, Buffer, "Pattern not found!", MB_ICONERROR | MB_OK);
            ExitProcess(EXIT_FAILURE);
        }
        return Pattern;
    }
};