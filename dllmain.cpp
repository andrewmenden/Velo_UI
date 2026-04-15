#include "App.hpp"
#include "AudioCapture.hpp"

// interface functions for communication between Velo.dll and Velo_UI.dll
// note that there are also a couple of utility functions that don't have anything to do with the settings UI

extern "C"
{
    // Called once at the very beginning to check whether the necessary 
    // Visual C++ Redistributables are installed (fails if not)
    __declspec(dllexport) void __cdecl Test() {}

    __declspec(dllexport) void __cdecl Memcpy(void* dst, void* src, size_t size)
    {
        memcpy(dst, src, size);
    }

    VOID CALLBACK WinEventProcCallback(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
    {
        if (dwEvent == EVENT_SYSTEM_FOREGROUND || dwEvent == EVENT_SYSTEM_MINIMIZEEND)
        {
            Global::foregroundHwnd = hwnd;
        }
    }

    __declspec(dllexport) void __cdecl SetGameHwnd(HWND hwnd)
    {
        Global::gameHwnd = hwnd;
        Global::hhkWinEvent = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_MINIMIZEEND, NULL, WinEventProcCallback, 0, 0, WINEVENT_OUTOFCONTEXT);
        Global::foregroundHwnd = GetForegroundWindow();
    }

    __declspec(dllexport) int32_t __cdecl IsGameFocused()
    {
        auto now = std::chrono::high_resolution_clock::now();
        if (now - Global::lastForegroundHwndRefresh >= std::chrono::milliseconds(500))
        {
            Global::foregroundHwnd = GetForegroundWindow(); // help out the foreground detected every now and then
            Global::lastForegroundHwndRefresh = now;
        }
        return Global::gameHwnd == Global::foregroundHwnd;
    }

    __declspec(dllexport) void __cdecl InitializeImGui_d3d11(IDXGISwapChain* swapChain)
    {
        Global::graphicsDeviceType = gdtD3D11;
        Global::swapChain = swapChain;
        swapChain->GetDevice(__uuidof(ID3D11Device), (void**)&Global::deviced3d11);
        Global::deviced3d11->GetImmediateContext(&Global::deviceContext);
        DXGI_SWAP_CHAIN_DESC sd;
        swapChain->GetDesc(&sd);
        ImGui::CreateContext();
        ImGui_ImplSDL2_InitForD3D(SDL_GetWindowFromID(1));
        ImGui_ImplDX11_Init(Global::deviced3d11, Global::deviceContext);
        Global::app.Init();
    }

    __declspec(dllexport) void __cdecl InitializeImGui_opengl()
    {
        Global::graphicsDeviceType = gdtOPENGL3;
        ImGui::CreateContext();
        ImGui_ImplSDL2_InitForOpenGL(SDL_GL_GetCurrentWindow(), SDL_GL_GetCurrentContext());
        ImGui_ImplOpenGL3_Init();
        Global::app.Init();
    }

    __declspec(dllexport) void __cdecl UnfocusAll()
    {
        ImGuiContext* ctx = ImGui::GetCurrentContext();
        for (auto& window : ctx->Windows)
        {
            window->Active = false;
        }
    }

    __declspec(dllexport) void __cdecl RenderImGui(float mouseX, float mouseY, float frameW, float frameH)
    {
        Global::windowSize = ImVec2{ frameW, frameH };

        switch (Global::graphicsDeviceType)
        {
        case gdtD3D11:
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            break;
        case gdtOPENGL3:
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            break;
        }

        ImGui::NewFrame();
        ImGui::GetIO().DisplaySize = ImVec2{ frameW, frameH };

        Global::app.RenderImGui();

        ImGui::EndFrame();
        ImGui::Render();

        switch (Global::graphicsDeviceType)
        {
        case gdtD3D11:
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            break;
        case gdtOPENGL3:
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        }
    }

    __declspec(dllexport) void __cdecl ShutdownImGui()
    {
        switch (Global::graphicsDeviceType)
        {
        case gdtD3D11:
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplSDL2_Shutdown();
            break;
        case gdtOPENGL3:
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplSDL2_Shutdown();
            break;
        }
        ImGui::DestroyContext();
    }

    __declspec(dllexport) void __cdecl ProcessEvent(SDL_Event *event)
    {
        ImGui_ImplSDL2_ProcessEvent(event);
    }

    // Initialize all settings from Velo
    __declspec(dllexport) void __cdecl LoadFromJson(const char* json)
    {
        nlohmann::json jsonData = nlohmann::json::parse(json);
        Global::app.LoadFromJson(jsonData);
    }

    // Update settings from Velo
    __declspec(dllexport) void __cdecl UpdateFromJson(const char* json)
    {
        nlohmann::json jsonData = nlohmann::json::parse(json);
        Global::app.UpdateFromJson(jsonData);
    }

    // Send setting changes to Velo
    __declspec(dllexport) char* __cdecl GetUpdatesAsJson()
    {
        std::string json = Global::app.ChangesAsJsonString();
        if (json.empty())
            return nullptr;
        char* out = (char*)CoTaskMemAlloc(json.size() + 1);
        if (out == nullptr)
            return nullptr;
        strcpy_s(out, json.size() + 1, json.data());
        return out;
    }

    // Is any ImGui item focused
    __declspec(dllexport) int32_t __cdecl IsAnyFocused()
    {
        return Global::graphicsDeviceType != gdtNONE && ImGui::IsAnyItemActive();
    }

    // Used for leaderboard run verification
    __declspec(dllexport) int32_t __cdecl WindowDragged()
    {
        return std::exchange(Global::windowDragged, false);
    }

    __declspec(dllexport) void __cdecl GetGamePadButtonState(char* pressed)
    {
        std::ranges::copy(
            std::span<char>{ pressed, Global::gamePadButtonsDown.size() } | 
                std::views::transform([](char c) { return (bool)c; }), 
            Global::gamePadButtonsDown.begin()
        );
    }

    LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        if (nCode == HC_ACTION)
        {
            PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
            switch (wParam)
            {
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                if (p->vkCode < 256)
                    Global::keysDown[p->vkCode] = true;
                break;
            case WM_KEYUP:
            case WM_SYSKEYUP:
                if (p->vkCode < 256)
                    Global::keysDown[p->vkCode] = false;
                break;
            }
        }
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        if (nCode == HC_ACTION)
        {
            switch (wParam)
            {
            case WM_LBUTTONDOWN:
                Global::keysDown[VK_LBUTTON] = true;
                break;
            case WM_LBUTTONUP:
                Global::keysDown[VK_LBUTTON] = false;
                break;
            case WM_RBUTTONDOWN:
                Global::keysDown[VK_RBUTTON] = true;
                break;
            case WM_RBUTTONUP:
                Global::keysDown[VK_RBUTTON] = false;
                break;
            case WM_MBUTTONDOWN:
                Global::keysDown[VK_MBUTTON] = true;
                break;
            case WM_MBUTTONUP:
                Global::keysDown[VK_MBUTTON] = false;
                break;
            case WM_XBUTTONDOWN:
            {
                PMSLLHOOKSTRUCT p = (PMSLLHOOKSTRUCT)lParam;
                WORD button = ((PWORD)&p->mouseData)[1];
                if (button == XBUTTON1)
                    Global::keysDown[VK_XBUTTON1] = true;
                else if (button == XBUTTON2)
                    Global::keysDown[VK_XBUTTON2] = true;
                break;
            }
            case WM_XBUTTONUP:
            {
                PMSLLHOOKSTRUCT p = (PMSLLHOOKSTRUCT)lParam;
                WORD button = ((PWORD)&p->mouseData)[1];
                if (button == XBUTTON1)
                    Global::keysDown[VK_XBUTTON1] = false;
                else if (button == XBUTTON2)
                    Global::keysDown[VK_XBUTTON2] = false;
                break;
            }
            }
        }
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        if (nCode >= HC_ACTION)
        {
            LPCWPRETSTRUCT cwp = (LPCWPRETSTRUCT)lParam;
            if (cwp->message == WM_WINDOWPOSCHANGING)
                Global::windowDragged = true;
        }
        
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    __declspec(dllexport) void __cdecl InitLLKeyboardHook()
    {
        if (Global::hhkLLKeyboard != NULL)
            UnhookWindowsHookEx(Global::hhkLLKeyboard);

        Global::hhkLLKeyboard = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);
    }

    __declspec(dllexport) void __cdecl InitLLMouseHook()
    {
        if (Global::hhkLLMouse != NULL)
            UnhookWindowsHookEx(Global::hhkLLMouse);

        Global::hhkLLMouse = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, 0, 0);

        Global::keysDown[VK_LBUTTON] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) > 0;
        Global::keysDown[VK_RBUTTON] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) > 0;
        Global::keysDown[VK_MBUTTON] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) > 0;
        Global::keysDown[VK_XBUTTON1] = (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) > 0;
        Global::keysDown[VK_XBUTTON2] = (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) > 0;
    }

    __declspec(dllexport) void __cdecl InitWndProcHook()
    {
        if (Global::hhkCallWndProc != NULL)
            UnhookWindowsHookEx(Global::hhkCallWndProc);

        HMODULE hInst = GetModuleHandle(0);
        Global::hhkCallWndProc = SetWindowsHookEx(WH_CALLWNDPROCRET, CallWndProc, hInst, GetCurrentThreadId());
    }

    __declspec(dllexport) void __cdecl RemoveHooks()
    {
        if (Global::hhkLLKeyboard != NULL)
            UnhookWindowsHookEx(Global::hhkLLKeyboard);
        Global::hhkLLKeyboard = NULL;
        if (Global::hhkLLMouse != NULL)
            UnhookWindowsHookEx(Global::hhkLLMouse);
        Global::hhkLLMouse = NULL;
        if (Global::hhkCallWndProc != NULL)
            UnhookWindowsHookEx(Global::hhkCallWndProc);
        Global::hhkCallWndProc = NULL;
    }

    __declspec(dllexport) void __cdecl PollLLHooks()
    {
        MSG msg;
        for (size_t i = 0; Global::hhkLLKeyboard != NULL && PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE | PM_QS_INPUT) && i < 100; i++)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        for (size_t i = 0; Global::hhkLLMouse != NULL && PeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE | PM_QS_INPUT) && i < 100; i++)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    __declspec(dllexport) bool __cdecl IsKeyDown(uint8_t key)
    {
        return Global::keysDown[key];
    }

    __declspec(dllexport) int __cdecl GetPressedKey()
    {
        return getPressedKey(0);
    }

    __declspec(dllexport) void __cdecl ClearKeys()
    {
        std::fill(Global::keysDown.begin(), Global::keysDown.end(), false);
    }

    __declspec(dllexport) char* __cdecl KeyToString(uint16_t key)
    {
        std::string string = keyCodeToString(key);
        char* out = (char*)CoTaskMemAlloc(string.size() + 1);
        if (out == nullptr)
            return nullptr;
        strcpy_s(out, string.size() + 1, string.data());
        return out;
    }

#pragma comment (lib, "ntdll.lib")

    NTSYSAPI NTSTATUS NTAPI NtSetTimerResolution(ULONG DesiredResolution, BOOLEAN SetResolution, PULONG CurrentResolution);
    NTSYSAPI NTSTATUS NTAPI NtQueryTimerResolution(OUT PULONG MinimumResolution, OUT PULONG MaximumResolution, OUT PULONG CurrentResolution);

    __declspec(dllexport) void __cdecl SetMaximumTimerResolution()
    {
        ULONG minimum;
        ULONG maximum;
        ULONG current;
        NtQueryTimerResolution(&minimum, &maximum, &current);
        NtSetTimerResolution(maximum, true, &current);
    }

    __declspec(dllexport) void __cdecl ResetTimerResolution()
    {
        ULONG current;
        NtSetTimerResolution(10000, true, &current);
    }

    // Used by the `capture` command
    __declspec(dllexport) void __cdecl StartAudioCapture(int sampleRate)
    {
        AudioCapture::Start(sampleRate);
    }
    
    // Used by the `capture` command
    __declspec(dllexport) void __cdecl StopAudioCapture(const char* filename)
    {
        AudioCapture::Stop(filename);
    }
}