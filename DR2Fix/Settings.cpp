#include "Settings.h"

static BOOL *SettingsInitialized;
static void *SettingsObject;
static void (__fastcall *ResetSettings)(void *self, void* dummy);
static void* (__fastcall *GetSettingInternal)(void *self, void* dummy, Settings::Value setting);

void Settings::Install() {
    // Extract and create our own 'GetSetting' function.
    // This function is inlined in the EXE, so we cannot use it as-is.
    auto Pattern = Utils::FindPattern("F6 05 ? ? ? ? 01 75 11 83 0D ? ? ? ? 01 B9 ? ? ? ? E8 ? ? ? ? 6A 00");

    SettingsInitialized = *static_cast<BOOL**>(Pattern.get_first(2));
    SettingsObject = *static_cast<void**>(Pattern.get_first(17));
    ResetSettings = static_cast<decltype(ResetSettings)>(ReadCallFrom(Pattern.get_first(21)));
    GetSettingInternal = static_cast<decltype(GetSettingInternal)>(ReadCallFrom(Pattern.get_first(33)));
}

void* Settings::GetValue(Value setting) {
    if (!*SettingsInitialized) {
        *SettingsInitialized = TRUE;
        ResetSettings(SettingsObject, nullptr);
    }

    return GetSettingInternal(SettingsObject, nullptr, setting);
}
