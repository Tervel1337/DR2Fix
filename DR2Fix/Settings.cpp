#include "Settings.h"

static BOOL *SettingsInitialized;
static void *SettingsObject;
static void (__fastcall *ResetSettings)(void *self, void* dummy);
static void* (__fastcall *GetSettingInternal)(void *self, void* dummy, Settings::Value setting);

void Settings::Install()
{
    // Extract, and create our own 'GetSetting' function.
    // This function is inlined in the EXE, so we cannot use it as-is.
    auto Pattern = Utils::FindPattern("F6 05 ? ? ? ? 01 75 11 83 0D ? ? ? ? 01 B9 ? ? ? ? E8 ? ? ? ? 6A 00");

    SettingsInitialized = *(BOOL**)Pattern.get_first(2);
    SettingsObject = *(void**)Pattern.get_first(0x45-0x35+1);
    ResetSettings = (decltype(ResetSettings))ReadCallFrom(Pattern.get_first(0x4A-0x35));
    GetSettingInternal = (decltype(GetSettingInternal))ReadCallFrom(Pattern.get_first(0x56-0x35));
}

void* Settings::GetValue(Value setting)
{
    if (!*SettingsInitialized)
    {
        *SettingsInitialized = TRUE;
	ResetSettings(SettingsObject, nullptr);
    }
    return GetSettingInternal(SettingsObject, nullptr, setting);
}
