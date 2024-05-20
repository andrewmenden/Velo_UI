#include "App.hpp"

using namespace Global;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern "C"
{
    WNDPROC origWndProc = nullptr;
    bool initialized = false;
    int resizing = 0;

    __declspec(dllexport) void InitializeImGui_d3d9(IDirect3DDevice9* device);
    __declspec(dllexport) void ShutdownImGui();

    LRESULT CALLBACK WndProcHook(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_SIZE:
        case WM_SIZING:
        case WM_ENTERSIZEMOVE:
        case WM_EXITSIZEMOVE:
        case WM_GETMINMAXINFO:
        case WM_ACTIVATE:
        case WM_ACTIVATEAPP:
            resizing = 2;
            if (initialized)
                ShutdownImGui();
            break;
        }

        if (initialized)
            ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam);
        return origWndProc(hwnd, uMsg, wParam, lParam);
    }

    __declspec(dllexport) void InitializeImGui_d3d9(IDirect3DDevice9* device)
    {
        gdt = gdtD3D9;
        deviced3d9 = device;
        D3DDEVICE_CREATION_PARAMETERS params;
        device->GetCreationParameters(&params);
        hwnd = params.hFocusWindow;

        ImGui::CreateContext();
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX9_Init(device);

        if (!origWndProc)
            origWndProc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG)&WndProcHook);
        initialized = true;

        app.setIo();
    }

    __declspec(dllexport) void InitializeImGui_d3d11(IDXGISwapChain* swapChain)
    {
        gdt = gdtD3D11;
        swapChain->GetDevice(__uuidof(ID3D11Device), (void**)&deviced3d11);
        deviced3d11->GetImmediateContext(&deviceContext);
        DXGI_SWAP_CHAIN_DESC sd;
        swapChain->GetDesc(&sd);
        hwnd = sd.OutputWindow;
        ImGui::CreateContext();
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(deviced3d11, deviceContext);
        app.setIo();
        app.loadFonts(16.0f);
    }

    __declspec(dllexport) void InitializeImGui_opengl()
    {
        gdt = gdtOPENGL3;
        ImGui::CreateContext();
        ImGui_ImplSDL2_InitForOpenGL(SDL_GL_GetCurrentWindow(), SDL_GL_GetCurrentContext());
        ImGui_ImplOpenGL3_Init();
        app.setIo();
        app.loadFonts(16.0f);
    }

    __declspec(dllexport) void unfocusAll()
    {
        ImGuiContext* ctx = ImGui::GetCurrentContext();
        for (int i = 0; i < ctx->Windows.Size; ++i)
        {
            ImGuiWindow* window = ctx->Windows[i];
            window->Active = false;
        }
    }

    __declspec(dllexport) void setUiHotkey(uint8_t keyCode)
    {
        Settings::EnableUI = keyCode;
    }

    __declspec(dllexport) void RenderImGui(float mouseX, float mouseY, float frameW, float frameH)
    {
        windowSize = ImVec2(frameW, frameH);

        switch (gdt)
        {
        case gdtD3D9:
            if (resizing-- >= 1)
                return;
            if (!initialized)
            {
                InitializeImGui_d3d9(deviced3d9);
                app.loadFonts(16.0f);
            }
            ImGui_ImplDX9_NewFrame();
            ImGui_ImplWin32_NewFrame();
            break;
        case gdtD3D11:
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            break;
        case gdtOPENGL3:
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            break;
        }

        ImGui::NewFrame();
        ImGui::GetIO().DisplaySize = ImVec2(frameW, frameH);
        //ImGui::GetIO().MousePos.x = mouseX;
        //ImGui::GetIO().MousePos.y = mouseY;

        app.RenderImGui();

        ImGui::EndFrame();
        ImGui::Render();

        switch (gdt)
        {
        case gdtD3D9:
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            break;
        case gdtD3D11:
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            break;
        case gdtOPENGL3:
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        }
    }

    __declspec(dllexport) void ShutdownImGui()
    {
        switch (gdt)
        {
        case gdtD3D9:
            ImGui_ImplDX9_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            initialized = false;
            return;
        case gdtD3D11:
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            break;
        case gdtOPENGL3:
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplSDL2_Shutdown();
            break;
        }
        ImGui::DestroyContext();
    }

    __declspec(dllexport) void ProcessEvent(SDL_Event* event)
    {
        ImGui_ImplSDL2_ProcessEvent(event);
    }

    //Load the ui on startup
    __declspec(dllexport) void __cdecl LoadProgram(char* json, int size)
    {
        nlohmann::json jsonData = nlohmann::json::parse(json, json + size);
        app.LoadFromJson(jsonData);
    }

    //Update the ui with new values
    __declspec(dllexport) void __cdecl UpdateProgram(char* json, int size)
    {
        nlohmann::json jsonData = nlohmann::json::parse(json, json + size);
        app.UpdateJson(jsonData);
    }

    //returns 0 if no changes have happened-- else returns the size of the json to be loaded into byte[]
    __declspec(dllexport) int __cdecl GetChangeSize()
    {
        return app.GetChangeSize();
    }

    //get the json {"Changes:[...]"} string from c++
    __declspec(dllexport) void __cdecl GetJsonUpdates(char* json)
    {
        strcpy(json, app.GetJsonString().c_str());
    }
}