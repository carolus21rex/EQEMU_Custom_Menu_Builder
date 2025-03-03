#ifndef AOT_SPELLS_UI_H
#define AOT_SPELLS_UI_H
//
// This class creates a specific window (in this case Spell UI) and handles rendering.
//

#include <string>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include "AoT_Graphics_Builder.h"
#include "eqgame.h"
#include "MQ2Main.h"





class AoT_Spells_UI {
public:
    AoT_Spells_UI();
    ~AoT_Spells_UI();

    bool isOpen() const { return open; }

    // Initializes SDL and the rendering window
    bool InitializeWindow(IDirect3DDevice9*, std::string);

    void UpdateDevice(IDirect3DDevice9*);
    void UpdateCon(DWORD* pCon)
    {
        if (open)
            con = pCon;
    };

    // render master, other renders are for each possible config.
    void Render(LPDIRECT3DDEVICE9);

    // Event listener callbacks
    void minimizeClick();

    // Cleans up resources
    void Cleanup();

    DWORD* con;
private:

    

    int config;
    bool isInitialized = false;
    bool open = false;
    AoT_Menu_Builder* menuBuilder = nullptr;
    IDirect3DDevice9* device;
    int spells = 0;
};

#endif // AOT_SPELLS_UI_H
