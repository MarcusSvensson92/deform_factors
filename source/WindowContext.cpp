#include "WindowContext.h"

#include <windowsx.h>
#include <assert.h>

LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    CWindowContext* wc = reinterpret_cast< CWindowContext* >( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
    switch ( msg )
    {
        case WM_CLOSE:
            PostQuitMessage( 0 );
            break;

        #define CASE_KEY_DOWN(CASE_LABEL, KEY_VALUE) \
            case CASE_LABEL: \
                assert( KEY_VALUE < 0xFF ); \
                if ( !wc->KeysDown[ KEY_VALUE ] ) \
                { \
                    wc->KeysDown[ KEY_VALUE ] = true; \
                    wc->KeysUp[ KEY_VALUE ] = false; \
                    wc->KeysPressed[ KEY_VALUE ] = true; \
                } \
                SetCapture( hwnd ); \
                break;
        #define CASE_KEY_UP(CASE_LABEL, KEY_VALUE) \
            case CASE_LABEL: \
                assert( KEY_VALUE < 0xFF ); \
                if ( !wc->KeysUp[ KEY_VALUE ] ) \
                { \
                    wc->KeysUp[ KEY_VALUE ] = true; \
                    wc->KeysDown[ KEY_VALUE ] = false; \
                    wc->KeysReleased[ KEY_VALUE ] = true; \
                } \
                ReleaseCapture(); \
                break;

        CASE_KEY_DOWN( WM_KEYDOWN, wparam )
        CASE_KEY_DOWN( WM_LBUTTONDOWN, MK_LBUTTON )
        CASE_KEY_DOWN( WM_MBUTTONDOWN, MK_MBUTTON )
        CASE_KEY_DOWN( WM_RBUTTONDOWN, MK_RBUTTON )

        CASE_KEY_UP( WM_KEYUP, wparam )
        CASE_KEY_UP( WM_LBUTTONUP, MK_LBUTTON )
        CASE_KEY_UP( WM_MBUTTONUP, MK_MBUTTON )
        CASE_KEY_UP( WM_RBUTTONUP, MK_RBUTTON )

        #undef CASE_KEY_DOWN
        #undef CASE_KEY_UP

        case WM_MOUSEMOVE:
            wc->DeltaMousePosition[ 0 ] = GET_X_LPARAM( lparam ) - wc->MousePosition[ 0 ];
            wc->DeltaMousePosition[ 1 ] = GET_Y_LPARAM( lparam ) - wc->MousePosition[ 1 ];
            wc->MousePosition[ 0 ] = GET_X_LPARAM( lparam );
            wc->MousePosition[ 1 ] = GET_Y_LPARAM( lparam );
            break;

        default:
            break;
    }
    return DefWindowProcA( hwnd, msg, wparam, lparam );
}

CWindowContext* CreateWindowContext()
{
    CWindowContext* wc = new CWindowContext();

    RECT rect;
    SetRect( &rect, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT );
    AdjustWindowRect( &rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE );

    WNDCLASSA wnd_class;
    wnd_class.style = 0;
    wnd_class.lpfnWndProc = WndProc;
    wnd_class.cbClsExtra = 0;
    wnd_class.cbWndExtra = 0;
    wnd_class.hInstance = GetModuleHandle( NULL );
    wnd_class.hIcon = LoadIcon( NULL, IDI_APPLICATION );
    wnd_class.hCursor = LoadCursor( NULL, IDC_ARROW );
    wnd_class.hbrBackground = NULL;
    wnd_class.lpszMenuName = NULL;
    wnd_class.lpszClassName = "WindowContext";
    RegisterClassA( &wnd_class );

    wc->Hwnd = CreateWindowA( "WindowContext", WINDOW_TITLE, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, NULL, NULL );
    assert( wc->Hwnd );

    SetWindowLongPtr( wc->Hwnd, GWLP_USERDATA, reinterpret_cast< LONG_PTR >( wc ) );

    ShowWindow( wc->Hwnd, SW_SHOWDEFAULT );
    UpdateWindow( wc->Hwnd );

    wc->KeysDown.reset();
    wc->KeysUp.set();
    wc->KeysPressed.reset();
    wc->KeysReleased.reset();
    wc->MousePosition[ 0 ] = 0;
    wc->MousePosition[ 1 ] = 0;
    wc->DeltaMousePosition[ 0 ] = 0;
    wc->DeltaMousePosition[ 1 ] = 0;

    return wc;
}

void DestroyWindowContext( CWindowContext* wc )
{
    DestroyWindow( wc->Hwnd );

    UnregisterClassA( "WindowContext", ( HINSTANCE )GetModuleHandle( NULL ) );

    delete wc;
    wc = nullptr;
}

void ResetInput( CWindowContext* wc )
{
    wc->KeysPressed.reset();
    wc->KeysReleased.reset();
    wc->DeltaMousePosition[ 0 ] = 0;
    wc->DeltaMousePosition[ 1 ] = 0;
}