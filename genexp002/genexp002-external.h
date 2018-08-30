#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdint.h>
#include <math.h>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <dxgi1_4.h>
#include <d3d12.h>
#include <windows.h>

#define EASTL_RTTI_ENABLED 0
#define EASTL_EXCEPTIONS_ENABLED 0
#include "EASTL/vector.h"
#include "EASTL/hash_map.h"
#include "EASTL/algorithm.h"

#include "DirectXMath.h"
#include "d3dx12.h"
#include "stb_image.h"
#include "stb_perlin.h"
#include "imgui.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#define VHR(hr) if (FAILED(hr)) { assert(0); }
#define SAFE_RELEASE(obj) if ((obj)) { (obj)->Release(); (obj) = nullptr; }

using namespace DirectX;
// vim: set ts=4 sw=4 expandtab:
