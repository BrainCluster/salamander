// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

// "This PC" plugin: a panel listing all drives (local, removable, optical, network) with their
// capacity and fullness, the panel goes there from the root of a drive (the ".." item)

#include "precomp.h"

#include "thispc.h"

#include "thispc.rh"
#include "thispc.rh2"
#include "lang\lang.rh"

// ****************************************************************************

HINSTANCE DLLInstance = NULL; // handle to the SPL - language-independent resources
HINSTANCE HLanguage = NULL;   // handle to the SLG - language-dependent resources

// plugin interface object whose methods are called from Salamander
CPluginInterface PluginInterface;

// file system interface
CPluginInterfaceForFS InterfaceForFS;

// general Salamander interface - valid from plugin start until it is unloaded
CSalamanderGeneralAbstract* SalamanderGeneral = NULL;

// variable definition for "dbg.h"
CSalamanderDebugAbstract* SalamanderDebug = NULL;

// variable definition for "spl_com.h"
int SalamanderVersion = 0;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
        DLLInstance = hinstDLL;
    return TRUE; // DLL can be loaded
}

char* LoadStr(int resID)
{
    return SalamanderGeneral->LoadStr(HLanguage, resID);
}

int WINAPI SalamanderPluginGetReqVer()
{
    return LAST_VERSION_OF_SALAMANDER;
}

CPluginInterfaceAbstract* WINAPI SalamanderPluginEntry(CSalamanderPluginEntryAbstract* salamander)
{
    // set SalamanderDebug for "dbg.h"
    SalamanderDebug = salamander->GetSalamanderDebug();
    // set SalamanderVersion for "spl_com.h"
    SalamanderVersion = salamander->GetVersion();

    CALL_STACK_MESSAGE1("SalamanderPluginEntry()");

    // this plugin is built for the current Salamander version and newer - verify it
    if (SalamanderVersion < LAST_VERSION_OF_SALAMANDER)
    { // reject older versions
        MessageBox(salamander->GetParentWindow(),
                   REQUIRE_LAST_VERSION_OF_SALAMANDER,
                   "This PC" /* do not translate */, MB_OK | MB_ICONERROR);
        return NULL;
    }

    // load the language module (.slg)
    HLanguage = salamander->LoadLanguageModule(salamander->GetParentWindow(), "This PC" /* do not translate */);
    if (HLanguage == NULL)
        return NULL;

    // obtain the general Salamander interface
    SalamanderGeneral = salamander->GetSalamanderGeneral();

    // set the basic plugin information
    salamander->SetBasicPluginData(LoadStr(IDS_PLUGINNAME),
                                   FUNCTION_FILESYSTEM,
                                   VERSINFO_VERSION_NO_PLATFORM,
                                   VERSINFO_COPYRIGHT,
                                   LoadStr(IDS_PLUGIN_DESCRIPTION),
                                   "THISPC" /* do not translate */, NULL, "thispc");

    salamander->SetPluginHomePageURL("github.com/OpenSalamander/salamander");

    // obtain our FS name (it may not be "thispc", Salamander can adjust it)
    SalamanderGeneral->GetPluginFSName(AssignedFSName, 0);

    if (!InitFS())
        return NULL; // error

    return &PluginInterface;
}

// ****************************************************************************
//
// CPluginInterface
//

void WINAPI
CPluginInterface::About(HWND parent)
{
    char buf[1000];
    _snprintf_s(buf, _TRUNCATE,
                "%s " VERSINFO_VERSION "\n\n" VERSINFO_COPYRIGHT "\n\n"
                "%s",
                LoadStr(IDS_PLUGINNAME),
                LoadStr(IDS_PLUGIN_DESCRIPTION));
    SalamanderGeneral->SalMessageBox(parent, buf, LoadStr(IDS_ABOUT), MB_OK | MB_ICONINFORMATION);
}

BOOL WINAPI
CPluginInterface::Release(HWND parent, BOOL force)
{
    CALL_STACK_MESSAGE2("CPluginInterface::Release(, %d)", force);
    ReleaseFS();
    return TRUE;
}

void WINAPI
CPluginInterface::Connect(HWND parent, CSalamanderConnectAbstract* salamander)
{
    CALL_STACK_MESSAGE1("CPluginInterface::Connect(,)");

    HBITMAP hBmp = (HBITMAP)LoadImage(DLLInstance, MAKEINTRESOURCE(IDB_THISPC),
                                      IMAGE_BITMAP, 16, 16, LR_DEFAULTCOLOR);
    salamander->SetBitmapWithIcons(hBmp);
    DeleteObject(hBmp);
    salamander->SetChangeDriveMenuItem(LoadStr(IDS_DRIVEMENUTEXT), 0);
    salamander->SetPluginIcon(0);
    salamander->SetPluginMenuAndToolbarIcon(0);
}

void WINAPI
CPluginInterface::ReleasePluginDataInterface(CPluginDataInterfaceAbstract* pluginData)
{
    delete ((CPluginDataInterface*)pluginData);
}

CPluginInterfaceForFSAbstract* WINAPI
CPluginInterface::GetInterfaceForFS()
{
    return &InterfaceForFS;
}

extern HIMAGELIST DriveImageList;

void WINAPI
CPluginInterface::Event(int event, DWORD param)
{
    if (event == PLUGINEVENT_COLORSCHANGED && DriveImageList != NULL)
        ImageList_SetBkColor(DriveImageList, SalamanderGeneral->GetCurrentColor(SALCOL_ITEM_BK_NORMAL));
}
