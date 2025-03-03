#ifndef AOT_MENU_BUILDER_H
#define AOT_MENU_BUILDER_H
#define STB_TRUETYPE_IMPLEMENTATION

#include <d3d9.h> // Include Direct3D 9
#include <d3dx9math.h>
#include <d3dx9.h>
#include <windows.h>
#include <memory>
#include <vector>
#include <functional>
#include <iostream>

// Forward declarations for Direct3D 9


class AoT_Menu_Builder {
public:
    enum class EventType { None, Click, Drag };

private:
    struct TextureData {
        LPDIRECT3DTEXTURE9 texture;     // D3D9 texture for rendering
        int relX, relY;                 // Position relative to the menu
        int width, height;              // Size of the texture
        EventType eventType;            // Type of event (Click, Drag, etc.)
        std::function<void()> callback; // Callback for event

        TextureData(LPDIRECT3DTEXTURE9 tex, int x, int y, int w, int h, EventType et, std::function<void()> cb)
            : texture(tex), relX(x), relY(y), width(w), height(h), eventType(et), callback(cb) {}
    };

    LPDIRECT3DDEVICE9 d3dDevice;   // D3D9 device for rendering
    ID3DXSprite* sprite = nullptr;

    std::vector<LPDIRECT3DTEXTURE9> loadedTextures;
    std::vector<TextureData> textures; // List of textures

    bool updateAtlas = true;
    LPDIRECT3DTEXTURE9 atlasTexture = nullptr;
    IDirect3DPixelShader9* pixelShader = nullptr;
    void InitializeSprite(IDirect3DDevice9*);
    void CombineTextures(LPDIRECT3DDEVICE9);

    int menuX = 0, menuY = 0;      // Position of the menu
    int menuWidth = 0, menuHeight = 0; // Size of the menu
    int gameWidth = 0, gameHeight = 0; // Size of the menu
    bool isDragging = false;       // Flag for dragging
    bool wasMouseDown = false;
    POINT dragStartPosGlobal, dragStartPosMenu; // Drag positions
    const int dragThreshold = 5;  // Threshold for drag initiation


public:
    AoT_Menu_Builder();
    ~AoT_Menu_Builder();

    void clearTextures() // cleanup procedure. TODO: I am not sure this is enough to avoid a memory leak.
    {
        for (TextureData& tex : textures)
        {
            if (tex.texture) 
            {
                tex.texture->Release();
                tex.texture = nullptr;
            }
        }
        textures.clear();

    };

    void RenderTextToD3D9Texture(LPDIRECT3DDEVICE9 d3dDevice, char* fontFile, const char* text,
        int fontSize, D3DCOLOR color, int maxWidth, int maxHeight);
    // Generates a text font to an offscreen buffer which is then copied into loadedTextures. This function or LoadD3D9TextureFromFile must be used before AddTexture

    void LoadD3D9TextureFromFile(LPDIRECT3DDEVICE9 d3dDevice, LPCSTR filePath);
    // Adds a texture to loadedTextures from your hard drive. This function or RenderText must be used before AddTexture

    void AddTexture(int texture, int relX, int relY);
    // the first param refers to the texture in loadedTexture. YOU MUST use RenderTextToD3D9Texture OR LoadD3D9TextureFromFile first.
    void AddTexture(int texture, int relX, int relY, int width, int height);
    // Scalable size overload to AddTexture
    void AddTexture(int texture, int relX, int relY, EventType eventType, std::function<void()> callback);
    // Event listener overload to AddTexture
    void AddTexture(int texture, int relX, int relY, int width, int height, EventType eventType, std::function<void()> callback);
    // Event listener and scalable overload to AddTexture

    void ToggleTexture(int textureID, bool show);   // toggles whether to show or hide a texture
    //void UpdateDevice(IDirect3DDevice9*);

    void SetMenuPosition(int x, int y);         // Move where the menu renders to x, y
    void UpdateMenuSize();                      // An expensive call, but when you toggle textures call this to avoid side effects.
    void UpdateGameSize(int width, int height); // If the game gets resized call this to avoid side effects.

    void Render(LPDIRECT3DDEVICE9);

    void CleanUp()
    {
        clearTextures();
        delete this;
    }

    POINT getDragStartGlobal() { return dragStartPosGlobal; }   // Cursor position relative to the game display
    POINT getDragStartMenu() { return dragStartPosMenu; }       // Cursor position relative to the menu

    void PollMouseInput();                                      // This is the mouse event handler... should probably be private

private:
    bool IsCursorRelevant(int mouseX, int mouseY) const;                    // optimization
    bool IsWithinBounds(const TextureData* texture, int x, int y) const;    // optimization
};

#endif // AOT_MENU_BUILDER_H
