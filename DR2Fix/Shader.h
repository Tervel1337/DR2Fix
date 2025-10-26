#pragma once
#include "Utils.h"

class Shader {
public:
    static short ShaderHashOffset;
    static void* InitShaders;
    static const char* BasePath;
    static const char* OTRPath;

    static bool IsSkinnedShader(unsigned int Hash);
    static void Install();
};