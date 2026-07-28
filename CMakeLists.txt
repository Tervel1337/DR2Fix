cmake_minimum_required(VERSION 3.20)

project(DR2Fix LANGUAGES C CXX)

add_library(DR2Fix SHARED
	"DR2Fix/DllMain.cpp"
	"DR2Fix/Font.cpp" "DR2Fix/Font.h"
	"DR2Fix/General.cpp" "DR2Fix/General.h"
	"DR2Fix/GPU.cpp" "DR2Fix/GPU.h"
	"DR2Fix/Patches.cpp" "DR2Fix/Patches.h"
	"DR2Fix/Shader.cpp" "DR2Fix/Shader.h"
	"DR2Fix/stdafx.h"
	"DR2Fix/Structs.h"
	"DR2Fix/Timing.cpp" "DR2Fix/Timing.h"
	"DR2Fix/Ultrawide.cpp" "DR2Fix/Ultrawide.h"
	"DR2Fix/Utils.h"
	"includes/Hooking.Patterns/Hooking.Patterns.cpp" "includes/Hooking.Patterns/Hooking.Patterns.h"
	"includes/MemoryMgr/MemoryMgr.h"
	"includes/safetyhook/safetyhook.cpp" "includes/safetyhook/safetyhook.hpp"
	"includes/safetyhook/Zydis.c" "includes/safetyhook/Zydis.h"
)

target_include_directories(DR2Fix PRIVATE
	"includes/Hooking.Patterns"
	"includes/MemoryMgr"
	"includes/safetyhook"
)

set_target_properties(DR2Fix PROPERTIES CXX_STANDARD 23)
