#include "Controller.h"
#include "General.h"
#include "GPU.h"
#include "HD.h"
#include "Timing.h"
#include "Font.h"
#include "Ultrawide.h"
#include "Settings.h"
#include "Shader.h"
#include "Patches.h"
#include "Utils.h"

void Patches::Install() {
    General::Install();
    Settings::Install();
    GPU::Install();
    Timing::Install();
    Font::Install();
    HD::Install();
    Ultrawide::Install();
    Shader::Install();
    Controller::Install();
}