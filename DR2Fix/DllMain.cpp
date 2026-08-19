#include "Patches.h"
#include "Utils.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		break;
	case DLL_PROCESS_DETACH:
		FreeLibraryAndExitThread(hModule, TRUE);
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	default:
		break;
	}
	return TRUE;
}

extern "C" __declspec(dllexport) void InitializeASI() {
	// This should not be done in DllMain, otherwise the game will fail to boot on the Steam Deck!
	// Microsoft's documentation says that a DLL should do as little as possible in DllMain:
	// https://learn.microsoft.com/en-us/windows/win32/dlls/dllmain
	Patches::Install();
}
