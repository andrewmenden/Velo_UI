#ifndef GLOBAL_H
#define GLOBAL_H

#define NOMINMAX

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
	constexpr const char* iniPath = "Velo\\imgui.ini";
	constexpr const char* fontPath = "Velo\\fonts\\UiFont.ttf";
	constexpr const char* iconsPath = "Velo\\fonts\\Icons.ttf";

	// Game's window size, updated on every frame via the RenderImGui function
	inline ImVec2 windowSize{};

	// Game's window handle
	inline HWND gameHwnd = NULL;

	// Foreground window handle
	inline HWND foregroundHwnd = NULL;

	// d3d11 handles
	inline ID3D11Device* deviced3d11 = NULL;
	inline ID3D11DeviceContext* deviceContext = nullptr;
	inline IDXGISwapChain* swapChain = nullptr;

	// Graphics device type (either d3d11 or OpenGL)
	inline GraphicsDeviceType graphicsDeviceType = gdtNONE;

	// Game's window was dragged
	inline bool windowDragged = false;

	// Key inputs
	inline std::array<bool, 256> keysDown{};

	// Game pad button inputs
	inline std::array<bool, 17> gamePadButtonsDown{};

	// Hooks
	inline HHOOK hhkLLKeyboard = NULL;
	inline HHOOK hhkLLMouse = NULL;
	inline HHOOK hhkCallWndProc = NULL;
	inline HWINEVENTHOOK hhkWinEvent = NULL;
}

#endif