#include "General.h"
#include "GPU.h"
#include "Timing.h"
#include "Font.h"
#include "Ultrawide.h"
#include "Shader.h"
#include "Patches.h"

void Patches::Install() {
    General::Install();
    GPU::Install();
    Timing::Install();
    Font::Install();
    Ultrawide::Install();
    Shader::Install();
}