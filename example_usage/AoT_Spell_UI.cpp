#include "AoT_Spell_UI.h"

struct Entry {
    int id;
    std::string name;
    std::string description;
};

/*
std::function<void()> sendPacket(AoT_Spells_UI* menu, int param) {
    return [menu, param]() {
        // Define the function pointer type based on the function's signature
        printf("test");
        typedef void (*SendMessageType)(
            DWORD* prevCon,
            int param1,
            int param2,
            const char* packetBuffer,
            size_t bufferSize,
            int param5,
            int param6
            );

        // Calculate the address of the function
        DWORD var = (((DWORD)0x008C4CE0 - 0x400000) + baseAddress);

        // Cast the address to a callable function pointer
        SendMessageType SendMessageFunction = reinterpret_cast<SendMessageType>(var);

        // Constant args
        int param1 = 4;
        int param2 = 4;
        int param5 = 0;
        int param6 = 0;

        // Packet data setup
        int packet_opcode = 2214; // Decimal interpretation of popup response opcode
        int packet_data = param;
        char packet_buffer[10];

        // Copy the opcode (2 bytes)
        memcpy(packet_buffer, &packet_opcode, 2);

        // Set button clicked (unused, 0)
        packet_buffer[2] = 0x00;

        // Set unknown fields to 0x00
        memset(packet_buffer + 3, 0x00, 3);

        // Set data (Bytes 6-9) in little-endian
        memcpy(packet_buffer + 6, &packet_data, 4);

        // Call the function at the calculated address
        SendMessageFunction(prevCon, param1, param2, packet_buffer, sizeof(packet_buffer), param5, param6);

    };
    menu->Cleanup();
}
*/

std::function<void()> sendPacket(AoT_Spells_UI* menu, int param) {
    return [menu, param]() {
        typedef unsigned char(__fastcall* SendMessageType)(
            DWORD* con,               // Passed in ECX
            unsigned __int32 unk,     // First parameter
            unsigned __int32 channel, // Second parameter
            char* buf,                // Buffer
            size_t size,              // Buffer size
            DWORD a6,                 // Extra argument
            DWORD a7                  // Extra argument
            );

        // Calculate the address of the function
        DWORD var = (((DWORD)0x008C4CE0 - 0x400000) + baseAddress);

        // Cast the address to a callable function pointer
        SendMessageType SendMessageFunction = reinterpret_cast<SendMessageType>(var);

        // Constant args

        // Packet data setup
        int packet_opcode = 2214; // Decimal interpretation of popup response opcode
        int packet_data = param;
        char packet_buffer[10];

        // Copy the opcode (2 bytes)
        memcpy(packet_buffer, &packet_opcode, 2);

        // Set button clicked (unused, 0)
        packet_buffer[2] = 0x00;

        // Set unknown fields to 0x00
        memset(packet_buffer + 3, 0x00, 3);

        // Set data (Bytes 6-9) in little-endian
        memcpy(packet_buffer + 6, &packet_data, 4);

        // Call the function at the calculated address
        SendMessageFunction(menu->con, 4, 4, packet_buffer, sizeof(packet_buffer), 0, 0);
        menu->Cleanup();

    };
}

void AoT_Spells_UI::minimizeClick()
{
    printf("minimized\n");
}


void printSystemMemoryStatus() {
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex)) {
        std::cout << "Available physical memory: " << statex.ullAvailPhys << " bytes\n";
        std::cout << "Total physical memory: " << statex.ullTotalPhys << " bytes\n";
    }
    else {
        std::cerr << "Failed to get system memory status.\n";
    }
}


std::vector<Entry> FindEntries(const std::string& filename, const std::vector<int>& targetIDs) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return {};
    }

    std::vector<Entry> results;
    std::string line;
    std::unordered_set<int> targets(targetIDs.begin(), targetIDs.end());

    while (std::getline(file, line)) {
        std::istringstream stream(line);
        std::string idStr, name, description;

        // Parse the line
        if (!std::getline(stream, idStr, '^') ||
            !std::getline(stream, name, '^') ||
            !std::getline(stream, description, '^')) {
            continue; // Skip malformed lines
        }

        // Convert ID and check if it's a target ID
        int id = std::stoi(idStr);
        if (targets.count(id)) {
            results.push_back({ id, name, description });
            targets.erase(id); // Remove from targets to avoid redundant checks
            if (targets.empty()) break; // Stop if all target IDs are found
        }
    }

    file.close();
    return results;
}


std::vector<int> ParseTargetIDs(const std::string& data) {
    std::vector<int> targetIDs;
    std::stringstream ss(data);
    std::string token;

    // Split the string by commas
    while (std::getline(ss, token, ',')) {
        try {
            // Convert each token to an integer
            targetIDs.push_back(std::stoi(token));
        }
        catch (const std::invalid_argument& e) {
            std::cerr << "Invalid number in data: " << token << std::endl;
        }
        catch (const std::out_of_range& e) {
            std::cerr << "Number out of range in data: " << token << std::endl;
        }
    }

    return targetIDs;
}


AoT_Spells_UI::AoT_Spells_UI() {}

AoT_Spells_UI::~AoT_Spells_UI() 
{
}

bool AoT_Spells_UI::InitializeWindow(IDirect3DDevice9* dev, std::string data) 
{



    // overhead
    device = dev;
    menuBuilder = new AoT_Menu_Builder();
    isInitialized = true;
    
    
    // spell processing

    std::vector<int> targetIDs = ParseTargetIDs(data);
    targetIDs.shrink_to_fit();
    std::string filename = "AoT_Spells.txt";

    std::vector<Entry> foundEntries = FindEntries(filename, targetIDs);
    foundEntries.shrink_to_fit();

    // create text texture
    spells = 0;
    for (const auto& entry : foundEntries) {
        std::cout << "ID: " << entry.id << ", Name: " << entry.name
            << ", Description: " << entry.description << std::endl;
        menuBuilder->RenderTextToD3D9Texture(device, ".\\PlayerStudio\\UI\\Resource\\Fonts\\Geo-MD.ttf",
            entry.name.c_str(), 20, D3DCOLOR_XRGB(34, 139, 34), 200, 40);
        
        menuBuilder->RenderTextToD3D9Texture(device, ".\\PlayerStudio\\UI\\Resource\\Fonts\\Geo-MD.ttf",
            entry.description.c_str(), 16, D3DCOLOR_XRGB(60, 65, 66), 200, 275);
        spells ++;
        
    }

    // load textures
    for (int i = 1; i <= 8; i++)
    {
        menuBuilder->LoadD3D9TextureFromFile(device, (".\\uifiles\\default\\spellUI\\parchment_textures\\parchment_texture_" + std::to_string(i) + ".png").c_str());

    }

    //printSystemMemoryStatus();
    //
    // Parchment Textures
    // ew triple for loop, I know. Gets the job done though.
    // To limit confusion: 
    // i is one spell column
    // j is one parchment pattern (in hindsight 100% should have just built 1 large parchment image)
    // k is a single parchment texture
    // 
    int x, y;
    for (int i = 0; i < spells; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            x = i * 256; 
            y = 128 * j + 8;

            for (int k = 0; k < 8; k++) 
            {
                menuBuilder->AddTexture(spells*2 + k, x + (k % 4) * 64, y + (k >= 4 ? 1 : 0) * 64);

            }
        }
    }
    
    //
    // Spell text textures
    //
    for (int i = 0; i < spells; i++)
    {
        menuBuilder->AddTexture(i * 2, 28 + i * 256, 15);
        menuBuilder->AddTexture(i * 2 + 1, 28 + i * 256, 60);
    }


    //
    // Topbar Textures
    // A lot more self explanatory, just 4 textures. 
    // hidden minimize button, 2 right bars (one for minimized mode, one for normal), 
    // and a middle section to move the window.
    //
    
    menuBuilder->LoadD3D9TextureFromFile(device, ".\\uifiles\\default\\spellUI\\menuRightTopbar.png");
    menuBuilder->AddTexture(2 * spells + 8, spells * 256 - 32, 0);
    //menuBuilder->AddTexture(2 * spells + 8, 32, 0);

    menuBuilder->LoadD3D9TextureFromFile(device, ".\\uifiles\\default\\spellUI\\menuLeftTopbar.png");
    menuBuilder->AddTexture(2 * spells + 9, 0, 0, AoT_Menu_Builder::EventType::Click, std::bind(&AoT_Spells_UI::minimizeClick, this));

    menuBuilder->LoadD3D9TextureFromFile(device, ".\\uifiles\\default\\spellUI\\menuMiddleTopbar.png");
    menuBuilder->AddTexture(2 * spells + 10, 32, 0, spells * 256 - 64, 16); // TODO: Drag

    
    //menuBuilder->ToggleTexture(spells * 24 + 3, false);
    
    //
    // Select buttons
    // 1 for each spell
    //
    menuBuilder->LoadD3D9TextureFromFile(device, ".\\uifiles\\default\\spellUI\\selectButton.png");
    for (int i = 0; i < spells; i++) // to use functions with parameters, use lambdas like so.
    {
        //std::function<void()> lambda = sendPacket(this, 1000 + i);
        std::function<void()> lambda = sendPacket(this, 1000 + i);
        menuBuilder->AddTexture(2 * spells + 11, 256 * i + 96, 216, AoT_Menu_Builder::EventType::Click, lambda);


    }

    menuBuilder->LoadD3D9TextureFromFile(device, ".\\uifiles\\default\\spellUI\\selectButton.png");
    
    open = true;

    menuBuilder -> SetMenuPosition(100, 100);
    config = 1;
    
    return true;
}


void AoT_Spells_UI::UpdateDevice(IDirect3DDevice9* dev)
{
    device = dev;
    //menuBuilder->UpdateDevice(dev);
}


void AoT_Spells_UI::Render(LPDIRECT3DDEVICE9 dev)
{
    if (!isInitialized || !open) return;
    //printf("render attempt.\n");
    //menuBuilder->Render(dev);
    menuBuilder->Render(dev);
    
}





void AoT_Spells_UI::Cleanup()
{
    printf("Cleaning up.\n");
    menuBuilder->CleanUp();
    printf("sanity2\n");
    menuBuilder = nullptr;
    printf("sanity\n");
    isInitialized = false;
    open = false;
}
