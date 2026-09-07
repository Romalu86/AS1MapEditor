#include "mapedit/runtime.hpp"

// Original source: main_edit.cpp, derived from MapEdit.exe + debug metadata.
int __stdcall AppStart(HWND__* hwnd, unsigned int msg, unsigned int wParam, long)
{
    DIALOG_COMBO_BOX drivers(hwnd, 0x421);
    DIALOG_COMBO_BOX modes(hwnd, 0x422);
    DIALOG_BUTTON fullscreen(hwnd, 0x7D1);
    DIALOG_BUTTON soundHighQuality(hwnd, 0x7D7);

    if (msg == 0x110) { // WM_INITDIALOG
        soundHighQuality.SetCheck(Registry->GetInt(STRING("SoundHighQuality"), 0));
        Graph->UpdateStartupDialog(&drivers, &modes, &fullscreen);
        return 1;
    }

    if (msg == 0x111) { // WM_COMMAND
        const unsigned int id = wParam & 0xFFFFu;
        const unsigned int notification = (wParam >> 16) & 0xFFFFu;

        if (id == 1) { // IDOK
            Graph->UpdateStartupDialog(&drivers, &modes, &fullscreen);
            Registry->SetInt(STRING("SoundHighQuality"), static_cast<int>(soundHighQuality.GetCheck()));
            EndDialog(hwnd, 1);
        } else if (id == 2) { // IDCANCEL
            EndDialog(hwnd, 0);
        } else if ((id == 0x421 || id == 0x422) && notification == 9) {
            Graph->UpdateStartupDialog(&drivers, &modes, &fullscreen);
        }
    }
    return 0;
}

long __stdcall AppWndProc(HWND__* hwnd, unsigned int msg, unsigned int wParam, long lParam)
{
    if (Map->WorkWndMessage(hwnd, msg, wParam, static_cast<unsigned long>(lParam)))
        return 1;
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int __stdcall WinMain(HINSTANCE__* hInst, HINSTANCE__* hPrev, char* commandLine, int showCommand)
{
    STRING command(commandLine);
    Map = new MAP_EDIT(hInst, hPrev, &command, showCommand, &GraphInit);
    if (Map) {
        if (Map->IsInitSuccess()) {
            while (!Map->Tact()) {
            }
        }
        delete Map;
    }
    return 0;
}
