// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include "thispc.h"

#include "thispc.rh"
#include "thispc.rh2"
#include "lang\lang.rh"

// FS name assigned by Salamander after the plugin is loaded
char AssignedFSName[MAX_PATH] = "";

// image list for simple FS icons (used until the real drive icons are loaded)
HIMAGELIST DriveImageList = NULL;

// global variables where we store pointers to Salamander's global variables
const CFileData** TransferFileData = NULL;
int* TransferIsDir = NULL;
char* TransferBuffer = NULL;
int* TransferLen = NULL;
DWORD* TransferRowData = NULL;
CPluginDataInterfaceAbstract** TransferPluginDataIface = NULL;
DWORD* TransferActCustomData = NULL;

// marker for the panel: column text "\x01" + (char)(percent + 1) + label is drawn as a fullness bar
#define PANEL_BAR_MARKER '\x01'

// how long we wait for slow drives (disconnected network drives, drives without media)
#define DRIVE_QUERY_TIMEOUT 2500

static volatile LONG RunningQueryThreads = 0; // number of running drive query threads

// ****************************************************************************
// FILE SYSTEM SECTION
// ****************************************************************************

BOOL InitFS()
{
    DriveImageList = ImageList_Create(16, 16, ILC_MASK | SalamanderGeneral->GetImageListColorFlags(), 1, 0);
    if (DriveImageList == NULL)
    {
        TRACE_E("Failed to create the image list.");
        return FALSE;
    }
    ImageList_SetImageCount(DriveImageList, 1); // initialization
    ImageList_SetBkColor(DriveImageList, SalamanderGeneral->GetCurrentColor(SALCOL_ITEM_BK_NORMAL));

    HICON hIcon = SalamanderGeneral->GetSalamanderIcon(SALICON_DIRECTORY, SALICONSIZE_16);
    if (hIcon != NULL)
    {
        ImageList_ReplaceIcon(DriveImageList, 0, hIcon);
        DestroyIcon(hIcon);
    }
    return TRUE;
}

void ReleaseFS()
{
    // wait for the drive query threads (a hung network drive can keep a thread running for a while),
    // the plugin code must not be unloaded while they run
    int waited = 0;
    while (RunningQueryThreads > 0 && waited < 10000)
    {
        Sleep(50);
        waited += 50;
    }
    if (DriveImageList != NULL)
    {
        ImageList_Destroy(DriveImageList);
        DriveImageList = NULL;
    }
}

// ****************************************************************************
//
// Drive enumeration
//

struct CDriveQuery
{
    CDriveInfo Info;
    volatile LONG RefCount; // the query is shared by the enumerating thread and the query thread
};

static void ReleaseDriveQuery(CDriveQuery* query)
{
    if (InterlockedDecrement(&query->RefCount) == 0)
        delete query;
}

static unsigned __stdcall DriveQueryThread(void* param)
{
    CDriveQuery* query = (CDriveQuery*)param;
    CDriveInfo* info = &query->Info;

    // no "insert disk" message boxes for empty drives
    DWORD oldMode;
    SetThreadErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX, &oldMode);

    if (info->Type == DRIVE_REMOTE)
    {
        char drive[3] = {info->Root[0], ':', 0};
        DWORD len = SAL_ARRAYSIZE(info->RemoteName);
        if (WNetGetConnection(drive, info->RemoteName, &len) != NO_ERROR)
            info->RemoteName[0] = 0;
    }

    DWORD serial, maxLen, flags;
    if (GetVolumeInformation(info->Root, info->Label, SAL_ARRAYSIZE(info->Label), &serial, &maxLen, &flags,
                             info->FileSystem, SAL_ARRAYSIZE(info->FileSystem)))
    {
        info->Ready = TRUE;
    }
    else
    {
        info->Label[0] = 0;
        info->FileSystem[0] = 0;
    }

    ULARGE_INTEGER freeAvail, total, totalFree;
    if (GetDiskFreeSpaceEx(info->Root, &freeAvail, &total, &totalFree))
    {
        info->Total = CQuadWord(total.LowPart, total.HighPart);
        info->Free = CQuadWord(freeAvail.LowPart, freeAvail.HighPart);
        info->SizeValid = TRUE;
        info->Ready = TRUE;
    }

    ReleaseDriveQuery(query);
    InterlockedDecrement(&RunningQueryThreads);
    return 0;
}

int EnumDrives(CDriveInfo* drives, int maxDrives)
{
    CALL_STACK_MESSAGE1("EnumDrives()");

    CDriveQuery* queries[26];
    HANDLE threads[26];
    int count = 0;

    DWORD mask = GetLogicalDrives();
    for (int i = 0; i < 26 && count < maxDrives; i++)
    {
        if ((mask & (1 << i)) == 0)
            continue;

        CDriveQuery* query = new CDriveQuery;
        if (query == NULL)
            break;
        memset(&query->Info, 0, sizeof(query->Info));
        query->Info.Root[0] = (char)('A' + i);
        query->Info.Root[1] = ':';
        query->Info.Root[2] = '\\';
        query->Info.Root[3] = 0;
        query->Info.Type = GetDriveType(query->Info.Root);
        if (query->Info.Type == DRIVE_NO_ROOT_DIR) // e.g. a SUBST drive pointing to a deleted path
        {
            delete query;
            continue;
        }
        query->RefCount = 2;

        InterlockedIncrement(&RunningQueryThreads);
        unsigned threadID;
        HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, DriveQueryThread, query, 0, &threadID);
        if (thread == NULL)
        {
            InterlockedDecrement(&RunningQueryThreads);
            query->RefCount = 1; // no thread, only the drive type is known
        }
        queries[count] = query;
        threads[count] = thread;
        count++;
    }

    // wait for all drives (in parallel) at most DRIVE_QUERY_TIMEOUT ms
    DWORD start = GetTickCount();
    for (int i = 0; i < count; i++)
    {
        if (threads[i] == NULL)
            continue;
        DWORD elapsed = GetTickCount() - start;
        DWORD wait = elapsed < DRIVE_QUERY_TIMEOUT ? DRIVE_QUERY_TIMEOUT - elapsed : 0;
        if (WaitForSingleObject(threads[i], wait) != WAIT_OBJECT_0)
        {
            // the drive does not answer: list it only with its type, the thread finishes on its own
            CDriveInfo* info = &drives[i];
            memset(info, 0, sizeof(*info));
            memcpy(info->Root, queries[i]->Info.Root, sizeof(info->Root));
            info->Type = queries[i]->Info.Type;
            CloseHandle(threads[i]);
            threads[i] = NULL;
            ReleaseDriveQuery(queries[i]);
            queries[i] = NULL;
            continue;
        }
        CloseHandle(threads[i]);
        threads[i] = NULL;
    }
    for (int i = 0; i < count; i++)
    {
        if (queries[i] != NULL) // finished (or no thread)
        {
            drives[i] = queries[i]->Info;
            ReleaseDriveQuery(queries[i]);
        }
    }
    return count;
}

static const char* GetDriveTypeName(UINT type)
{
    switch (type)
    {
    case DRIVE_FIXED:
        return LoadStr(IDS_TYPE_FIXED);
    case DRIVE_REMOVABLE:
        return LoadStr(IDS_TYPE_REMOVABLE);
    case DRIVE_CDROM:
        return LoadStr(IDS_TYPE_CDROM);
    case DRIVE_REMOTE:
        return LoadStr(IDS_TYPE_REMOTE);
    case DRIVE_RAMDISK:
        return LoadStr(IDS_TYPE_RAMDISK);
    }
    return LoadStr(IDS_TYPE_UNKNOWN);
}

// label shown in the Label column: volume label, for network drives the share name if there is no label
static const char* GetDriveLabel(const CDriveInfo* info)
{
    if (info->Label[0] != 0)
        return info->Label;
    if (info->RemoteName[0] != 0)
        return info->RemoteName;
    return "";
}

static int GetUsedPercent(const CDriveInfo* info)
{
    if (!info->SizeValid || info->Total == CQuadWord(0, 0))
        return -1;
    CQuadWord used = info->Total - info->Free;
    int percent = (int)((used * CQuadWord(100, 0)) / info->Total).Value;
    if (percent < 0)
        percent = 0;
    if (percent > 100)
        percent = 100;
    return percent;
}

//
// ****************************************************************************
// CPluginInterfaceForFS
//

CPluginFSInterfaceAbstract* WINAPI
CPluginInterfaceForFS::OpenFS(const char* fsName, int fsNameIndex)
{
    ActiveFSCount++;
    return new CPluginFSInterface;
}

void WINAPI
CPluginInterfaceForFS::CloseFS(CPluginFSInterfaceAbstract* fs)
{
    CPluginFSInterface* thisPCFS = (CPluginFSInterface*)fs; // ensure the correct destructor is called
    ActiveFSCount--;
    if (thisPCFS != NULL)
        delete thisPCFS;
}

void WINAPI
CPluginInterfaceForFS::ExecuteChangeDriveMenuItem(int panel)
{
    CALL_STACK_MESSAGE2("CPluginInterfaceForFS::ExecuteChangeDriveMenuItem(%d)", panel);
    SalamanderGeneral->ChangePanelPathToPluginFS(panel, AssignedFSName, "");
}

void WINAPI
CPluginInterfaceForFS::ExecuteOnFS(int panel, CPluginFSInterfaceAbstract* pluginFS,
                                   const char* pluginFSName, int pluginFSNameIndex,
                                   CFileData& file, int isDir)
{
    CALL_STACK_MESSAGE2("CPluginInterfaceForFS::ExecuteOnFS(%d, , , , ,)", panel);
    if (isDir == 1 && file.PluginData != NULL) // a drive: open its root
    {
        const CDriveInfo* info = (const CDriveInfo*)file.PluginData;
        char root[4];
        memcpy(root, info->Root, sizeof(root));
        SalamanderGeneral->ChangePanelPathToDisk(panel, root);
    }
}

BOOL WINAPI
CPluginInterfaceForFS::DisconnectFS(HWND parent, BOOL isInPanel, int panel,
                                    CPluginFSInterfaceAbstract* pluginFS,
                                    const char* pluginFSName, int pluginFSNameIndex)
{
    BOOL ret = FALSE;
    if (isInPanel)
    {
        SalamanderGeneral->DisconnectFSFromPanel(parent, panel);
        ret = SalamanderGeneral->GetPanelPluginFS(panel) != pluginFS;
    }
    else
        ret = SalamanderGeneral->CloseDetachedFS(parent, pluginFS);
    return ret;
}

//
// ****************************************************************************
// CPluginDataInterface
//

void WINAPI
CPluginDataInterface::ReleasePluginData(CFileData& file, BOOL isDir)
{
    if (file.PluginData != NULL)
        delete (CDriveInfo*)file.PluginData;
}

HIMAGELIST WINAPI
CPluginDataInterface::GetSimplePluginIcons(int iconSize)
{
    return DriveImageList;
}

HICON WINAPI
CPluginDataInterface::GetPluginIcon(const CFileData* file, int iconSize, BOOL& destroyIcon)
{
    destroyIcon = FALSE;
    if (file->PluginData == NULL)
        return NULL;
    const CDriveInfo* info = (const CDriveInfo*)file->PluginData;
    HICON hIcon = NULL;
    if (SalamanderGeneral->GetFileIcon(info->Root, FALSE, &hIcon, iconSize, TRUE, TRUE))
        destroyIcon = TRUE;
    return hIcon;
}

int WINAPI
CPluginDataInterface::CompareFilesFromFS(const CFileData* file1, const CFileData* file2)
{
    return strcmp(file1->Name, file2->Name);
}

// column callbacks: obtain the texts of the drive in *TransferFileData

static const CDriveInfo* GetTransferDrive()
{
    if (*TransferIsDir == 2) // up-dir (not used in this FS)
        return NULL;
    return (const CDriveInfo*)(*TransferFileData)->PluginData;
}

static void SetTransferText(const char* text)
{
    lstrcpyn(TransferBuffer, text, TRANSFER_BUFFER_MAX);
    *TransferLen = (int)strlen(TransferBuffer);
}

static void WINAPI GetLabelText()
{
    const CDriveInfo* info = GetTransferDrive();
    SetTransferText(info != NULL ? GetDriveLabel(info) : "");
}

static void WINAPI GetTypeText()
{
    const CDriveInfo* info = GetTransferDrive();
    SetTransferText(info != NULL ? GetDriveTypeName(info->Type) : "");
}

static void WINAPI GetTotalText()
{
    const CDriveInfo* info = GetTransferDrive();
    if (info != NULL && info->SizeValid)
        SalamanderGeneral->PrintDiskSize(TransferBuffer, info->Total, 0);
    else
        TransferBuffer[0] = 0;
    *TransferLen = (int)strlen(TransferBuffer);
}

static void WINAPI GetFreeText()
{
    const CDriveInfo* info = GetTransferDrive();
    if (info != NULL && info->SizeValid)
        SalamanderGeneral->PrintDiskSize(TransferBuffer, info->Free, 0);
    else
        TransferBuffer[0] = 0;
    *TransferLen = (int)strlen(TransferBuffer);
}

static void WINAPI GetUsedText()
{
    const CDriveInfo* info = GetTransferDrive();
    int percent = info != NULL ? GetUsedPercent(info) : -1;
    if (percent < 0)
    {
        *TransferLen = 0;
        return;
    }
    // drawn by the panel as a fullness bar followed by the percentage
    TransferBuffer[0] = PANEL_BAR_MARKER;
    TransferBuffer[1] = (char)(percent + 1);
    *TransferLen = 2 + sprintf(TransferBuffer + 2, "%d %%", percent);
}

static void InitColumn(CColumn* column, int nameID, int descriptionID, FColumnGetText getText, BOOL leftAlignment)
{
    lstrcpyn(column->Name, LoadStr(nameID), COLUMN_NAME_MAX);
    lstrcpyn(column->Description, LoadStr(descriptionID), COLUMN_DESCRIPTION_MAX);
    column->GetText = getText;
    column->CustomData = 0;
    column->SupportSorting = 0;
    column->LeftAlignment = leftAlignment ? 1 : 0;
    column->ID = COLUMN_ID_CUSTOM;
    column->Width = 0;
    column->FixedWidth = 0;
}

void WINAPI
CPluginDataInterface::SetupView(BOOL leftPanel, CSalamanderViewAbstract* view, const char* archivePath,
                                const CFileData* upperDir)
{
    // obtain the global variables for quick access
    view->GetTransferVariables(TransferFileData, TransferIsDir, TransferBuffer, TransferLen,
                               TransferRowData, TransferPluginDataIface, TransferActCustomData);

    if (view->GetViewMode() == VIEW_MODE_DETAILED)
    {
        view->SetViewMode(VIEW_MODE_DETAILED, VALID_DATA_NONE); // keep only the Name column

        CColumn column;
        int index = 1;
        InitColumn(&column, IDS_COL_LABEL, IDS_COL_LABEL_DESC, GetLabelText, TRUE);
        view->InsertColumn(index++, &column);
        InitColumn(&column, IDS_COL_TYPE, IDS_COL_TYPE_DESC, GetTypeText, TRUE);
        view->InsertColumn(index++, &column);
        InitColumn(&column, IDS_COL_TOTAL, IDS_COL_TOTAL_DESC, GetTotalText, FALSE);
        view->InsertColumn(index++, &column);
        InitColumn(&column, IDS_COL_FREE, IDS_COL_FREE_DESC, GetFreeText, FALSE);
        view->InsertColumn(index++, &column);
        InitColumn(&column, IDS_COL_USED, IDS_COL_USED_DESC, GetUsedText, TRUE);
        column.FixedWidth = 1; // the bar has no natural width, the column has a fixed width
        column.Width = 170;
        view->InsertColumn(index++, &column);
    }
}

BOOL WINAPI
CPluginDataInterface::GetInfoLineContent(int panel, const CFileData* file, BOOL isDir,
                                         int selectedFiles, int selectedDirs, BOOL displaySize,
                                         const CQuadWord& selectedSize, char* buffer,
                                         DWORD* hotTexts, int& hotTextsCount)
{
    hotTextsCount = 0;
    if (file == NULL || file->PluginData == NULL)
        return FALSE;
    const CDriveInfo* info = (const CDriveInfo*)file->PluginData;

    char name[MAX_PATH + 50];
    const char* label = GetDriveLabel(info);
    if (label[0] != 0)
        _snprintf_s(name, _TRUNCATE, "%s (%c:)", label, info->Root[0]);
    else
        _snprintf_s(name, _TRUNCATE, "%s (%c:)", GetDriveTypeName(info->Type), info->Root[0]);

    if (info->SizeValid)
    {
        char freeText[100], totalText[100];
        SalamanderGeneral->PrintDiskSize(freeText, info->Free, 0);
        SalamanderGeneral->PrintDiskSize(totalText, info->Total, 0);
        _snprintf_s(buffer, 1000, _TRUNCATE, LoadStr(IDS_INFOLINE_SIZE), name, freeText, totalText,
                    info->FileSystem[0] != 0 ? info->FileSystem : "?");
    }
    else
        _snprintf_s(buffer, 1000, _TRUNCATE, LoadStr(IDS_INFOLINE_NOTREADY), name);
    return TRUE;
}

//
// ****************************************************************************
// CPluginFSInterface
//

BOOL WINAPI
CPluginFSInterface::GetCurrentPath(char* userPart)
{
    userPart[0] = 0;
    return TRUE;
}

BOOL WINAPI
CPluginFSInterface::GetRootPath(char* userPart)
{
    userPart[0] = 0;
    return TRUE;
}

BOOL WINAPI
CPluginFSInterface::GetFullName(CFileData& file, int isDir, char* buf, int bufSize)
{
    if (isDir == 2 || file.PluginData == NULL)
        return FALSE; // there is no up-dir
    lstrcpyn(buf, ((const CDriveInfo*)file.PluginData)->Root, bufSize);
    return TRUE;
}

BOOL WINAPI
CPluginFSInterface::GetFullFSPath(HWND parent, const char* fsName, char* path, int pathSize, BOOL& success)
{
    return FALSE; // translation is not possible, let Salamander report the error itself
}

BOOL WINAPI
CPluginFSInterface::IsCurrentPath(int currentFSNameIndex, int fsNameIndex, const char* userPart)
{
    return currentFSNameIndex == fsNameIndex && userPart[0] == 0;
}

BOOL WINAPI
CPluginFSInterface::IsOurPath(int currentFSNameIndex, int fsNameIndex, const char* userPart)
{
    return TRUE;
}

BOOL WINAPI
CPluginFSInterface::ChangePath(int currentFSNameIndex, char* fsName, int fsNameIndex,
                               const char* userPart, char* cutFileName, BOOL* pathWasCut,
                               BOOL forceRefresh, int mode)
{
    if (cutFileName != NULL)
        *cutFileName = 0;
    // there is only one path (the root), any sub-path is shortened to it
    if (pathWasCut != NULL)
        *pathWasCut = userPart != NULL && userPart[0] != 0;
    if (FatalError)
    {
        FatalError = FALSE;
        return FALSE; // ListCurrentPath failed (out of memory), fatal error
    }
    return TRUE;
}

BOOL WINAPI
CPluginFSInterface::ListCurrentPath(CSalamanderDirectoryAbstract* dir,
                                    CPluginDataInterfaceAbstract*& pluginData,
                                    int& iconsType, BOOL forceRefresh)
{
    CALL_STACK_MESSAGE1("CPluginFSInterface::ListCurrentPath()");

    CDriveInfo drives[26];
    int count = EnumDrives(drives, 26);

    dir->SetValidData(VALID_DATA_NONE);
    for (int i = 0; i < count; i++)
    {
        CFileData file;
        memset(&file, 0, sizeof(file));
        char name[3] = {drives[i].Root[0], ':', 0};
        file.Name = SalamanderGeneral->DupStr(name);
        if (file.Name == NULL)
            goto ON_FATAL_ERROR;
        file.NameLen = 2;
        file.Ext = file.Name + file.NameLen; // drives have no extension
        file.Attr = FILE_ATTRIBUTE_DIRECTORY;
        file.IconOverlayIndex = ICONOVERLAYINDEX_NOTUSED;
        CDriveInfo* info = new CDriveInfo(drives[i]);
        if (info == NULL)
        {
            SalamanderGeneral->Free(file.Name);
            goto ON_FATAL_ERROR;
        }
        file.PluginData = (DWORD_PTR)info; // released in CPluginDataInterface::ReleasePluginData
        if (!dir->AddDir(NULL, file, NULL))
        {
            SalamanderGeneral->Free(file.Name);
            delete info;
            goto ON_FATAL_ERROR;
        }
    }

    iconsType = pitFromPlugin;
    pluginData = new CPluginDataInterface;
    if (pluginData == NULL)
        goto ON_FATAL_ERROR;
    return TRUE;

ON_FATAL_ERROR:
    dir->Clear(NULL);
    FatalError = TRUE;
    return FALSE;
}

BOOL WINAPI
CPluginFSInterface::TryCloseOrDetach(BOOL forceClose, BOOL canDetach, BOOL& detach, int reason)
{
    detach = FALSE;
    return TRUE;
}

DWORD WINAPI
CPluginFSInterface::GetSupportedServices()
{
    return FS_SERVICE_GETFSICON |
           FS_SERVICE_SHOWPROPERTIES |
           FS_SERVICE_GETPATHFORMAINWNDTITLE |
           FS_SERVICE_OPENACTIVEFOLDER;
}

HICON WINAPI
CPluginFSInterface::GetFSIcon(BOOL& destroyIcon)
{
    destroyIcon = FALSE;
    HICON icon = NULL;
    LPITEMIDLIST pidl = NULL;
    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidl)) && pidl != NULL)
    {
        if (SalamanderGeneral->GetFileIcon((const char*)pidl, TRUE, &icon, SALICONSIZE_16, FALSE, FALSE))
            destroyIcon = TRUE;
        else
            icon = NULL;
        CoTaskMemFree(pidl);
    }
    return icon;
}

BOOL WINAPI
CPluginFSInterface::GetPathForMainWindowTitle(const char* fsName, int mode, char* buf, int bufSize)
{
    lstrcpyn(buf, LoadStr(IDS_TITLE), bufSize);
    return TRUE;
}

void WINAPI
CPluginFSInterface::ShowProperties(const char* fsName, HWND parent, int panel,
                                   int selectedFiles, int selectedDirs)
{
    BOOL isDir;
    const CFileData* file = SalamanderGeneral->GetPanelFocusedItem(panel, &isDir);
    if (file == NULL || file->PluginData == NULL)
        return;
    const CDriveInfo* info = (const CDriveInfo*)file->PluginData;
    WCHAR root[4] = {(WCHAR)info->Root[0], L':', L'\\', 0};
    SHObjectProperties(parent, SHOP_FILEPATH, root, NULL);
}

void WINAPI
CPluginFSInterface::OpenActiveFolder(const char* fsName, HWND parent)
{
    // This PC in Windows Explorer
    ShellExecute(parent, "open", "explorer.exe", "shell:MyComputerFolder", NULL, SW_SHOWNORMAL);
}
