#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dxgi1_4.h>
#include <d2d1_3.h>
#include <d3d11.h>

#include "stb_perlin.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")
using namespace D2D1;

#define VHR(hr) if (FAILED(hr)) { assert(0); }
#define CRELEASE(obj) if ((obj)) { (obj)->Release(); (obj) = nullptr; }
// vim: set ts=4 sw=4 expandtab:
