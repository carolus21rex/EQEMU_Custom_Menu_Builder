#include "AoT_Graphics_Builder.h"
#include "stb_truetype.h"

HWND GetEverQuestWindow() {
    // Replace with the actual window class name or title of EverQuest
    const char* windowTitle = "EverQuest";

    HWND hwnd = FindWindow(nullptr, windowTitle); // Search by title (nullptr for class name)

    if (!hwnd) {
        std::cerr << "EverQuest window not found!" << std::endl;
    }

    return hwnd;
}


char* GetAbsolutePath(const std::string& relativePath) {
    char fullPath[MAX_PATH];
    if (GetFullPathNameA(relativePath.c_str(), MAX_PATH, fullPath, nullptr)) {
        return fullPath;
    }
    return "";
}

int getWordWidth(const stbtt_fontinfo& fontInfo, const char* word, float fontSize) {
    float scale = stbtt_ScaleForPixelHeight(&fontInfo, fontSize);
    printf("Scale: %f\n", scale);

    int width = 0;
    for (const char* p = word; *p; ++p) {
        printf("codepoint: %d", *p);
        if (!stbtt_FindGlyphIndex(&fontInfo, *p)) {
            printf("Invalid codepoint: %d\n", *p);
        }

        int advanceWidth, leftBearing;
        stbtt_GetCodepointHMetrics(&fontInfo, *p, &advanceWidth, &leftBearing);

        // Ensure the scale is applied correctly. 
        // We are scaling by the pixel height and ensuring the metrics are in pixels.
        width += static_cast<int>(advanceWidth * scale);
        printf("w: %d\n", width);
    }
    return width;
}


const char* wrapTextByWord(stbtt_fontinfo& fontInfo, const char* text, float scale, int maxWidth) {
    int currentLineWidth = 0;
    const char* wordStart = text;
    const char* currentChar = text;
    char* result = new char[strlen(text) * 2]; // Allocate enough space for the wrapped text
    int resultIndex = 0;

    while (*currentChar) {
        printf("%c\n", *currentChar);
        if (*currentChar == ' ' || *(currentChar + 1) == '\0') {
            // End of word or end of string reached, calculate word width
            size_t wordLength = currentChar - wordStart + 1;
            char* word = new char[wordLength + 1];
            strncpy(word, wordStart, wordLength);
            word[wordLength] = '\0';  // Null-terminate the word
            printf("word_size calc\n");
            int wordWidth = getWordWidth(fontInfo, word, scale);

            // If adding this word exceeds max width, insert a newline before it
            if (currentLineWidth + wordWidth > maxWidth) {
                if (currentLineWidth > 0) {
                    result[resultIndex++] = '\n';  // Insert a newline
                    currentLineWidth = 0;  // Reset line width for the new line
                }
            }
            printf("added new line\n");


            strncpy(result + resultIndex, word, wordLength);
            resultIndex += wordLength;
            currentLineWidth += wordWidth;  // Update the current line width

            delete[] word;  // Free the word buffer

            // Set the next word's start position
            wordStart = currentChar + 1;
            printf("next word\n");
        }

        ++currentChar;
    }

    result[resultIndex] = '\0';  // Null-terminate the wrapped text
    //std::strcpy(text, result);   // Copy the wrapped text back to the original buffer
    printf("complete results: %s", result);

    
    return result;
}


void AoT_Menu_Builder::RenderTextToD3D9Texture(LPDIRECT3DDEVICE9 d3dDevice, char* fontFile, const char* text,
    int fontSize, D3DCOLOR color, int maxWidth, int maxHeight) {
    if (loadedTextures.capacity() != 64)
    {
        loadedTextures.reserve(64);
        loadedTextures.clear();
    }
    // fontFile = GetAbsolutePath(fontFile);
    // printf("Font path: %s\n", fontFile);

    // Load font file into memory
    FILE* font;
    errno_t err = fopen_s(&font, fontFile, "rb");
    if (err != 0 || !font) {
        std::cerr << "Error opening font file: " << fontFile << std::endl;
        return;
    }
    printf("Checkpoint 0\n");
    fseek(font, 0, SEEK_END);
    size_t fontSizeInBytes = ftell(font);
    fseek(font, 0, SEEK_SET);
    unsigned char* fontBuffer = new unsigned char[fontSizeInBytes];
    fread(fontBuffer, 1, fontSizeInBytes, font);
    fclose(font);

    printf("Checkpoint 1\n");
    // Initialize font with stb_truetype
    stbtt_fontinfo fontInfo;
    if (!stbtt_InitFont(&fontInfo, fontBuffer, stbtt_GetFontOffsetForIndex(fontBuffer, 0))) {
        std::cerr << "Error initializing font with stb_truetype" << std::endl;
        delete[] fontBuffer;
        return;
    }
    printf("Checkpoint 2\n");

    // Scale the font to the desired size
    float scale = stbtt_ScaleForPixelHeight(&fontInfo, fontSize);
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
    ascent = static_cast<int>(ascent * scale);
    descent = static_cast<int>(descent * scale);

    printf("Checkpoint 3\n");
    // Wrap text
    text = wrapTextByWord(fontInfo, text, fontSize, maxWidth);
    // Calculate text dimensions
    int width = 0, height = ascent - descent, maxW = 0;
    int textHeight = height;
    std::string q = text;
    for (int p = 0; q[p] != '\0'; ++p) {
        int advanceWidth, leftBearing;
        stbtt_GetCodepointHMetrics(&fontInfo, q[p], &advanceWidth, &leftBearing);
        if (q[p] == '\n')
        {
            width = 0;
            height += textHeight + lineGap;
        }
        else
        { 
            width += static_cast<int>(advanceWidth * scale);
        }
        maxW < width ? maxW = width : maxW = maxW;
    }

    printf("Checkpoint 4\n");

    if (width > maxWidth || height > maxHeight) {
        std::cerr << "Text exceeds the allowed dimensions" << std::endl;
        delete[] fontBuffer;
        return;
    }

    // Allocate pixel buffer for the rendered text
    int* pixels = new int[maxW * height]();  // Using int to store ARGB values
    if (!pixels) {
        std::cerr << "Error allocating pixel buffer" << std::endl;
        delete[] fontBuffer;
        return;
    }
    printf("Checkpoint 5\n");

    // Render each character
    int x = 0, y = 0; // Cursor positions for rendering
    for (const char* p = text; *p; ++p) {
        if (*p == '\n') {
            x = 0;                  // Reset x for a new line
            y += textHeight + lineGap; // Advance y to the next line
            continue;
        }

        int cWidth, cHeight, xOffset, yOffset;
        unsigned char* bitmap = stbtt_GetCodepointBitmap(&fontInfo, 0, scale, *p, &cWidth, &cHeight, &xOffset, &yOffset);

        for (int cy = 0; cy < cHeight; ++cy) {
            for (int cx = 0; cx < cWidth; ++cx) {
                int destX = x + cx + xOffset;
                int destY = y + ascent + cy + yOffset;

                if (destX >= 0 && destX < maxW && destY >= 0 && destY < height) {
                    unsigned char alpha = bitmap[cy * cWidth + cx];
                    int pixelColor = (alpha << 24) | (color & 0x00FFFFFF); // ARGB format
                    pixels[destY * maxW + destX] = pixelColor; // Correct indexing
                }
            }
        }

        stbtt_FreeBitmap(bitmap, nullptr);

        int advanceWidth, leftBearing;
        stbtt_GetCodepointHMetrics(&fontInfo, *p, &advanceWidth, &leftBearing);
        x += static_cast<int>(advanceWidth * scale);

        // Wrap text manually if exceeding maxWidth
        if (x >= maxWidth) {
            x = 0;
            y += textHeight + lineGap;
        }
    }

    printf("Checkpoint 6\n");
    // Create D3D9 texture
    LPDIRECT3DTEXTURE9 d3dTexture = nullptr;
    HRESULT hr = d3dDevice->CreateTexture(
        maxW, height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &d3dTexture, nullptr
    );

    if (FAILED(hr)) {
        std::cerr << "Error creating D3D texture: " << std::hex << hr << std::endl;
        delete[] pixels;
        delete[] fontBuffer;
        return;
    }

    // Lock the texture to fill it with the pixel data
    D3DLOCKED_RECT lockedRect;
    hr = d3dTexture->LockRect(0, &lockedRect, nullptr, D3DLOCK_DISCARD);
    if (FAILED(hr)) {
        std::cerr << "Error locking texture: " << std::hex << hr << std::endl;
        d3dTexture->Release();
        delete[] pixels;
        delete[] fontBuffer;
        return;
    }

    // Copy the pixel data into the texture
    for (int y = 0; y < height; ++y) {
        memcpy((unsigned char*)lockedRect.pBits + y * lockedRect.Pitch, &pixels[y * maxW], maxW * sizeof(int));
    }

    // Unlock the texture after copying the data
    d3dTexture->UnlockRect(0);

    // Clean up
    delete[] pixels;
    delete[] fontBuffer;

    loadedTextures.push_back(d3dTexture);
}


void AoT_Menu_Builder::LoadD3D9TextureFromFile(LPDIRECT3DDEVICE9 d3dDevice, LPCSTR filePath) {

    if (loadedTextures.capacity() != 64)
    {
        loadedTextures.reserve(64);
        loadedTextures.clear();
    }
    LPDIRECT3DTEXTURE9 pTexture = nullptr;

    // Load the texture with specific format (e.g., D3DFMT_A8R8G8B8)
    if (FAILED(D3DXCreateTextureFromFileEx(
        d3dDevice,
        filePath,
        D3DX_DEFAULT,          // Width
        D3DX_DEFAULT,          // Height
        D3DX_DEFAULT,          // Mip levels
        0,                     // Usage
        D3DFMT_A8R8G8B8,       // Desired format
        D3DPOOL_MANAGED,       // Memory pool
        D3DX_DEFAULT,          // Filter
        D3DX_DEFAULT,          // Mip filter
        0,                     // Color key (0 = no transparency by color key)
        nullptr,               // Image info (optional)
        nullptr,               // Palette (optional)
        &pTexture))) {
        std::cerr << "Failed to load texture!" << std::endl;
        return;
    }

    loadedTextures.push_back(pTexture);
}

AoT_Menu_Builder::AoT_Menu_Builder()
    : menuX(0), menuY(0), menuWidth(0), menuHeight(0), isDragging(false), wasMouseDown(false) {
    printf("survavil?\n");
    textures.reserve(512);
    printf("No?\n");
}

AoT_Menu_Builder::~AoT_Menu_Builder() {
    // Cleanup all textures
    
    textures.clear();  // Clear the texture vector
}

// Add the texture to the list with default properties (no events, no callbacks)
void AoT_Menu_Builder::AddTexture(int index, int relX, int relY) {
    printf("Count: %d", loadedTextures.size());
    if (!loadedTextures[index]) {
        std::cerr << "Invalid texture passed to AddTexture!" << std::endl;
        return;
    }

    // Retrieve the texture's width and height using Direct3D's method
    D3DSURFACE_DESC surfaceDesc;
    loadedTextures[index]->GetLevelDesc(0, &surfaceDesc);  // Get the texture's properties (e.g., width, height)
    int width = surfaceDesc.Width;
    int height = surfaceDesc.Height;
    printf("Adding texture: width=%d, height=%d, relX=%d, relY=%d\n", width, height, relX, relY);
    textures.push_back(TextureData(loadedTextures[index], relX, relY, width, height, EventType::None, nullptr));

    UpdateMenuSize();
}


// Add the texture with the specified size, very useful for scaling textures.
void AoT_Menu_Builder::AddTexture(int index, int relX, int relY, int width, int height) {
    if (!loadedTextures[index]) {
        std::cerr << "Invalid texture passed to AddTexture!" << std::endl;
        return;
    }
    printf("Adding texture: width=%d, height=%d, relX=%d, relY=%d\n", width, height, relX, relY);

    textures.push_back(TextureData(loadedTextures[index], relX, relY, width, height, EventType::None, nullptr));
    UpdateMenuSize();
}


// Add the texture with the specified event type and callback, without scaling.
void AoT_Menu_Builder::AddTexture(int index, int relX, int relY, EventType eventType, std::function<void()> callback) {
    if (!loadedTextures[index]) {
        std::cerr << "Invalid texture passed to AddTexture!" << std::endl;
        return;
    }

    // Retrieve texture dimensions
    D3DSURFACE_DESC surfaceDesc;
    loadedTextures[index]->GetLevelDesc(0, &surfaceDesc);  // Get texture width and height
    int width = surfaceDesc.Width;
    int height = surfaceDesc.Height;

    printf("Adding texture: width=%d, height=%d, relX=%d, relY=%d\n", width, height, relX, relY);
    textures.push_back(TextureData(loadedTextures[index], relX, relY, width, height, eventType, callback));
    UpdateMenuSize();
}


// Scalling and event type.
void AoT_Menu_Builder::AddTexture(int index, int relX, int relY, int width, int height, EventType eventType, std::function<void()> callback) {
    if (!loadedTextures[index]) {
        std::cerr << "Invalid texture passed to AddTexture!" << std::endl;
        return;
    }

    // Add the texture with all specified parameters
    printf("Adding texture: width=%d, height=%d, relX=%d, relY=%d\n", width, height, relX, relY);
    textures.push_back(TextureData(loadedTextures[index], relX, relY, width, height, eventType, callback));
    UpdateMenuSize();
}


void AoT_Menu_Builder::SetMenuPosition(int x, int y) {

    HWND hwnd = GetEverQuestWindow();
    if (!hwnd) {
        gameWidth = 0;
        gameHeight = 0;
        return;
    }

    RECT rect;
    if (GetWindowRect(hwnd, &rect)) {
        gameWidth = rect.right - rect.left;
        gameHeight = rect.bottom - rect.top;
    }
    else {
        std::cerr << "Failed to get EverQuest window size!" << std::endl;
        gameWidth = 0;
        gameHeight = 0;
    }


    if (x + menuWidth > gameWidth)
        x = gameWidth - menuWidth;
    
    if (y + menuHeight > gameHeight)
        y = gameHeight - menuHeight;

    menuX = x;
    menuY = y;
}

void AoT_Menu_Builder::UpdateMenuSize() {
    printf("Updating menu size\n");

    int width = 0;
    int height = 0;

    // Iterate through the texture list to calculate the menu size
    for (const auto& texture : textures) {
        int textureRight = texture.relX + texture.width;
        int textureBottom = texture.relY + texture.height;

        // Update the width and height based on the farthest texture edges
        if (textureRight > width) {
            width = textureRight;
        }
        if (textureBottom > height) {
            height = textureBottom;
        }
    }

    // Update menu dimensions
    menuWidth = width;
    menuHeight = height;

    // Ensure the menu position is within valid screen bounds
    SetMenuPosition(menuX, menuY);
}


void AoT_Menu_Builder::UpdateGameSize(int width, int height) {
    printf("updating game size\n");
    gameWidth = width;
    gameHeight = height;
    // offscreen menus aren't ideal
    SetMenuPosition(menuX, menuY);
}


struct Vertex
{
    float x, y, z, rhw, u, v;
};


const char* GetFormatString(D3DFORMAT format) {
    switch (format) {
    case D3DFMT_A8R8G8B8: return "D3DFMT_A8R8G8B8";
    case D3DFMT_X8R8G8B8: return "D3DFMT_X8R8G8B8";
    case D3DFMT_R5G6B5:   return "D3DFMT_R5G6B5";
    case D3DFMT_X1R5G5B5: return "D3DFMT_X1R5G5B5";
    case D3DFMT_A1R5G5B5: return "D3DFMT_A1R5G5B5";
    case D3DFMT_A4R4G4B4: return "D3DFMT_A4R4G4B4";
    case D3DFMT_R3G3B2:   return "D3DFMT_R3G3B2";
    case D3DFMT_A8:       return "D3DFMT_A8";
    case D3DFMT_A8P8:     return "D3DFMT_A8P8";
    case D3DFMT_P8:       return "D3DFMT_P8";
    case D3DFMT_L8:       return "D3DFMT_L8";
    case D3DFMT_A8L8:     return "D3DFMT_A8L8";
    case D3DFMT_V8U8:     return "D3DFMT_V8U8";
    case D3DFMT_DXT1:     return "D3DFMT_DXT1";
    case D3DFMT_DXT2:     return "D3DFMT_DXT2";
    case D3DFMT_DXT3:     return "D3DFMT_DXT3";
    case D3DFMT_DXT4:     return "D3DFMT_DXT4";
    case D3DFMT_DXT5:     return "D3DFMT_DXT5";
    default:              return "Unknown Format";
    }
}





void AoT_Menu_Builder::CombineTextures(IDirect3DDevice9* device) {
    IDirect3DStateBlock9* stateBlock = nullptr;
    HRESULT hr = device->CreateStateBlock(D3DSBT_ALL, &stateBlock);
    if (FAILED(hr)) {
        std::cerr << "Failed to create state block!" << std::endl;
        return;
    }
    stateBlock->Capture();

    // Create the atlas texture
    atlasTexture = nullptr;
    hr = device->CreateTexture(
        menuWidth, menuHeight, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &atlasTexture, nullptr);
    if (FAILED(hr)) {
        std::cerr << "Failed to create atlas texture!" << std::endl;
        return;
    }



    for (const auto& textureData : textures) {
        // Get the surface of the source texture
        LPDIRECT3DSURFACE9 srcSurface = nullptr;
        textureData.texture->GetSurfaceLevel(0, &srcSurface);

        D3DLOCKED_RECT srcLockedRect;
        D3DLOCKED_RECT destLockedRect;

        // Lock the source surface to access its pixels
        hr = srcSurface->LockRect(&srcLockedRect, nullptr, D3DLOCK_READONLY);
        if (FAILED(hr)) {
            std::cerr << "Failed to lock source surface!" << std::endl;
            srcSurface->Release();
            continue;
        }
        D3DSURFACE_DESC srcDesc;
        srcSurface->GetDesc(&srcDesc);
        int srcWidth = srcDesc.Width;
        int srcHeight = srcDesc.Height;
        

        // Lock the destination surface to access its pixels
        hr = atlasTexture->LockRect(0, &destLockedRect, nullptr, 0);
        if (FAILED(hr)) {
            std::cerr << "Failed to lock destination surface!" << std::endl;
            srcSurface->UnlockRect();
            srcSurface->Release();
            continue;
        }

        // Calculate scaling factors
        float scaleX = (float)textureData.width / (float)srcWidth;
        float scaleY = (float)textureData.height / (float)srcHeight;

        // Manually copy and scale the pixels
        for (int y = 0; y < textureData.height; ++y) {
            for (int x = 0; x < textureData.width; ++x) {
                // Find corresponding source pixel in the scaled position
                int srcX = (int)(x / scaleX);
                int srcY = (int)(y / scaleY);

                if (srcX < 0 || srcX >= srcWidth || srcY < 0 || srcY >= srcHeight) {
                    continue; // Skip if the source coordinates are out of bounds
                }

                uint8_t* srcPixel = (uint8_t*)srcLockedRect.pBits + srcY * srcLockedRect.Pitch + srcX * 4;  // RGBA, 4 bytes per pixel

                // Apply opacity by modifying the alpha channel
                srcPixel[3] = static_cast<unsigned char>(srcPixel[3]);  // Adjust alpha based on opacity
                if (srcPixel[3] == 0)
                    continue;  // Skip fully transparent pixels

                // Calculate the pixel offset in the destination surface
                uint8_t* destPixel = (uint8_t*)destLockedRect.pBits + (textureData.relY + y) * destLockedRect.Pitch + (textureData.relX + x) * 4;

                // Perform opacity blending
                if (srcPixel[3] < 255) {
                    float opacity = (float)srcPixel[3] / 255.0f;
                    destPixel[0] = (unsigned char)(srcPixel[0] * opacity + destPixel[0] * (1 - opacity));  // Blue
                    destPixel[1] = (unsigned char)(srcPixel[1] * opacity + destPixel[1] * (1 - opacity));  // Green
                    destPixel[2] = (unsigned char)(srcPixel[2] * opacity + destPixel[2] * (1 - opacity));  // Red
                }
                else {
                    destPixel[0] = srcPixel[0];  // Blue
                    destPixel[1] = srcPixel[1];  // Green
                    destPixel[2] = srcPixel[2];  // Red
                    destPixel[3] = 255;  // Fully opaque
                }
            }
        }

        // Unlock the surfaces
        srcSurface->UnlockRect();
        atlasTexture->UnlockRect(0);

        // Release the source surface
        srcSurface->Release();
    }

    updateAtlas = false;
}






void AoT_Menu_Builder::InitializeSprite(IDirect3DDevice9* device) {
    if (!sprite) {
        if (FAILED(D3DXCreateSprite(device, &sprite))) {
            std::cerr << "Failed to create sprite!" << std::endl;
        }
    }
}


void AoT_Menu_Builder::Render(LPDIRECT3DDEVICE9 device) {
    InitializeSprite(device);
    if (updateAtlas)
        CombineTextures(device);

    // Load texture if not already loaded


    // Begin sprite drawing
    sprite->Begin(D3DXSPRITE_ALPHABLEND); // Enable alpha blending

    // Define the position on the screen where the texture will be drawn
    D3DXVECTOR3 position1(menuX, menuY, 0.0f);
    sprite->Draw(
        atlasTexture,                // Reused texture
        nullptr,                            // Full texture
        nullptr,                            // No scaling/rotation center
        &position1,                         // First position
        D3DCOLOR_ARGB(255, 255, 255, 255)   // Full opacity
    );




    // End sprite drawing
    sprite->End();

    PollMouseInput();
}




void AoT_Menu_Builder::PollMouseInput() {
    // Chatgpt code, can you tell? it works lol
    // Get current mouse position relative to the game window
    POINT cursorPos;
    GetCursorPos(&cursorPos);  // Get cursor position in screen coordinates
    ScreenToClient(GetEverQuestWindow(), &cursorPos);  // Convert to client (menu) coordinates
    int mouseX = cursorPos.x;
    int mouseY = cursorPos.y;

    // Check left mouse button state
    bool isMouseDown = GetAsyncKeyState(VK_LBUTTON) & 0x8000;

    // Optimization: Early exit if cursor is irrelevant
    if (!IsCursorRelevant(mouseX- menuX, mouseY -menuY)) {
        return; // Skip further processing if mouse is outside the menu area
    }

    // Dragging logic
    if (isMouseDown) {
        if (!isDragging) {
            // If drag hasn't started, initialize drag start position and check threshold
            if (dragStartPosGlobal.x == -1 && dragStartPosGlobal.y == -1) {
                dragStartPosGlobal = { mouseX, mouseY };
                dragStartPosMenu = { mouseX - menuX, mouseY - menuY };
            }

            int deltaX = abs(mouseX - dragStartPosGlobal.x);
            int deltaY = abs(mouseY - dragStartPosGlobal.y);

            if (deltaX > dragThreshold || deltaY > dragThreshold) {
                isDragging = true;  // Start dragging
            }
        }

        if (isDragging) {
            for (const auto& texture : textures) {
                if (texture.eventType == EventType::Drag &&
                    IsWithinBounds(&texture, dragStartPosMenu.x, dragStartPosMenu.y)) {
                    // Update texture callback based on the new relative position
                    int relX = mouseX - menuX - texture.relX;
                    int relY = mouseY - menuY - texture.relY;
                    texture.callback();  // Call the callback function for drag event
                }
            }
        }
        wasMouseDown = true;  // Remember mouse down state
    }
    else {
        if (isDragging) {
            isDragging = false;  // Reset drag state when mouse is released
        }

        // On mouse release, check for click events
        if (wasMouseDown)
        {
            for (const auto& texture : textures) {
                if (texture.eventType == EventType::Click &&
                    IsWithinBounds(&texture, mouseX - menuX, mouseY - menuY)) {
                    int relX = mouseX - menuX - texture.relX;
                    int relY = mouseY - menuY - texture.relY;
                    texture.callback();  // Call the callback function for click event
                }
            }
            wasMouseDown = false;  // Reset mouse down state after click handling
        }

        // Reset drag start positions after the mouse is released
        dragStartPosGlobal = { -1, -1 };
        dragStartPosMenu = { -1, -1 };
    }
}


bool AoT_Menu_Builder::IsCursorRelevant(int mouseX, int mouseY) const {
    if (isDragging) {
        return true;  // If dragging, cursor is always relevant
    }

    // Check each texture to see if the cursor is inside a clickable or draggable area
    for (const auto& texture : textures) {
        if (IsWithinBounds(&texture, mouseX, mouseY) &&
            (texture.eventType == EventType::Click || texture.eventType == EventType::Drag)) {
            return true;
        }
    }

    return false;  // No relevant texture found under the cursor
}

void AoT_Menu_Builder::ToggleTexture(int textureID, bool show)
{
    return;
    //textures[textureID]->displayed = show;
}
/*
void AoT_Menu_Builder::UpdateDevice(LPDIRECT3DDEVICE9 device)
{
    //d3dDevice = device;
}
*/

bool AoT_Menu_Builder::IsWithinBounds(const TextureData* texture, int x, int y) const {
    // Check if the (x, y) coordinates are within the bounds of the texture
    return x >= texture->relX && x < texture->relX + texture->width &&
        y >= texture->relY && y < texture->relY + texture->height;
}

