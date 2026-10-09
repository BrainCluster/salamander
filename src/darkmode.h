// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// ****************************************************************************
//
// Dark mode (experimental)
//
// Most of Salamander's own UI (panels, menus, toolbars, directory/status lines,
// configuration tree, ...) is custom painted using GetSysColor() and
// GetSysColorBrush(). Those two calls are redirected (see the macros below) to
// SalGetSysColor()/SalGetSysColorBrush(), which return a dark palette while
// dark mode is active and the real system values otherwise.
//
// Standard Windows controls (dialogs, buttons, edits, list/tree views, scroll
// bars, title bars, popup menus) are switched to dark via the undocumented
// uxtheme dark mode API, DWM and a WM_CTLCOLOR* subclass of dialogs.
//

extern BOOL DarkModeActive; // TRUE = dark mode is currently applied

// returns a system color; the dark palette while dark mode is active
COLORREF SalGetSysColor(int index);
// returns a system color brush; the dark palette while dark mode is active
// (like GetSysColorBrush, the returned brush must not be deleted)
HBRUSH SalGetSysColorBrush(int index);

// switches dark mode on/off; 'force' re-applies even if the state did not change
// (used at startup after the configuration is loaded)
void DarkMode_Apply(BOOL enable, BOOL force);

// applies the dark (or light) look to a window and its children (title bar, scroll bars,
// standard controls); safe to call on any window
void DarkMode_ApplyToWindow(HWND hWnd);

// patches the imports of a plugin DLL (system colors, window creation), called after the DLL is loaded
void DarkMode_PatchModule(HMODULE module);

void DarkMode_Release();

#ifndef DARKMODE_NO_REDIRECT
#define GetSysColor SalGetSysColor
#define GetSysColorBrush SalGetSysColorBrush
#endif // DARKMODE_NO_REDIRECT
