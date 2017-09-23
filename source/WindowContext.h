#pragma once

#include <Windows.h>
#include <bitset>

static const UINT32 WINDOW_WIDTH  = 1366;
static const UINT32 WINDOW_HEIGHT = 768;
static const char*  WINDOW_TITLE  = "Deform Factors";

class CWindowContext
{
public:
    HWND                Hwnd;

    std::bitset<0xFF>   KeysDown;
    std::bitset<0xFF>   KeysUp;
    std::bitset<0xFF>   KeysPressed;
    std::bitset<0xFF>   KeysReleased;
    int                 MousePosition[ 2 ];
    int                 DeltaMousePosition[ 2 ];
};

CWindowContext* CreateWindowContext();
void DestroyWindowContext( CWindowContext* wc );

void ResetInput( CWindowContext* wc );