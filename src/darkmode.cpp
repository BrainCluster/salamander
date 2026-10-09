// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include "cfgdlg.h"
#include "mainwnd.h"
#include "editwnd.h"

// this module needs the real system colors
#undef GetSysColor
#undef GetSysColorBrush

BOOL DarkModeActive = FALSE;

// ****************************************************************************
//
// Dark palette for system colors (indexed by COLOR_xxx)
//

#define DARK_SYSCOLORS_COUNT 31

static const COLORREF DarkSysColors[DARK_SYSCOLORS_COUNT] =
    {
        RGB(60, 60, 60),    //  0 COLOR_SCROLLBAR
        RGB(30, 30, 30),    //  1 COLOR_BACKGROUND
        RGB(0, 84, 153),    //  2 COLOR_ACTIVECAPTION
        RGB(60, 60, 60),    //  3 COLOR_INACTIVECAPTION
        RGB(43, 43, 43),    //  4 COLOR_MENU
        RGB(30, 30, 30),    //  5 COLOR_WINDOW
        RGB(90, 90, 90),    //  6 COLOR_WINDOWFRAME
        RGB(230, 230, 230), //  7 COLOR_MENUTEXT
        RGB(220, 220, 220), //  8 COLOR_WINDOWTEXT
        RGB(255, 255, 255), //  9 COLOR_CAPTIONTEXT
        RGB(43, 43, 43),    // 10 COLOR_ACTIVEBORDER
        RGB(43, 43, 43),    // 11 COLOR_INACTIVEBORDER
        RGB(30, 30, 30),    // 12 COLOR_APPWORKSPACE
        RGB(0, 95, 170),    // 13 COLOR_HIGHLIGHT
        RGB(255, 255, 255), // 14 COLOR_HIGHLIGHTTEXT
        RGB(43, 43, 43),    // 15 COLOR_BTNFACE
        RGB(100, 100, 100), // 16 COLOR_BTNSHADOW (lighter than the face, otherwise gray menu text is invisible)
        RGB(128, 128, 128), // 17 COLOR_GRAYTEXT
        RGB(230, 230, 230), // 18 COLOR_BTNTEXT
        RGB(200, 200, 200), // 19 COLOR_INACTIVECAPTIONTEXT
        RGB(65, 65, 65),    // 20 COLOR_BTNHIGHLIGHT
        RGB(15, 15, 15),    // 21 COLOR_3DDKSHADOW
        RGB(55, 55, 55),    // 22 COLOR_3DLIGHT
        RGB(230, 230, 230), // 23 COLOR_INFOTEXT
        RGB(55, 55, 55),    // 24 COLOR_INFOBK
        RGB(43, 43, 43),    // 25 (unused)
        RGB(100, 170, 255), // 26 COLOR_HOTLIGHT
        RGB(0, 84, 153),    // 27 COLOR_GRADIENTACTIVECAPTION
        RGB(60, 60, 60),    // 28 COLOR_GRADIENTINACTIVECAPTION
        RGB(0, 95, 170),    // 29 COLOR_MENUHILIGHT
        RGB(43, 43, 43),    // 30 COLOR_MENUBAR
};

static HBRUSH DarkSysBrushes[DARK_SYSCOLORS_COUNT]; // created on demand, never released (like system brushes)

COLORREF SalGetSysColor(int index)
{
    if (DarkModeActive && index >= 0 && index < DARK_SYSCOLORS_COUNT && index != COLOR_BACKGROUND)
        return DarkSysColors[index];
    return GetSysColor(index);
}

HBRUSH SalGetSysColorBrush(int index)
{
    if (DarkModeActive && index >= 0 && index < DARK_SYSCOLORS_COUNT && index != COLOR_BACKGROUND)
    {
        if (DarkSysBrushes[index] == NULL)
            DarkSysBrushes[index] = CreateSolidBrush(DarkSysColors[index]);
        if (DarkSysBrushes[index] != NULL)
            return DarkSysBrushes[index];
    }
    return GetSysColorBrush(index);
}

// ****************************************************************************
//
// Dark panel color scheme
//

SALCOLOR DarkColors[NUMBER_OF_COLORS] =
    {
        // pen colors for the frame around the item
        RGBF(200, 200, 200, 0), // FOCUS_ACTIVE_NORMAL
        RGBF(200, 200, 200, 0), // FOCUS_ACTIVE_SELECTED
        RGBF(110, 110, 110, 0), // FOCUS_FG_INACTIVE_NORMAL
        RGBF(110, 110, 110, 0), // FOCUS_FG_INACTIVE_SELECTED
        RGBF(30, 30, 30, 0),    // FOCUS_BK_INACTIVE_NORMAL
        RGBF(30, 30, 30, 0),    // FOCUS_BK_INACTIVE_SELECTED

        // item text colors in the panel
        RGBF(220, 220, 220, 0), // ITEM_FG_NORMAL
        RGBF(255, 110, 110, 0), // ITEM_FG_SELECTED
        RGBF(240, 240, 240, 0), // ITEM_FG_FOCUSED
        RGBF(255, 110, 110, 0), // ITEM_FG_FOCSEL
        RGBF(220, 220, 220, 0), // ITEM_FG_HIGHLIGHT

        // background colors of items in the panel
        RGBF(30, 30, 30, 0), // ITEM_BK_NORMAL
        RGBF(30, 30, 30, 0), // ITEM_BK_SELECTED
        RGBF(62, 62, 66, 0), // ITEM_BK_FOCUSED
        RGBF(62, 62, 66, 0), // ITEM_BK_FOCSEL
        RGBF(45, 45, 48, 0), // ITEM_BK_HIGHLIGHT

        // icon blend colors
        RGBF(255, 110, 110, 0), // ICON_BLEND_SELECTED
        RGBF(128, 128, 128, 0), // ICON_BLEND_FOCUSED
        RGBF(255, 60, 60, 0),   // ICON_BLEND_FOCSEL

        // progress bar colors
        RGBF(220, 220, 220, 0), // PROGRESS_FG_NORMAL
        RGBF(255, 255, 255, 0), // PROGRESS_FG_SELECTED
        RGBF(30, 30, 30, 0),    // PROGRESS_BK_NORMAL
        RGBF(0, 95, 170, 0),    // PROGRESS_BK_SELECTED

        // colors of hot items
        RGBF(100, 170, 255, 0), // HOT_PANEL
        RGBF(150, 190, 240, 0), // HOT_ACTIVE
        RGBF(170, 170, 170, 0), // HOT_INACTIVE

        // panel title colors
        RGBF(255, 255, 255, 0), // ACTIVE_CAPTION_FG
        RGBF(0, 84, 153, 0),    // ACTIVE_CAPTION_BK
        RGBF(200, 200, 200, 0), // INACTIVE_CAPTION_FG
        RGBF(60, 60, 60, 0),    // INACTIVE_CAPTION_BK

        // pen colors for the frame around thumbnails
        RGBF(80, 80, 80, 0),    // THUMBNAIL_FRAME_NORMAL
        RGBF(200, 200, 200, 0), // THUMBNAIL_FRAME_FOCUSED
        RGBF(255, 110, 110, 0), // THUMBNAIL_FRAME_SELECTED
        RGBF(200, 60, 60, 0),   // THUMBNAIL_FRAME_FOCSEL
};

// panel color scheme that was active before dark mode switched to the Dark scheme
static SALCOLOR* SchemeBeforeDarkMode = NULL;

SALCOLOR* GetColorSchemeByIndex(int index)
{
    switch (index)
    {
    case 0:
        return SalamanderColors;
    case 1:
        return ExplorerColors;
    case 2:
        return NortonColors;
    case 3:
        return NavigatorColors;
    case 4:
        return DarkColors;
    }
    return NULL; // COLOR_SCHEME_CUSTOM_INDEX
}

int GetColorSchemeIndex(SALCOLOR* colors)
{
    for (int i = 0; i < COLOR_SCHEME_CUSTOM_INDEX; i++)
    {
        if (GetColorSchemeByIndex(i) == colors)
            return i;
    }
    return COLOR_SCHEME_CUSTOM_INDEX;
}

// ****************************************************************************
//
// Windows API for dark mode (loaded dynamically, partly undocumented)
//

enum DarkPreferredAppMode
{
    dpamDefault = 0,
    dpamAllowDark = 1,
    dpamForceDark = 2,
    dpamForceLight = 3,
};

typedef HRESULT(WINAPI* FSetWindowTheme)(HWND, LPCWSTR, LPCWSTR);
typedef BOOL(WINAPI* FAllowDarkModeForWindow)(HWND, BOOL);     // uxtheme ordinal 133
typedef int(WINAPI* FSetPreferredAppMode)(int);                // uxtheme ordinal 135 (Windows 10 1903+)
typedef void(WINAPI* FFlushMenuThemes)();                      // uxtheme ordinal 136
typedef void(WINAPI* FRefreshImmersiveColorPolicyState)();     // uxtheme ordinal 104
typedef HRESULT(WINAPI* FDwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);

static BOOL DarkApiLoaded = FALSE;
static HMODULE UxThemeDLL = NULL;
static HMODULE DwmApiDLL = NULL;
static FSetWindowTheme DynSetWindowTheme = NULL;
static FAllowDarkModeForWindow DynAllowDarkModeForWindow = NULL;
static FSetPreferredAppMode DynSetPreferredAppMode = NULL;
static FFlushMenuThemes DynFlushMenuThemes = NULL;
static FRefreshImmersiveColorPolicyState DynRefreshImmersiveColorPolicyState = NULL;
static FDwmSetWindowAttribute DynDwmSetWindowAttribute = NULL;

static void DarkMode_LoadApi()
{
    if (DarkApiLoaded)
        return;
    DarkApiLoaded = TRUE;

    UxThemeDLL = LoadLibraryEx("uxtheme.dll", NULL, 0x00000800 /* LOAD_LIBRARY_SEARCH_SYSTEM32 */);
    if (UxThemeDLL != NULL)
    {
        DynSetWindowTheme = (FSetWindowTheme)GetProcAddress(UxThemeDLL, "SetWindowTheme");
        // the undocumented functions are available since Windows 10 1903 (build 18362)
        OSVERSIONINFOEXW osvi = {sizeof(osvi)};
        typedef LONG(WINAPI * FRtlGetVersion)(OSVERSIONINFOEXW*);
        FRtlGetVersion rtlGetVersion = (FRtlGetVersion)GetProcAddress(GetModuleHandle("ntdll.dll"), "RtlGetVersion");
        if (rtlGetVersion != NULL && rtlGetVersion(&osvi) == 0 &&
            osvi.dwMajorVersion >= 10 && osvi.dwBuildNumber >= 18362)
        {
            DynAllowDarkModeForWindow = (FAllowDarkModeForWindow)GetProcAddress(UxThemeDLL, MAKEINTRESOURCEA(133));
            DynSetPreferredAppMode = (FSetPreferredAppMode)GetProcAddress(UxThemeDLL, MAKEINTRESOURCEA(135));
            DynFlushMenuThemes = (FFlushMenuThemes)GetProcAddress(UxThemeDLL, MAKEINTRESOURCEA(136));
            DynRefreshImmersiveColorPolicyState = (FRefreshImmersiveColorPolicyState)GetProcAddress(UxThemeDLL, MAKEINTRESOURCEA(104));
        }
    }
    DwmApiDLL = LoadLibraryEx("dwmapi.dll", NULL, 0x00000800 /* LOAD_LIBRARY_SEARCH_SYSTEM32 */);
    if (DwmApiDLL != NULL)
        DynDwmSetWindowAttribute = (FDwmSetWindowAttribute)GetProcAddress(DwmApiDLL, "DwmSetWindowAttribute");
}

static void DarkMode_SetTheme(HWND hWnd, LPCWSTR subAppName)
{
    if (DynAllowDarkModeForWindow != NULL)
        DynAllowDarkModeForWindow(hWnd, DarkModeActive);
    if (DynSetWindowTheme != NULL)
        DynSetWindowTheme(hWnd, subAppName, NULL);
}

static void DarkMode_SetTitleBar(HWND hWnd)
{
    if (DynDwmSetWindowAttribute == NULL)
        return;
    BOOL dark = DarkModeActive;
    // DWMWA_USE_IMMERSIVE_DARK_MODE = 20 (Windows 10 20H1+), 19 on older builds
    if (FAILED(DynDwmSetWindowAttribute(hWnd, 20, &dark, sizeof(dark))))
        DynDwmSetWindowAttribute(hWnd, 19, &dark, sizeof(dark));
}

// ****************************************************************************
//
// Theming of standard controls
//

// list views: the text of the header control (column titles) has to be colored via custom draw,
// whose notifications are sent to the list view (the parent of the header)
#define DARKMODE_LV_SUBCLASS_ID 0x5A1DA4D

static LRESULT CALLBACK DarkMode_ListViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                                      UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_NOTIFY:
    {
        NMHDR* nmh = (NMHDR*)lParam;
        if (DarkModeActive && nmh->code == NM_CUSTOMDRAW && nmh->hwndFrom == ListView_GetHeader(hWnd))
        {
            NMCUSTOMDRAW* cd = (NMCUSTOMDRAW*)lParam;
            if (cd->dwDrawStage == CDDS_PREPAINT)
                return CDRF_NOTIFYITEMDRAW;
            if (cd->dwDrawStage == CDDS_ITEMPREPAINT)
            {
                SetTextColor(cd->hdc, SalGetSysColor(COLOR_BTNTEXT));
                return CDRF_DODEFAULT;
            }
        }
        break;
    }

    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, DarkMode_ListViewSubclassProc, uIdSubclass);
        break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// ****************************************************************************
//
// Menu bars of standard windows (internal viewer, plugin windows, ...)
//
// Windows draws menu bars always light. The menu bar is painted by the undocumented
// WM_UAHDRAWMENU (background) and WM_UAHDRAWMENUITEM (items) messages, which we handle
// ourselves in dark mode (the same technique is used e.g. by Notepad++).
//

#define WM_UAHDRAWMENU 0x0091
#define WM_UAHDRAWMENUITEM 0x0092

typedef union tagUAHMENUITEMMETRICS
{
    struct
    {
        DWORD cx;
        DWORD cy;
    } rgsizeBar[2];
    struct
    {
        DWORD cx;
        DWORD cy;
    } rgsizePopup[4];
} UAHMENUITEMMETRICS;

typedef struct tagUAHMENUPOPUPMETRICS
{
    DWORD rgcx[4];
    DWORD fUpdateMaxWidths : 2;
} UAHMENUPOPUPMETRICS;

typedef struct tagUAHMENU
{
    HMENU hmenu;
    HDC hdc;
    DWORD dwFlags;
} UAHMENU;

typedef struct tagUAHMENUITEM
{
    int iPosition;
    UAHMENUITEMMETRICS umim;
    UAHMENUPOPUPMETRICS umpm;
} UAHMENUITEM;

typedef struct tagUAHDRAWMENUITEM
{
    DRAWITEMSTRUCT dis;
    UAHMENU um;
    UAHMENUITEM umi;
} UAHDRAWMENUITEM;

#define DARKMODE_MENUBAR_SUBCLASS_ID 0x5A1DA4E

#define DARK_MENUBAR_HOT RGB(62, 62, 66)
#define DARK_MENUBAR_PUSHED RGB(80, 80, 85)

// the 1px light line Windows draws between the menu bar and the client area
static void DarkMode_PaintMenuBarBottomLine(HWND hWnd)
{
    MENUBARINFO mbi = {sizeof(mbi)};
    if (!GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi))
        return;
    RECT rcClient;
    GetClientRect(hWnd, &rcClient);
    MapWindowPoints(hWnd, NULL, (POINT*)&rcClient, 2);
    RECT rcWindow;
    GetWindowRect(hWnd, &rcWindow);
    OffsetRect(&rcClient, -rcWindow.left, -rcWindow.top);
    RECT rcLine = rcClient;
    rcLine.bottom = rcLine.top;
    rcLine.top--;
    HDC hDC = GetWindowDC(hWnd);
    if (hDC != NULL)
    {
        FillRect(hDC, &rcLine, SalGetSysColorBrush(COLOR_MENUBAR));
        ReleaseDC(hWnd, hDC);
    }
}

static LRESULT CALLBACK DarkMode_MenuBarSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                                     UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    if (DarkModeActive)
    {
        switch (uMsg)
        {
        case WM_UAHDRAWMENU:
        {
            UAHMENU* um = (UAHMENU*)lParam;
            MENUBARINFO mbi = {sizeof(mbi)};
            if (GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi))
            {
                RECT rcWindow;
                GetWindowRect(hWnd, &rcWindow);
                RECT rc = mbi.rcBar;
                OffsetRect(&rc, -rcWindow.left, -rcWindow.top);
                rc.top -= 1;
                FillRect(um->hdc, &rc, SalGetSysColorBrush(COLOR_MENUBAR));
            }
            return TRUE;
        }

        case WM_UAHDRAWMENUITEM:
        {
            UAHDRAWMENUITEM* udmi = (UAHDRAWMENUITEM*)lParam;
            wchar_t text[256] = L"";
            MENUITEMINFOW mii = {sizeof(mii)};
            mii.fMask = MIIM_STRING;
            mii.dwTypeData = text;
            mii.cch = _countof(text) - 1;
            GetMenuItemInfoW(udmi->um.hmenu, udmi->umi.iPosition, TRUE, &mii);

            UINT state = udmi->dis.itemState;
            COLORREF bk = SalGetSysColor(COLOR_MENUBAR);
            COLORREF fg = SalGetSysColor(COLOR_MENUTEXT);
            if (state & (ODS_INACTIVE | ODS_GRAYED | ODS_DISABLED))
                fg = SalGetSysColor(COLOR_GRAYTEXT);
            if (state & ODS_SELECTED)
                bk = DARK_MENUBAR_PUSHED;
            else if (state & ODS_HOTLIGHT)
                bk = DARK_MENUBAR_HOT;

            HBRUSH brush = CreateSolidBrush(bk);
            FillRect(udmi->um.hdc, &udmi->dis.rcItem, brush);
            DeleteObject(brush);

            DWORD dtFlags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;
            if (state & ODS_NOACCEL)
                dtFlags |= DT_HIDEPREFIX;
            int oldMode = SetBkMode(udmi->um.hdc, TRANSPARENT);
            COLORREF oldColor = SetTextColor(udmi->um.hdc, fg);
            DrawTextW(udmi->um.hdc, text, -1, &udmi->dis.rcItem, dtFlags);
            SetTextColor(udmi->um.hdc, oldColor);
            SetBkMode(udmi->um.hdc, oldMode);
            return TRUE;
        }

        case WM_NCPAINT:
        case WM_NCACTIVATE:
        {
            LRESULT res = DefSubclassProc(hWnd, uMsg, wParam, lParam);
            DarkMode_PaintMenuBarBottomLine(hWnd);
            return res;
        }
        }
    }
    if (uMsg == WM_NCDESTROY)
        RemoveWindowSubclass(hWnd, DarkMode_MenuBarSubclassProc, uIdSubclass);
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// ****************************************************************************
//
// Status bars: the themed status bar is always light, in dark mode we paint it ourselves
// (owner drawn parts are left to the owner)
//

#define DARKMODE_STATUSBAR_SUBCLASS_ID 0x5A1DA4F

static void DarkMode_PaintStatusBar(HWND hWnd, HDC hDC)
{
    RECT rcClient;
    GetClientRect(hWnd, &rcClient);
    FillRect(hDC, &rcClient, SalGetSysColorBrush(COLOR_BTNFACE));

    HFONT font = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
    HFONT oldFont = font != NULL ? (HFONT)SelectObject(hDC, font) : NULL;
    int oldMode = SetBkMode(hDC, TRANSPARENT);
    COLORREF oldColor = SetTextColor(hDC, SalGetSysColor(COLOR_BTNTEXT));

    HPEN pen = CreatePen(PS_SOLID, 1, SalGetSysColor(COLOR_BTNSHADOW));
    HPEN oldPen = (HPEN)SelectObject(hDC, pen);

    BOOL simple = (BOOL)SendMessage(hWnd, SB_ISSIMPLE, 0, 0);
    int parts = simple ? 1 : (int)SendMessage(hWnd, SB_GETPARTS, 0, 0);
    for (int i = 0; i < parts; i++)
    {
        int index = simple ? SB_SIMPLEID : i;
        RECT rc;
        if (!SendMessage(hWnd, SB_GETRECT, simple ? 0 : i, (LPARAM)&rc))
            continue;
        DWORD lenAndType = (DWORD)SendMessage(hWnd, SB_GETTEXTLENGTHW, index, 0);
        if (HIWORD(lenAndType) & SBT_OWNERDRAW)
        {
            // owner drawn part: let the owner draw it
            DRAWITEMSTRUCT dis = {0};
            dis.CtlType = ODT_STATIC;
            dis.CtlID = GetDlgCtrlID(hWnd);
            dis.itemID = i;
            dis.hwndItem = hWnd;
            dis.hDC = hDC;
            dis.rcItem = rc;
            dis.itemData = (ULONG_PTR)SendMessage(hWnd, SB_GETTEXTW, index, 0);
            SendMessage(GetParent(hWnd), WM_DRAWITEM, dis.CtlID, (LPARAM)&dis);
            continue;
        }
        if (i > 0 && !simple) // separator between parts
        {
            MoveToEx(hDC, rc.left - 1, rc.top + 2, NULL);
            LineTo(hDC, rc.left - 1, rc.bottom - 2);
        }
        int len = LOWORD(lenAndType);
        if (len > 0)
        {
            wchar_t* text = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
            if (text != NULL)
            {
                SendMessage(hWnd, SB_GETTEXTW, index, (LPARAM)text);
                RECT rcText = rc;
                rcText.left += 4;
                rcText.right -= 2;
                DrawTextW(hDC, text, -1, &rcText, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX | DT_END_ELLIPSIS);
                free(text);
            }
        }
    }

    SelectObject(hDC, oldPen);
    DeleteObject(pen);
    SetTextColor(hDC, oldColor);
    SetBkMode(hDC, oldMode);
    if (oldFont != NULL)
        SelectObject(hDC, oldFont);
}

static LRESULT CALLBACK DarkMode_StatusBarSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                                       UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    if (DarkModeActive)
    {
        switch (uMsg)
        {
        case WM_ERASEBKGND:
            return TRUE;

        case WM_PAINT:
        case WM_PRINTCLIENT:
        {
            PAINTSTRUCT ps;
            HDC hDC = uMsg == WM_PAINT ? BeginPaint(hWnd, &ps) : (HDC)wParam;
            DarkMode_PaintStatusBar(hWnd, hDC);
            if (uMsg == WM_PAINT)
                EndPaint(hWnd, &ps);
            return 0;
        }
        }
    }
    if (uMsg == WM_NCDESTROY)
        RemoveWindowSubclass(hWnd, DarkMode_StatusBarSubclassProc, uIdSubclass);
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// window classes registered with a system color brush, e.g. (HBRUSH)(COLOR_WINDOW + 1),
// get the matching dark brush (the change affects all windows of the class)
static void DarkMode_ApplyClassBackground(HWND hWnd)
{
    ULONG_PTR brush = GetClassLongPtr(hWnd, GCLP_HBRBACKGROUND);
    if (brush == 0)
        return;
    int index = -1;
    if (brush >= 1 && brush <= DARK_SYSCOLORS_COUNT) // (HBRUSH)(COLOR_xxx + 1)
        index = (int)brush - 1;
    else
    {
        for (int i = 0; i < DARK_SYSCOLORS_COUNT; i++)
        {
            if (DarkSysBrushes[i] != NULL && (ULONG_PTR)DarkSysBrushes[i] == brush)
            {
                index = i;
                break;
            }
        }
    }
    if (index < 0 || index == COLOR_BACKGROUND)
        return;
    ULONG_PTR newBrush = DarkModeActive ? (ULONG_PTR)SalGetSysColorBrush(index) : (ULONG_PTR)(index + 1);
    if (newBrush != brush)
        SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, (LONG_PTR)newBrush);
}

static void DarkMode_ApplyToControl(HWND hWnd)
{
    DarkMode_ApplyClassBackground(hWnd);

    char className[64];
    if (GetClassName(hWnd, className, _countof(className)) == 0)
        return;

    BOOL dark = DarkModeActive;
    if (StrICmp(className, "Button") == 0)
    {
        DWORD type = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE) & BS_TYPEMASK;
        if (type == BS_PUSHBUTTON || type == BS_DEFPUSHBUTTON || type == BS_SPLITBUTTON || type == BS_DEFSPLITBUTTON)
            DarkMode_SetTheme(hWnd, dark ? L"DarkMode_Explorer" : NULL);
        else if (type != BS_OWNERDRAW)
        {
            // themed check boxes, radio buttons and group boxes ignore WM_CTLCOLORxxx text colors,
            // so we use the classic look in dark mode (the text color is then set in WM_CTLCOLORSTATIC)
            if (DynSetWindowTheme != NULL)
                DynSetWindowTheme(hWnd, dark ? L"" : NULL, dark ? L"" : NULL);
        }
    }
    else if (StrICmp(className, "Edit") == 0 || StrICmp(className, "ComboBox") == 0)
        DarkMode_SetTheme(hWnd, dark ? L"DarkMode_CFD" : NULL);
    else if (StrICmp(className, "ScrollBar") == 0 || StrICmp(className, "ListBox") == 0)
        DarkMode_SetTheme(hWnd, dark ? L"DarkMode_Explorer" : NULL);
    else if (StrICmp(className, "SysHeader32") == 0)
        DarkMode_SetTheme(hWnd, dark ? L"DarkMode_ItemsView" : NULL);
    else if (StrICmp(className, "SysListView32") == 0)
    {
        DarkMode_SetTheme(hWnd, dark ? L"DarkMode_Explorer" : NULL);
        SetWindowSubclass(hWnd, DarkMode_ListViewSubclassProc, DARKMODE_LV_SUBCLASS_ID, 0);
        ListView_SetBkColor(hWnd, SalGetSysColor(COLOR_WINDOW));
        ListView_SetTextBkColor(hWnd, SalGetSysColor(COLOR_WINDOW));
        ListView_SetTextColor(hWnd, SalGetSysColor(COLOR_WINDOWTEXT));
    }
    else if (StrICmp(className, "SysTreeView32") == 0)
    {
        DarkMode_SetTheme(hWnd, dark ? L"DarkMode_Explorer" : NULL);
        TreeView_SetBkColor(hWnd, SalGetSysColor(COLOR_WINDOW));
        TreeView_SetTextColor(hWnd, SalGetSysColor(COLOR_WINDOWTEXT));
    }
    else if (StrICmp(className, "tooltips_class32") == 0)
        DarkMode_SetTheme(hWnd, dark ? L"DarkMode_Explorer" : NULL);
    else if (StrICmp(className, "msctls_statusbar32") == 0)
    {
        SetWindowSubclass(hWnd, DarkMode_StatusBarSubclassProc, DARKMODE_STATUSBAR_SUBCLASS_ID, 0);
        InvalidateRect(hWnd, NULL, TRUE);
    }
}

// window property: 1 = light look applied, 2 = dark look applied
#define DARKMODE_STATE_PROP "SalamanderDarkModeState"

static BOOL CALLBACK DarkMode_EnumChildProc(HWND hWnd, LPARAM lParam)
{
    DarkMode_ApplyToControl(hWnd);
    return TRUE;
}

void DarkMode_ApplyToWindow(HWND hWnd)
{
    if (hWnd == NULL || !IsWindow(hWnd))
        return;
    DarkMode_LoadApi();

    if (DynAllowDarkModeForWindow != NULL)
        DynAllowDarkModeForWindow(hWnd, DarkModeActive);
    if ((GetWindowLongPtr(hWnd, GWL_STYLE) & WS_CHILD) == 0)
    {
        DarkMode_SetTitleBar(hWnd);
        // non-client scroll bars of top-level windows (e.g. viewers); dialogs are not themed
        // as a whole, their controls are themed one by one
        char className[32];
        if (GetClassName(hWnd, className, _countof(className)) != 0 && strcmp(className, "#32770") != 0)
            DarkMode_SetTheme(hWnd, DarkModeActive ? L"DarkMode_Explorer" : NULL);
        // standard menu bar (the subclass must be installed by the thread owning the window)
        if (GetMenu(hWnd) != NULL && GetWindowThreadProcessId(hWnd, NULL) == GetCurrentThreadId())
        {
            SetWindowSubclass(hWnd, DarkMode_MenuBarSubclassProc, DARKMODE_MENUBAR_SUBCLASS_ID, 0);
            DrawMenuBar(hWnd);
        }
    }
    SetProp(hWnd, DARKMODE_STATE_PROP, (HANDLE)(INT_PTR)(DarkModeActive ? 2 : 1));
    DarkMode_ApplyToControl(hWnd);
    EnumChildWindows(hWnd, DarkMode_EnumChildProc, 0);
    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
}

// ****************************************************************************
//
// WM_CTLCOLORxxx handling: dialogs and windows hosting standard controls
//

#define DARKMODE_SUBCLASS_ID 0x5A1DA4C

static BOOL IsSystemDefaultBrush(HBRUSH brush)
{
    return brush == NULL ||
           brush == GetSysColorBrush(COLOR_BTNFACE) ||
           brush == GetSysColorBrush(COLOR_WINDOW) ||
           brush == GetSysColorBrush(COLOR_3DFACE);
}

static LRESULT CALLBACK DarkMode_SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                              UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    {
        LRESULT res = DefSubclassProc(hWnd, uMsg, wParam, lParam);
        // override only the default colors, keep colors chosen by the window itself
        if (DarkModeActive && IsSystemDefaultBrush((HBRUSH)res))
        {
            HDC hDC = (HDC)wParam;
            BOOL field = (uMsg == WM_CTLCOLOREDIT || uMsg == WM_CTLCOLORLISTBOX);
            HWND hCtrl = (HWND)lParam;
            BOOL disabled = (uMsg != WM_CTLCOLORDLG && hCtrl != NULL && !IsWindowEnabled(hCtrl));
            SetTextColor(hDC, SalGetSysColor(disabled ? COLOR_GRAYTEXT : (field ? COLOR_WINDOWTEXT : COLOR_BTNTEXT)));
            SetBkColor(hDC, SalGetSysColor(field ? COLOR_WINDOW : COLOR_BTNFACE));
            return (LRESULT)SalGetSysColorBrush(field ? COLOR_WINDOW : COLOR_BTNFACE);
        }
        return res;
    }

    case WM_INITDIALOG:
    {
        LRESULT res = DefSubclassProc(hWnd, uMsg, wParam, lParam);
        if (DarkModeActive)
            DarkMode_ApplyToWindow(hWnd);
        return res;
    }

    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, DarkMode_SubclassProc, uIdSubclass);
        break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static void DarkMode_Subclass(HWND hWnd)
{
    if (hWnd != NULL)
        SetWindowSubclass(hWnd, DarkMode_SubclassProc, DARKMODE_SUBCLASS_ID, 0);
}

// ****************************************************************************
//
// CBT hooks: dark look for newly created dialogs, scroll bars and top-level windows
//
// The hook is installed per thread: in the main thread by DarkMode_Apply() and in other
// threads (viewers, Find, progress dialogs, plugin windows) when the thread creates its first
// window or dialog (see the hooked CreateWindowEx & co. below).
//

static __declspec(thread) HHOOK ThreadCBTHook = NULL;

static LRESULT CALLBACK DarkMode_CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (DarkModeActive)
    {
        if (nCode == HCBT_CREATEWND)
        {
            HWND hWnd = (HWND)wParam;
            char className[32];
            if (GetClassName(hWnd, className, _countof(className)) != 0)
            {
                if (strcmp(className, "#32770") == 0) // dialog (the controls are themed in WM_INITDIALOG)
                    DarkMode_Subclass(hWnd);
                else if (StrICmp(className, "ScrollBar") == 0)
                    DarkMode_SetTheme(hWnd, L"DarkMode_Explorer");
            }
        }
        else if (nCode == HCBT_ACTIVATE)
        {
            // top-level windows that are not dialogs (dialogs are handled in WM_INITDIALOG)
            HWND hWnd = (HWND)wParam;
            if ((INT_PTR)GetProp(hWnd, DARKMODE_STATE_PROP) != 2)
                DarkMode_ApplyToWindow(hWnd);
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

static void DarkMode_EnsureThreadHook()
{
    if (DarkModeActive && ThreadCBTHook == NULL)
        ThreadCBTHook = SetWindowsHookEx(WH_CBT, DarkMode_CBTProc, NULL, GetCurrentThreadId());
}

// ****************************************************************************
//
// Import table hooks
//
// Plugins (and windows of other threads) do not know about dark mode. We patch the import
// tables of the plugin DLLs (and of salamand.exe for the window creation functions):
//   - GetSysColor / GetSysColorBrush -> SalGetSysColor / SalGetSysColorBrush (custom painted UI)
//   - window and dialog creation functions -> install the CBT hook in the calling thread
//

typedef HWND(WINAPI* FCreateWindowExA)(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
typedef HWND(WINAPI* FCreateWindowExW)(DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
typedef HWND(WINAPI* FCreateDialogParamA)(HINSTANCE, LPCSTR, HWND, DLGPROC, LPARAM);
typedef HWND(WINAPI* FCreateDialogParamW)(HINSTANCE, LPCWSTR, HWND, DLGPROC, LPARAM);
typedef HWND(WINAPI* FCreateDialogIndirectParamA)(HINSTANCE, LPCDLGTEMPLATEA, HWND, DLGPROC, LPARAM);
typedef HWND(WINAPI* FCreateDialogIndirectParamW)(HINSTANCE, LPCDLGTEMPLATEW, HWND, DLGPROC, LPARAM);
typedef INT_PTR(WINAPI* FDialogBoxParamA)(HINSTANCE, LPCSTR, HWND, DLGPROC, LPARAM);
typedef INT_PTR(WINAPI* FDialogBoxParamW)(HINSTANCE, LPCWSTR, HWND, DLGPROC, LPARAM);
typedef INT_PTR(WINAPI* FDialogBoxIndirectParamA)(HINSTANCE, LPCDLGTEMPLATEA, HWND, DLGPROC, LPARAM);
typedef INT_PTR(WINAPI* FDialogBoxIndirectParamW)(HINSTANCE, LPCDLGTEMPLATEW, HWND, DLGPROC, LPARAM);
typedef int(WINAPI* FMessageBoxA)(HWND, LPCSTR, LPCSTR, UINT);
typedef int(WINAPI* FMessageBoxW)(HWND, LPCWSTR, LPCWSTR, UINT);

static FCreateWindowExA RealCreateWindowExA = NULL;
static FCreateWindowExW RealCreateWindowExW = NULL;
static FCreateDialogParamA RealCreateDialogParamA = NULL;
static FCreateDialogParamW RealCreateDialogParamW = NULL;
static FCreateDialogIndirectParamA RealCreateDialogIndirectParamA = NULL;
static FCreateDialogIndirectParamW RealCreateDialogIndirectParamW = NULL;
static FDialogBoxParamA RealDialogBoxParamA = NULL;
static FDialogBoxParamW RealDialogBoxParamW = NULL;
static FDialogBoxIndirectParamA RealDialogBoxIndirectParamA = NULL;
static FDialogBoxIndirectParamW RealDialogBoxIndirectParamW = NULL;
static FMessageBoxA RealMessageBoxA = NULL;
static FMessageBoxW RealMessageBoxW = NULL;
static void* RealGetSysColor = NULL;
static void* RealGetSysColorBrush = NULL;

static HWND WINAPI Hook_CreateWindowExA(DWORD exStyle, LPCSTR className, LPCSTR windowName, DWORD style, int x, int y,
                                        int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID param)
{
    DarkMode_EnsureThreadHook();
    return RealCreateWindowExA(exStyle, className, windowName, style, x, y, width, height, parent, menu, instance, param);
}

static HWND WINAPI Hook_CreateWindowExW(DWORD exStyle, LPCWSTR className, LPCWSTR windowName, DWORD style, int x, int y,
                                        int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID param)
{
    DarkMode_EnsureThreadHook();
    return RealCreateWindowExW(exStyle, className, windowName, style, x, y, width, height, parent, menu, instance, param);
}

static HWND WINAPI Hook_CreateDialogParamA(HINSTANCE instance, LPCSTR templ, HWND parent, DLGPROC proc, LPARAM param)
{
    DarkMode_EnsureThreadHook();
    return RealCreateDialogParamA(instance, templ, parent, proc, param);
}

static HWND WINAPI Hook_CreateDialogParamW(HINSTANCE instance, LPCWSTR templ, HWND parent, DLGPROC proc, LPARAM param)
{
    DarkMode_EnsureThreadHook();
    return RealCreateDialogParamW(instance, templ, parent, proc, param);
}

static HWND WINAPI Hook_CreateDialogIndirectParamA(HINSTANCE instance, LPCDLGTEMPLATEA templ, HWND parent, DLGPROC proc, LPARAM param)
{
    DarkMode_EnsureThreadHook();
    return RealCreateDialogIndirectParamA(instance, templ, parent, proc, param);
}

static HWND WINAPI Hook_CreateDialogIndirectParamW(HINSTANCE instance, LPCDLGTEMPLATEW templ, HWND parent, DLGPROC proc, LPARAM param)
{
    DarkMode_EnsureThreadHook();
    return RealCreateDialogIndirectParamW(instance, templ, parent, proc, param);
}

static INT_PTR WINAPI Hook_DialogBoxParamA(HINSTANCE instance, LPCSTR templ, HWND parent, DLGPROC proc, LPARAM param)
{
    DarkMode_EnsureThreadHook();
    return RealDialogBoxParamA(instance, templ, parent, proc, param);
}

static INT_PTR WINAPI Hook_DialogBoxParamW(HINSTANCE instance, LPCWSTR templ, HWND parent, DLGPROC proc, LPARAM param)
{
    DarkMode_EnsureThreadHook();
    return RealDialogBoxParamW(instance, templ, parent, proc, param);
}

static INT_PTR WINAPI Hook_DialogBoxIndirectParamA(HINSTANCE instance, LPCDLGTEMPLATEA templ, HWND parent, DLGPROC proc, LPARAM param)
{
    DarkMode_EnsureThreadHook();
    return RealDialogBoxIndirectParamA(instance, templ, parent, proc, param);
}

static INT_PTR WINAPI Hook_DialogBoxIndirectParamW(HINSTANCE instance, LPCDLGTEMPLATEW templ, HWND parent, DLGPROC proc, LPARAM param)
{
    DarkMode_EnsureThreadHook();
    return RealDialogBoxIndirectParamW(instance, templ, parent, proc, param);
}

static int WINAPI Hook_MessageBoxA(HWND parent, LPCSTR text, LPCSTR caption, UINT type)
{
    DarkMode_EnsureThreadHook();
    return RealMessageBoxA(parent, text, caption, type);
}

static int WINAPI Hook_MessageBoxW(HWND parent, LPCWSTR text, LPCWSTR caption, UINT type)
{
    DarkMode_EnsureThreadHook();
    return RealMessageBoxW(parent, text, caption, type);
}

static COLORREF WINAPI Hook_GetSysColor(int index)
{
    return SalGetSysColor(index);
}

static HBRUSH WINAPI Hook_GetSysColorBrush(int index)
{
    return SalGetSysColorBrush(index);
}

struct CDarkModeImportHook
{
    const char* Name;
    void** Real;
    void* Hook;
    BOOL Colors; // TRUE = GetSysColor & co. (not patched in salamand.exe, it uses the redirect macros)
};

static CDarkModeImportHook DarkModeImportHooks[] =
    {
        {"CreateWindowExA", (void**)&RealCreateWindowExA, (void*)Hook_CreateWindowExA, FALSE},
        {"CreateWindowExW", (void**)&RealCreateWindowExW, (void*)Hook_CreateWindowExW, FALSE},
        {"CreateDialogParamA", (void**)&RealCreateDialogParamA, (void*)Hook_CreateDialogParamA, FALSE},
        {"CreateDialogParamW", (void**)&RealCreateDialogParamW, (void*)Hook_CreateDialogParamW, FALSE},
        {"CreateDialogIndirectParamA", (void**)&RealCreateDialogIndirectParamA, (void*)Hook_CreateDialogIndirectParamA, FALSE},
        {"CreateDialogIndirectParamW", (void**)&RealCreateDialogIndirectParamW, (void*)Hook_CreateDialogIndirectParamW, FALSE},
        {"DialogBoxParamA", (void**)&RealDialogBoxParamA, (void*)Hook_DialogBoxParamA, FALSE},
        {"DialogBoxParamW", (void**)&RealDialogBoxParamW, (void*)Hook_DialogBoxParamW, FALSE},
        {"DialogBoxIndirectParamA", (void**)&RealDialogBoxIndirectParamA, (void*)Hook_DialogBoxIndirectParamA, FALSE},
        {"DialogBoxIndirectParamW", (void**)&RealDialogBoxIndirectParamW, (void*)Hook_DialogBoxIndirectParamW, FALSE},
        {"MessageBoxA", (void**)&RealMessageBoxA, (void*)Hook_MessageBoxA, FALSE},
        {"MessageBoxW", (void**)&RealMessageBoxW, (void*)Hook_MessageBoxW, FALSE},
        {"GetSysColor", &RealGetSysColor, (void*)Hook_GetSysColor, TRUE},
        {"GetSysColorBrush", &RealGetSysColorBrush, (void*)Hook_GetSysColorBrush, TRUE},
};

static BOOL DarkModeImportHooksReady = FALSE;
static BOOL DarkModeMainModulePatched = FALSE;

static void DarkMode_InitImportHooks()
{
    if (DarkModeImportHooksReady)
        return;
    DarkModeImportHooksReady = TRUE;
    HMODULE user32 = GetModuleHandle("user32.dll");
    if (user32 == NULL)
        return;
    for (int i = 0; i < _countof(DarkModeImportHooks); i++)
        *DarkModeImportHooks[i].Real = (void*)GetProcAddress(user32, DarkModeImportHooks[i].Name);
}

static void DarkMode_PatchImports(HMODULE module, BOOL colors)
{
    DarkMode_InitImportHooks();
    if (module == NULL)
        return;

    __try
    {
        BYTE* base = (BYTE*)module;
        IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
            return;
        IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE)
            return;
        IMAGE_DATA_DIRECTORY* dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (dir->VirtualAddress == 0 || dir->Size == 0)
            return;

        IMAGE_IMPORT_DESCRIPTOR* imp = (IMAGE_IMPORT_DESCRIPTOR*)(base + dir->VirtualAddress);
        for (; imp->Name != 0; imp++)
        {
            const char* dllName = (const char*)(base + imp->Name);
            if (StrICmp(dllName, "user32.dll") != 0)
                continue;
            IMAGE_THUNK_DATA* thunk = (IMAGE_THUNK_DATA*)(base + imp->FirstThunk);
            for (; thunk->u1.Function != 0; thunk++)
            {
                void* func = (void*)thunk->u1.Function;
                for (int i = 0; i < _countof(DarkModeImportHooks); i++)
                {
                    CDarkModeImportHook* hook = &DarkModeImportHooks[i];
                    if (*hook->Real == NULL || func != *hook->Real || (hook->Colors && !colors))
                        continue;
                    DWORD oldProtect;
                    if (VirtualProtect(&thunk->u1.Function, sizeof(thunk->u1.Function), PAGE_READWRITE, &oldProtect))
                    {
                        thunk->u1.Function = (ULONG_PTR)hook->Hook;
                        VirtualProtect(&thunk->u1.Function, sizeof(thunk->u1.Function), oldProtect, &oldProtect);
                    }
                    break;
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        // a damaged import table, leave the module as it is
    }
}

void DarkMode_PatchModule(HMODULE module)
{
    DarkMode_PatchImports(module, TRUE);
}

// ****************************************************************************

// the Bug Reporter (salmon.exe) is a separate process, it reads this value from its registry key
static void DarkMode_SaveForBugReporter(BOOL enable)
{
    HKEY hKey;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Open Salamander\\Bug Reporter", 0, NULL,
                       REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        DWORD value = enable ? 1 : 0;
        RegSetValueEx(hKey, "Dark Mode", 0, REG_DWORD, (const BYTE*)&value, sizeof(value));
        RegCloseKey(hKey);
    }
}

void DarkMode_Apply(BOOL enable, BOOL force)
{
    CALL_STACK_MESSAGE3("DarkMode_Apply(%d, %d)", enable, force);
    enable = enable ? TRUE : FALSE;
    if (!force && enable == DarkModeActive)
        return;

    DarkMode_LoadApi();

    // switch the panel color scheme: dark mode uses the Dark scheme, the user can choose
    // another scheme on the Colors page afterwards (in both modes)
    if (enable && !DarkModeActive)
    {
        if (CurrentColors != DarkColors)
        {
            SchemeBeforeDarkMode = CurrentColors;
            CurrentColors = DarkColors;
        }
    }
    else if (!enable && DarkModeActive)
    {
        if (CurrentColors == DarkColors) // the user kept the Dark scheme, return to the previous one
            CurrentColors = SchemeBeforeDarkMode != NULL ? SchemeBeforeDarkMode : SalamanderColors;
        SchemeBeforeDarkMode = NULL;
    }
    DarkModeActive = enable;
    DarkMode_SaveForBugReporter(enable);

    // window creation hooks of salamand.exe (the windows of other threads, e.g. viewers and Find)
    if (enable && !DarkModeMainModulePatched)
    {
        DarkModeMainModulePatched = TRUE;
        DarkMode_PatchImports(GetModuleHandle(NULL), FALSE);
    }
    DarkMode_EnsureThreadHook();

    // popup menus (context menus, system menu) and other immersive parts of the system UI
    if (DynSetPreferredAppMode != NULL)
        DynSetPreferredAppMode(enable ? dpamForceDark : dpamDefault);
    if (DynRefreshImmersiveColorPolicyState != NULL)
        DynRefreshImmersiveColorPolicyState();
    if (DynFlushMenuThemes != NULL)
        DynFlushMenuThemes();

    // brushes obtained once in InitializeConstGraphics()
    HDialogBrush = SalGetSysColorBrush(COLOR_BTNFACE);
    HButtonTextBrush = SalGetSysColorBrush(COLOR_BTNTEXT);
    HMenuSelectedBkBrush = SalGetSysColorBrush(COLOR_HIGHLIGHT);
    HMenuSelectedTextBrush = SalGetSysColorBrush(COLOR_HIGHLIGHTTEXT);
    HMenuHilightBrush = SalGetSysColorBrush(COLOR_3DHILIGHT);
    HMenuGrayTextBrush = SalGetSysColorBrush(COLOR_3DSHADOW);

    // rebuild all colors, bitmaps and image lists as after a system color change
    ColorsChanged(TRUE, FALSE, TRUE);

    if (MainWindow != NULL && MainWindow->HWindow != NULL)
    {
        DarkMode_Subclass(MainWindow->HWindow);
        if (MainWindow->EditWindow != NULL && MainWindow->EditWindow->HWindow != NULL)
            DarkMode_Subclass(GetParent(MainWindow->EditWindow->HWindow));
        DarkMode_ApplyToWindow(MainWindow->HWindow);
    }
}

void DarkMode_Release()
{
    if (ThreadCBTHook != NULL)
    {
        UnhookWindowsHookEx(ThreadCBTHook);
        ThreadCBTHook = NULL;
    }
}
