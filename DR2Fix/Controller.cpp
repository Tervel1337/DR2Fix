#include "Controller.h"

#define DIRECTINPUT_VERSION 0x800
#include <dinput.h>
#include <xinput.h>
#include <SDL3/SDL.h>

#include "Utils.h"

static double (__cdecl *GetTimeForTick)(_LARGE_INTEGER Tick);
static SDL_Gamepad *gamepads[4];
static DWORD packet_number;

static void ProcessEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_EVENT_GAMEPAD_ADDED:
				for (auto &gamepad : gamepads)
				{
					if (gamepad == nullptr)
					{
						gamepad = SDL_OpenGamepad(event.gdevice.which);
						break;
					}
				}
				break;

			case SDL_EVENT_GAMEPAD_REMOVED:
				for (auto &gamepad : gamepads)
				{
					if (gamepad == SDL_GetGamepadFromID(event.gdevice.which))
					{
						SDL_CloseGamepad(gamepad);
						gamepad = nullptr;
						break;
					}
				}
				break;
		}
	}
}

static double __cdecl GetTimeForTickHijack(const _LARGE_INTEGER Tick)
{
	// Update SDL controller state.
	ProcessEvents();

	return GetTimeForTick(Tick);
}

static HRESULT __stdcall EnumDevicesReplacement(const LPVOID self, const DWORD dwDevType, const LPDIENUMDEVICESCALLBACK lpCallback, const LPVOID pvRef, const DWORD dwFlags)
{
	// TODO: Fill in the rest of this struct?
	DIDEVICEINSTANCE device_instance = {};
	device_instance.dwSize = sizeof(device_instance);
	// The game only allows the following IDs:
	//   0x028E045E - Xbox360 Controller
	//   0x02A1045E - Xbox 360 Wireless Receiver for Windows
	//   0x02D1045E - Xbox One Controller
	//   0x44260738 - Unknown Mad Catz Device
	device_instance.guidProduct.Data1 = 0x028E045E;
	device_instance.dwDevType = DI8DEVTYPE_GAMEPAD;
	strcpy(device_instance.tszInstanceName, "SDL Gamepad 1");
	strcpy(device_instance.tszProductName, "SDL Gamepad");

	for (const auto &gamepad : gamepads)
		if (gamepad != nullptr)
			if (lpCallback(&device_instance, pvRef) == DIENUM_STOP)
				break;

	return DI_OK;
}

static DWORD GamepadFromPlayerIndex(const DWORD player_index, const auto &callback)
{
	if (player_index >= std::size(gamepads))
		return ERROR_DEVICE_NOT_CONNECTED;

	SDL_Gamepad* const gamepad = gamepads[player_index];

	if (gamepad == nullptr)
		return ERROR_DEVICE_NOT_CONNECTED;

	callback(gamepad);

	return ERROR_SUCCESS;
}

static DWORD __stdcall GetControllerState(const DWORD player_index, XINPUT_STATE* const output_state)
{
	return GamepadFromPlayerIndex(player_index,
		[&](SDL_Gamepad* const gamepad)
		{
			output_state->dwPacketNumber = packet_number++; // TODO: Only increment this when there have been changes!

			const auto &DoButton = [&](const unsigned int bit, const SDL_GamepadButton button)
			{
				output_state->Gamepad.wButtons |= static_cast<unsigned int>(SDL_GetGamepadButton(gamepad, button)) << bit;
			};

			output_state->Gamepad.wButtons = 0;
			DoButton( 0, SDL_GAMEPAD_BUTTON_DPAD_UP);
			DoButton( 1, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
			DoButton( 2, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
			DoButton( 3, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
			DoButton( 4, SDL_GAMEPAD_BUTTON_START);
			DoButton( 5, SDL_GAMEPAD_BUTTON_BACK);
			DoButton( 6, SDL_GAMEPAD_BUTTON_LEFT_STICK);
			DoButton( 7, SDL_GAMEPAD_BUTTON_RIGHT_STICK);
			DoButton( 8, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
			DoButton( 9, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
			DoButton(12, SDL_GAMEPAD_BUTTON_SOUTH);
			DoButton(13, SDL_GAMEPAD_BUTTON_EAST);
			DoButton(14, SDL_GAMEPAD_BUTTON_WEST);
			DoButton(15, SDL_GAMEPAD_BUTTON_NORTH);

			// XInput triggers are unsigned 8-bit, but SDL's are unsigned 15-bit.
			output_state->Gamepad.bLeftTrigger  = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER ) / 0x80;
			output_state->Gamepad.bRightTrigger = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) / 0x80;

			// The Y axis is inverted. A negation is the obvious choice, but a bitwise NOT is what SDL actually does.
			// Negation is not actually correct anyway, since -32768 cannot be made positive without overflowing.
			output_state->Gamepad.sThumbLX =  SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
			output_state->Gamepad.sThumbLY = ~SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);
			output_state->Gamepad.sThumbRX =  SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTX);
			output_state->Gamepad.sThumbRY = ~SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY);
		}
	);
}

static DWORD __stdcall SetControllerState(const DWORD player_index, XINPUT_VIBRATION* const vibration)
{
	return GamepadFromPlayerIndex(player_index,
		[&](SDL_Gamepad* const gamepad)
		{
			SDL_RumbleGamepad(gamepad, vibration->wLeftMotorSpeed, vibration->wRightMotorSpeed, 10 * 1000); // 10-second time-out.
		}
	);
}

void Controller::Install()
{
	// TODO: Deinitialise on shutdown?
	SDL_Init(SDL_INIT_GAMEPAD);

	// Process events to register controllers as soon as possible.
	ProcessEvents();

	// Insert a hook which calls 'SDL_PollEvents' roughly every frame, otherwise the controllers will not be read.
	auto Pattern = Utils::FindPattern("E8 ? ? ? ? ? ? ? ? 8B 0D ? ? ? ? 83 C4 ? 57");
	InterceptCall(Pattern.get_first(), GetTimeForTick, &GetTimeForTickHijack);

	// Hijack the DirectInput gamepad enumeration, so we can insert our SDL controllers.
	// Without this, the game will not 'register' the controllers, and it will disable controller support altogether.
	Pattern = Utils::FindPattern("8B 08 8B 51 10 6A 01 6A 00 68 ? ? ? ? 6A 04 50 FF D2");
	Patch(Pattern.get_first(), 0xBA); // mov edx
	Patch(Pattern.get_first(1), &EnumDevicesReplacement);

	// Insert custom XInputGetState function (reads inputs).
	Pattern = Utils::FindPattern("C1 E0 04 8D 84 30 ? ? ? ? 50 51 E8");
	InjectHook(Pattern.get_first(12), &GetControllerState, HookType::Call);

	// Insert custom XInputSetState function (controls rumble).
	Pattern = Utils::FindPattern("DD D8 52 50 E8");
	InjectHook(Pattern.get_first(4), &SetControllerState, HookType::Call);
}
