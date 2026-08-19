cmake_minimum_required(VERSION 3.20)

project(DR2Fix LANGUAGES C CXX)

add_library(DR2Fix SHARED
	"DR2Fix/Controller.cpp" "DR2Fix/Controller.h"
	"DR2Fix/DllMain.cpp"
	"DR2Fix/Font.cpp" "DR2Fix/Font.h"
	"DR2Fix/General.cpp" "DR2Fix/General.h"
	"DR2Fix/GPU.cpp" "DR2Fix/GPU.h"
	"DR2Fix/HD.cpp" "DR2Fix/HD.h"
	"DR2Fix/Patches.cpp" "DR2Fix/Patches.h"
	"DR2Fix/Settings.cpp" "DR2Fix/Settings.h"
	"DR2Fix/Shader.cpp" "DR2Fix/Shader.h"
	"DR2Fix/stdafx.h"
	"DR2Fix/Structs.h"
	"DR2Fix/Timing.cpp" "DR2Fix/Timing.h"
	"DR2Fix/Ultrawide.cpp" "DR2Fix/Ultrawide.h"
	"DR2Fix/Utils.h"
	"Includes/Hooking.Patterns/Hooking.Patterns.cpp" "Includes/Hooking.Patterns/Hooking.Patterns.h"
	"Includes/MemoryMgr/MemoryMgr.h"
	"Includes/safetyhook/safetyhook.cpp" "Includes/safetyhook/safetyhook.hpp"
	"Includes/safetyhook/Zydis.c" "Includes/safetyhook/Zydis.h"
)

target_include_directories(DR2Fix PRIVATE
	"Includes/Hooking.Patterns"
	"Includes/MemoryMgr"
	"Includes/safetyhook"
)

# Enable Unicode, to match the game's EXE.
target_compile_definitions(DR2Fix PRIVATE UNICODE _UNICODE)

set_target_properties(DR2Fix PROPERTIES CXX_STANDARD 23)

# For the improved controller support, build and link a stripped-down SDL3.

# Produce static library only.
set(SDL_STATIC ON)
set(SDL_SHARED OFF)

# Disable unneeded subsystems; we just want controller functionality.
set(SDL_AUDIO OFF)
set(SDL_VIDEO OFF)
set(SDL_GPU OFF)
set(SDL_RENDER OFF)
set(SDL_CAMERA OFF)
set(SDL_HAPTIC OFF)
set(SDL_POWER OFF)
set(SDL_SENSOR OFF)
set(SDL_DIALOG OFF)
set(SDL_TRAY OFF)
add_subdirectory("Includes/SDL" EXCLUDE_FROM_ALL)

# Remove bloat.
target_compile_definitions(SDL3-static PRIVATE SDL_LEAN_AND_MEAN)

# Trick SDL into disabling its dynamic API, to reduce bloat further.
target_compile_definitions(SDL3-static PRIVATE __INTELLISENSE__)

target_link_libraries(DR2Fix PRIVATE SDL3::SDL3-static)
