
#include "AoT_LvUp_Spells.h"

// Global variable for Direct3D device
IDirect3DDevice9* g_pd3dDevice = nullptr;

// Function to retrieve the base address of the game module
uintptr_t GetGameBaseAddress() {
    HMODULE hModule = GetModuleHandle("eqgame.exe");
    if (hModule) {
        return reinterpret_cast<uintptr_t>(hModule);
    }
    MessageBox(NULL, "Failed to find eqgame.exe module!", "Error", MB_OK);
    return 0;
}

// Function to calculate the Direct3D device address
uintptr_t GetDeviceAddress() {
    uintptr_t baseAddress = GetGameBaseAddress();
    if (baseAddress == 0) {
        return 0; // Failed to find base address
    }

    // Offset for the Direct3D device (you can toggle between EC8 or F08 if needed)
    uintptr_t offset = 0xEC8; // Adjust this based on your findings

    // Return the calculated address
    return baseAddress + offset;
}

// Function to initialize the global Direct3D device pointer
bool InitializeDevicePointer() {
    uintptr_t deviceAddress = GetDeviceAddress();
    if (deviceAddress == 0) {
        return false; // Failed to calculate device address
    }

    // Dereference the calculated address to retrieve the device
    g_pd3dDevice = *reinterpret_cast<IDirect3DDevice9**>(deviceAddress);
    if (!g_pd3dDevice) {
        MessageBox(NULL, "Failed to retrieve Direct3D device pointer!", "Error", MB_OK);
        return false;
    }

    MessageBox(NULL, "Direct3D device pointer initialized successfully!", "Success", MB_OK);
    return true;
}

// ImGui window function
void OpenLevelUpSpellWindow() {
    if (!g_pd3dDevice) {
        MessageBox(NULL, "Direct3D device pointer is null.", "Error", MB_OK);
        return;
    }

    // Initialize ImGui
    ImGui::CreateContext();
    HWND hwnd = GetActiveWindow();
    if (!ImGui_ImplWin32_Init(hwnd)) {
        MessageBox(NULL, "Failed to initialize ImGui Win32 platform.", "Error", MB_OK);
        return;
    }
    if (!ImGui_ImplDX9_Init(g_pd3dDevice)) {
        MessageBox(NULL, "Failed to initialize ImGui DirectX9 platform.", "Error", MB_OK);
        return;
    }

    // Render loop
    bool show_window = true;
    while (show_window) {
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Level Up Spells", &show_window, ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::Button("Close Window")) {
            show_window = false;
        }
        ImGui::End();

        ImGui::Render();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0) {
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
    }

    // Shutdown ImGui
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}