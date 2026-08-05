#pragma once

#include "Utils.h"

namespace Settings
{
    enum class Value : DWORD
    {
        MSAA_TYPE,
        FULLSCREEN,
        ZOMBIE_COUNTS,
        COMBINED_BLUR,
        TEXTURE_FILTERING,
        SHADOW_QUALITY
    };

    void Install();
    void* GetValue(Value value);
}
