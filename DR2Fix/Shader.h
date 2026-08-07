#pragma once

namespace Shader {
    short GetShaderHashOffset();
    bool IsSkinnedShader(unsigned int Hash);
    void Install();
};