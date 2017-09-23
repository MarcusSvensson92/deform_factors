#pragma once

#include <Windows.h>
#include <CommCtrl.h>

#pragma comment(lib, "Comctl32.lib")

typedef void( *ButtonCallback )( void* );

enum SimpleComponentType
{
    COMPONENT_TYPE_LABEL,
    COMPONENT_TYPE_BUTTON,
    COMPONENT_TYPE_SLIDER,
    COMPONENT_TYPE_DROPDOWN,
    COMPONENT_TYPE_COUNT
};
struct SimpleLabelDesc
{
    const char*                 Text;
};
struct SimpleButtonDesc
{
    const char*                 Text;
    ButtonCallback              Callback;
    void*                       UserData;
};
struct SimpleSliderDesc
{
    const char*                 Text;
    unsigned int                ValueMin;
    unsigned int                ValueMax;
    unsigned int*               ValuePtr;
};
struct SimpleDropdownDesc
{
    const char*                 Text;
    unsigned int                ItemCount;
    const char**                ItemNames;
    unsigned int*               ItemIndexPtr;
};
struct SimpleComponentDesc
{
    SimpleComponentType         Type;
    union
    {
        SimpleLabelDesc         LabelDesc;
        SimpleButtonDesc        ButtonDesc;
        SimpleSliderDesc        SliderDesc;
        SimpleDropdownDesc      DropdownDesc;
    };

    static SimpleComponentDesc Label( const char* text )
    {
        SimpleComponentDesc component_desc;
        component_desc.Type = COMPONENT_TYPE_LABEL;
        component_desc.LabelDesc.Text = text;
        return component_desc;
    }
    static SimpleComponentDesc Button( const char* text, ButtonCallback callback, void* user_data )
    {
        SimpleComponentDesc component_desc;
        component_desc.Type = COMPONENT_TYPE_BUTTON;
        component_desc.ButtonDesc.Text = text;
        component_desc.ButtonDesc.Callback = callback;
        component_desc.ButtonDesc.UserData = user_data;
        return component_desc;
    }
    static SimpleComponentDesc Slider( const char* text, unsigned int value_min, unsigned int value_max, unsigned int* value_ptr )
    {
        SimpleComponentDesc component_desc;
        component_desc.Type = COMPONENT_TYPE_SLIDER;
        component_desc.SliderDesc.Text = text;
        component_desc.SliderDesc.ValueMin = value_min;
        component_desc.SliderDesc.ValueMax = value_max;
        component_desc.SliderDesc.ValuePtr = value_ptr;
        return component_desc;
    }
    static SimpleComponentDesc Dropdown( const char* text, unsigned int item_count, const char** item_names, unsigned int* item_index_ptr )
    {
        SimpleComponentDesc component_desc;
        component_desc.Type = COMPONENT_TYPE_DROPDOWN;
        component_desc.DropdownDesc.Text = text;
        component_desc.DropdownDesc.ItemCount = item_count;
        component_desc.DropdownDesc.ItemNames = item_names;
        component_desc.DropdownDesc.ItemIndexPtr = item_index_ptr;
        return component_desc;
    }
};
struct SimpleTweakbarSettings
{
    unsigned int                Width                   = 240;
    unsigned int                Margin                  = 4;

    unsigned int                ComponentHeight         = 26;
    unsigned int                ComponentDividerOffset  = ( Width - Margin * 2 ) / 3;
    unsigned int                ComponentDropdownHeight = 120;

    const char*                 FontName                = "Tahoma";
    unsigned int                FontSize                = 14;
};
struct SimpleTweakbarDesc
{
    HWND                        Parent;
    const char*                 Title;
    unsigned int                ComponentCount;
    SimpleComponentDesc*        ComponentDescs;
    SimpleTweakbarSettings      Settings;
};

class SimpleTweakbar
{
private:
    struct Button
    {
        ButtonCallback          Callback;
        void*                   UserData;
    };
    struct Slider
    {
        unsigned int*           ValuePtr;
    };
    struct Dropdown
    {
        unsigned int*           ItemIndexPtr;
    };

    HWND                        Window;
    Button*                     Buttons;
    Slider*                     Sliders;
    Dropdown*                   Dropdowns;

    friend LRESULT CALLBACK SimpleTweakbarWndProc( HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR );
    friend SimpleTweakbar* CreateSimpleTweakbar( const SimpleTweakbarDesc& );
    friend void DestroySimpleTweakbar( SimpleTweakbar* tweakbar );
};

LRESULT CALLBACK SimpleTweakbarWndProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id_subclass, DWORD_PTR ref_data )
{
    SimpleTweakbar* tweakbar = ( SimpleTweakbar* )ref_data;
    switch ( msg )
    {
        case WM_COMMAND:
        {
            switch ( HIWORD( wparam ) )
            {
                case BN_CLICKED:
                {
                    const SimpleTweakbar::Button& button = tweakbar->Buttons[ LOWORD( wparam ) ];
                    button.Callback( button.UserData );
                    break;
                }
                case CBN_SELCHANGE:
                {
                    const SimpleTweakbar::Dropdown& dropdown = tweakbar->Dropdowns[ LOWORD( wparam ) ];
                    *dropdown.ItemIndexPtr = static_cast< unsigned int >( SendMessage( reinterpret_cast< HWND >( lparam ), CB_GETCURSEL, NULL, NULL ) );
                    break;
                }
            }
            break;
        }
        case WM_HSCROLL:
        {
            switch ( LOWORD( wparam ) )
            {
                case TB_THUMBPOSITION:
                case TB_THUMBTRACK:
                {
                    const SimpleTweakbar::Slider& slider = tweakbar->Sliders[ GetWindowLongPtr( reinterpret_cast< HWND >( lparam ), GWLP_USERDATA ) ];
                    *slider.ValuePtr = static_cast< unsigned int >( HIWORD( wparam ) );
                    break;
                }
            }
            break;
        }
        case WM_ERASEBKGND:
        {
            RECT rect;
            GetClientRect( tweakbar->Window, &rect );
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint( tweakbar->Window, &ps );
            FillRect( hdc, &rect, GetSysColorBrush( COLOR_3DFACE ) );
            EndPaint( tweakbar->Window, &ps );
            break;
        }
        default:
            return DefSubclassProc( hwnd, msg, wparam, lparam );
    }
    return 0;
}

SimpleTweakbar* CreateSimpleTweakbar( const SimpleTweakbarDesc& tweakbar_desc )
{
    const SimpleTweakbarSettings& settings = tweakbar_desc.Settings;

    // Memory allocation
    size_t component_count[ COMPONENT_TYPE_COUNT ] = {};
    for ( unsigned int i = 0; i < tweakbar_desc.ComponentCount; ++i )
    {
        ++component_count[ tweakbar_desc.ComponentDescs[ i ].Type ];
    }
    size_t memory_size = sizeof( SimpleTweakbar );
    for ( unsigned int i = 0; i < COMPONENT_TYPE_COUNT; ++i )
    {
        size_t component_size = 0;
        switch ( i )
        {
            case COMPONENT_TYPE_BUTTON:
                component_size = sizeof( SimpleTweakbar::Button );
                break;
            case COMPONENT_TYPE_SLIDER:
                component_size = sizeof( SimpleTweakbar::Slider );
                break;
            case COMPONENT_TYPE_DROPDOWN:
                component_size = sizeof( SimpleTweakbar::Dropdown );
                break;
        }
        memory_size += component_count[ i ] * component_size;
    }
    char* memory = new char[ memory_size ];
    SimpleTweakbar* tweakbar = reinterpret_cast< SimpleTweakbar* >( memory );
    tweakbar->Buttons = reinterpret_cast< SimpleTweakbar::Button* >( tweakbar + 1 );
    tweakbar->Sliders = reinterpret_cast< SimpleTweakbar::Slider* >( tweakbar->Buttons + component_count[ COMPONENT_TYPE_BUTTON ] );
    tweakbar->Dropdowns = reinterpret_cast< SimpleTweakbar::Dropdown* >( tweakbar->Sliders + component_count[ COMPONENT_TYPE_SLIDER ] );

    // Create tweakbar window
    DWORD tweakbar_style = WS_CAPTION | WS_VISIBLE | WS_POPUP;
    int tweakbar_w = settings.Width;
    int tweakbar_h = ( settings.ComponentHeight + settings.Margin ) * tweakbar_desc.ComponentCount + settings.Margin;
    RECT tweakbar_rect = { 0, 0, tweakbar_w, tweakbar_h };
    AdjustWindowRect( &tweakbar_rect, tweakbar_style, FALSE );
    CLIENTCREATESTRUCT client_create_struct = {};
    tweakbar->Window = CreateWindow(
        "MDICLIENT",
        tweakbar_desc.Title,
        tweakbar_style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        tweakbar_rect.right - tweakbar_rect.left,
        tweakbar_rect.bottom - tweakbar_rect.top,
        tweakbar_desc.Parent,
        NULL,
        GetModuleHandle( NULL ),
        &client_create_struct );
    SetWindowSubclass( tweakbar->Window, SimpleTweakbarWndProc, 0, reinterpret_cast< DWORD_PTR >( tweakbar ) );

    // Create font
    HFONT font = CreateFont(
        settings.FontSize,
        0,
        0,
        0,
        FW_REGULAR,
        FALSE,
        FALSE,
        FALSE,
        ANSI_CHARSET,
        OUT_TT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        settings.FontName );

    // Create component windows
    memset( component_count, 0, sizeof( size_t ) * COMPONENT_TYPE_COUNT );
    for ( unsigned int i = 0; i < tweakbar_desc.ComponentCount; ++i )
    {
        const SimpleComponentDesc& component_desc = tweakbar_desc.ComponentDescs[ i ];
        const size_t component_index = component_count[ component_desc.Type ];

        switch ( component_desc.Type )
        {
            case COMPONENT_TYPE_LABEL:
            {
                DWORD text_style = WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE;
                int text_x = settings.Margin;
                int text_y = ( settings.ComponentHeight + settings.Margin ) * i + settings.Margin;
                int text_w = settings.Width - settings.Margin * 2;
                int text_h = settings.ComponentHeight;
                HWND text_window = CreateWindow(
                    "STATIC",
                    component_desc.LabelDesc.Text,
                    text_style,
                    text_x,
                    text_y,
                    text_w,
                    text_h,
                    tweakbar->Window,
                    NULL,
                    GetModuleHandle( NULL ),
                    NULL );
                SendMessage( text_window, WM_SETFONT, reinterpret_cast< WPARAM >( font ), TRUE );

                break;
            }
            case COMPONENT_TYPE_BUTTON:
            {
                DWORD button_style = WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON;
                int button_x = settings.Margin;
                int button_y = ( settings.ComponentHeight + settings.Margin ) * i + settings.Margin;
                int button_w = settings.Width - settings.Margin * 2;
                int button_h = settings.ComponentHeight;
                HWND button_window = CreateWindow(
                    "BUTTON",
                    component_desc.ButtonDesc.Text,
                    button_style,
                    button_x,
                    button_y,
                    button_w,
                    button_h,
                    tweakbar->Window,
                    reinterpret_cast< HMENU >( component_index ),
                    GetModuleHandle( NULL ),
                    NULL );
                SendMessage( button_window, WM_SETFONT, reinterpret_cast< WPARAM >( font ), TRUE );

                SimpleTweakbar::Button& button = tweakbar->Buttons[ component_index ];
                button.Callback = component_desc.ButtonDesc.Callback;
                button.UserData = component_desc.ButtonDesc.UserData;

                break;
            }
            case COMPONENT_TYPE_SLIDER:
            {
                DWORD text_style = WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE;
                int text_x = settings.Margin;
                int text_y = ( settings.ComponentHeight + settings.Margin ) * i + settings.Margin;
                int text_w = settings.ComponentDividerOffset;
                int text_h = settings.ComponentHeight;
                HWND text_window = CreateWindow(
                    "STATIC",
                    component_desc.SliderDesc.Text,
                    text_style,
                    text_x,
                    text_y,
                    text_w,
                    text_h,
                    tweakbar->Window,
                    NULL,
                    GetModuleHandle( NULL ),
                    NULL );
                SendMessage( text_window, WM_SETFONT, reinterpret_cast< WPARAM >( font ), TRUE );

                DWORD slider_style = WS_VISIBLE | WS_CHILD | TBS_TOOLTIPS;
                int slider_x = settings.Margin + settings.ComponentDividerOffset;
                int slider_y = ( settings.ComponentHeight + settings.Margin ) * i + settings.Margin;
                int slider_w = settings.Width - settings.Margin * 2 - settings.ComponentDividerOffset;
                int slider_h = settings.ComponentHeight;
                HWND slider_window = CreateWindow(
                    TRACKBAR_CLASS,
                    NULL,
                    slider_style,
                    slider_x,
                    slider_y,
                    slider_w,
                    slider_h,
                    tweakbar->Window,
                    NULL,
                    GetModuleHandle( NULL ),
                    NULL );
                SetWindowLongPtr( slider_window, GWLP_USERDATA, static_cast< LONG_PTR >( component_index ) );
                SendMessage( slider_window, WM_SETFONT, reinterpret_cast< WPARAM >( font ), TRUE );
                SendMessage( slider_window, TBM_SETRANGEMIN, TRUE, component_desc.SliderDesc.ValueMin );
                SendMessage( slider_window, TBM_SETRANGEMAX, TRUE, component_desc.SliderDesc.ValueMax );
                SendMessage( slider_window, TBM_SETPOS, TRUE, *component_desc.SliderDesc.ValuePtr );

                SimpleTweakbar::Slider& slider = tweakbar->Sliders[ component_index ];
                slider.ValuePtr = component_desc.SliderDesc.ValuePtr;

                break;
            }
            case COMPONENT_TYPE_DROPDOWN:
            {
                DWORD text_style = WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE;
                int text_x = settings.Margin;
                int text_y = ( settings.ComponentHeight + settings.Margin ) * i + settings.Margin;
                int text_w = settings.ComponentDividerOffset;
                int text_h = settings.ComponentHeight;
                HWND text_window = CreateWindow(
                    "STATIC",
                    component_desc.DropdownDesc.Text,
                    text_style,
                    text_x,
                    text_y,
                    text_w,
                    text_h,
                    tweakbar->Window,
                    NULL,
                    GetModuleHandle( NULL ),
                    NULL );
                SendMessage( text_window, WM_SETFONT, reinterpret_cast< WPARAM >( font ), TRUE );

                DWORD dropdown_style = WS_VISIBLE | WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_DISABLENOSCROLL;
                int dropdown_x = settings.Margin + settings.ComponentDividerOffset;
                int dropdown_y = ( settings.ComponentHeight + settings.Margin ) * i + settings.Margin;
                int dropdown_w = settings.Width - settings.Margin * 2 - settings.ComponentDividerOffset;
                int dropdown_h = settings.ComponentDropdownHeight;
                HWND dropdown_window = CreateWindow(
                    "COMBOBOX",
                    NULL,
                    dropdown_style,
                    dropdown_x,
                    dropdown_y,
                    dropdown_w,
                    dropdown_h,
                    tweakbar->Window,
                    reinterpret_cast< HMENU >( component_index ),
                    GetModuleHandle( NULL ),
                    NULL );
                for ( unsigned int j = 0; j < component_desc.DropdownDesc.ItemCount; ++j )
                {
                    SendMessage( dropdown_window, CB_ADDSTRING, 0, reinterpret_cast< LPARAM >( component_desc.DropdownDesc.ItemNames[ j ] ) );
                }
                SendMessage( dropdown_window, CB_SETCURSEL, static_cast< WPARAM >( *component_desc.DropdownDesc.ItemIndexPtr ), 0 );
                SendMessage( dropdown_window, WM_SETFONT, reinterpret_cast< WPARAM >( font ), TRUE );

                RECT dropdown_rect;
                GetClientRect( dropdown_window, &dropdown_rect );
                dropdown_y += ( settings.ComponentHeight - dropdown_rect.bottom ) / 2;
                SetWindowPos( dropdown_window, NULL, dropdown_x, dropdown_y, 0, 0, SWP_NOSIZE );

                SimpleTweakbar::Dropdown& dropdown = tweakbar->Dropdowns[ component_index ];
                dropdown.ItemIndexPtr = component_desc.DropdownDesc.ItemIndexPtr;

                break;
            }
        }

        ++component_count[ component_desc.Type ];
    }

    return tweakbar;
}

void DestroySimpleTweakbar( SimpleTweakbar* tweakbar )
{
    DestroyWindow( tweakbar->Window );

    delete[] tweakbar;
    tweakbar = NULL;
}