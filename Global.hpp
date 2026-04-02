#ifndef GLOBAL_H
#define GLOBAL_H

#include <Windows.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_sdl2.h>
#include <SDL_syswm.h>
#include <d3d11.h>
#include <dxgi.h>
#include <SDL.h>

enum GraphicsDeviceType
{
	gdtNONE,
	gdtD3D11,
	gdtOPENGL3
};

// Global state accessible from anywhere (try to minimize this as much as possible)
namespace Global
{
	// Game's window size, updated on every frame via the RenderImGui function
	inline ImVec2 windowSize{};

	// Settings input width, updates on every frame
	inline float inputWidth{};

	// UI toggle key, updates on every frame
	inline int enableUiKey{};

	// True if any setting has changed
	inline bool settingChanged{};

	// Game's window handle
	inline HWND gameHwnd = NULL;

	// d3d11 handles
	ID3D11Device* deviced3d11 = NULL;
	ID3D11DeviceContext* deviceContext = nullptr;
	IDXGISwapChain* swapChain = nullptr;

	// Graphics device type (either d3d11 or OpenGL)
	GraphicsDeviceType graphicsDeviceType = gdtNONE;

	// Game's window was dragged
	inline bool windowDragged = false;

	// Key inputs
	inline std::array<bool, 256> keysDown{};

	// Game pad button inputs
	inline std::array<bool, 17> gamePadButtonsDown{};

	// Hooks
	HHOOK hhkLLKeyboard = NULL;
	HHOOK hhkLLMouse = NULL;
	HHOOK hhkCallWndProc = NULL;
}

#endif