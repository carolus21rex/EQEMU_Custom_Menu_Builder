## EQEMU Menu Builder


![image](https://github.com/user-attachments/assets/2abdf202-82ab-43c5-8325-af8c32edee36)



# What is the goal?
The goal of this repo is to make a custom UI for eqemu inside dinput to be able to do various tasks.


# Features
Use any texture or font you would like for your menu.

Menu Builder supports drag and click mouse events. More may come soon.


# How to use it
1. Copy the required files into your dinput repository
2. (optional) Make images to use as textures for your custom UI
3. (optional) If you want a server packet to open your menu use HandleWorldMessage_Detour in eqgame.cpp like so:
```
unsigned char __fastcall HandleWorldMessage_Trampoline(DWORD *con, DWORD edx, unsigned __int32 unk, unsigned __int16 opcode, char* buf, size_t size);
unsigned char __fastcall HandleWorldMessage_Detour(DWORD *con, DWORD edx, unsigned __int32 unk, unsigned __int16 opcode, char* buf, size_t size)
{
	//printf("test2\n");

	if (opcode >= AoT_MIN_OPCODE && opcode <= AoT_MAX_OPCODE) {
		

		// Continue handling the original packet
		HandleOpcode(opcode, buf); 
	}

	return HandleWorldMessage_Trampoline(con, edx, unk, opcode, buf, size);
}

DETOUR_TRAMPOLINE_EMPTY(unsigned char __fastcall HandleWorldMessage_Trampoline(DWORD *con, DWORD edx, unsigned __int32 unk, unsigned __int16 opcode, char* buf, size_t size));

```
4. In your usage, initialize D3D9Hooks. It will launch Gui_Master which will in turn launch your menu. It is meant to support multiple types of custom menus but this functionality is not finished yet
5. In your menu (example is AoT_Spells_UI.cpp) initialize fonts and textures in `AoT_Menu_Builder* menuBuilder = nullptr` an example is `menuBuilder->RenderTextToD3D9Texture(device, ".\\PlayerStudio\\UI\\Resource\\Fonts\\Geo-MD.ttf"`
6. Add the texture and click type to menubuilder
```
menuBuilder->AddTexture(2 * spells + 8, spells * 256 - 32, 0);
    //menuBuilder->AddTexture(2 * spells + 8, 32, 0);

    menuBuilder->LoadD3D9TextureFromFile(device, ".\\uifiles\\default\\spellUI\\menuLeftTopbar.png");
    menuBuilder->AddTexture(2 * spells + 9, 0, 0, AoT_Menu_Builder::EventType::Click, std::bind(&AoT_Spells_UI::minimizeClick, this));
```
 A large mistake of how I made this is the indecies for graphical elements are convoluted. How it works is the elements are added to a vector as they are initialized. The intention is to minimize the amount of files that need to be pulled into memory. This almost certainly needs to be improved.
7. Any interactive portions of your menu will need their functionality made.
8. When youre done with your menu, call your menus cleanup. this will clean up the menu builder as well.
9. If there are no guis running call cleanup on guimaster as well to avoid wasting resources.

