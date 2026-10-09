// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// general Salamander interface - valid from plugin start until it is unloaded
extern CSalamanderGeneralAbstract* SalamanderGeneral;

// FS name assigned by Salamander after the plugin is loaded
extern char AssignedFSName[MAX_PATH];

extern HINSTANCE DLLInstance; // handle to the SPL - language-independent resources
extern HINSTANCE HLanguage;   // handle to the SLG - language-dependent resources

char* LoadStr(int resID);

BOOL InitFS();
void ReleaseFS();

#define SAL_ARRAYSIZE(x) (sizeof(x) / sizeof(x[0]))

// ****************************************************************************
//
// CDriveInfo - one item of the "This PC" listing (stored in CFileData::PluginData)
//

struct CDriveInfo
{
    char Root[4];              // "C:\"
    UINT Type;                 // DRIVE_xxx
    char Label[MAX_PATH];      // volume label (for network drives the remote name if there is no label)
    char FileSystem[32];       // "NTFS", ...
    char RemoteName[MAX_PATH]; // "\\server\share" for network drives
    BOOL Ready;                // volume information was obtained (media present, drive accessible)
    BOOL SizeValid;            // Total and Free are valid
    CQuadWord Total;
    CQuadWord Free;
};

// fills 'drives' with information about all drives (drive letters); slow drives (disconnected network
// drives, drives without media) are queried in parallel with a time limit, drives that do not answer
// in time are listed without label and sizes
int EnumDrives(CDriveInfo* drives, int maxDrives);

// ****************************************************************************
//
// CPluginInterface
//

class CPluginInterfaceForFS : public CPluginInterfaceForFSAbstract
{
protected:
    int ActiveFSCount; // number of active FS interfaces (only to verify deallocation)

public:
    CPluginInterfaceForFS() { ActiveFSCount = 0; }
    int GetActiveFSCount() { return ActiveFSCount; }

    virtual CPluginFSInterfaceAbstract* WINAPI OpenFS(const char* fsName, int fsNameIndex);
    virtual void WINAPI CloseFS(CPluginFSInterfaceAbstract* fs);

    virtual void WINAPI ExecuteOnFS(int panel, CPluginFSInterfaceAbstract* pluginFS,
                                    const char* pluginFSName, int pluginFSNameIndex,
                                    CFileData& file, int isDir);
    virtual BOOL WINAPI DisconnectFS(HWND parent, BOOL isInPanel, int panel,
                                     CPluginFSInterfaceAbstract* pluginFS,
                                     const char* pluginFSName, int pluginFSNameIndex);

    virtual void WINAPI ConvertPathToInternal(const char* fsName, int fsNameIndex,
                                              char* fsUserPart) {}
    virtual void WINAPI ConvertPathToExternal(const char* fsName, int fsNameIndex,
                                              char* fsUserPart) {}

    virtual void WINAPI EnsureShareExistsOnServer(int panel, const char* server, const char* share) {}

    virtual void WINAPI ExecuteChangeDriveMenuItem(int panel);
    virtual BOOL WINAPI ChangeDriveMenuItemContextMenu(HWND parent, int panel, int x, int y,
                                                       CPluginFSInterfaceAbstract* pluginFS,
                                                       const char* pluginFSName, int pluginFSNameIndex,
                                                       BOOL isDetachedFS, BOOL& refreshMenu,
                                                       BOOL& closeMenu, int& postCmd, void*& postCmdParam)
    {
        return FALSE;
    }
    virtual void WINAPI ExecuteChangeDrivePostCommand(int panel, int postCmd, void* postCmdParam) {}
};

class CPluginInterface : public CPluginInterfaceAbstract
{
public:
    virtual void WINAPI About(HWND parent);

    virtual BOOL WINAPI Release(HWND parent, BOOL force);

    virtual void WINAPI LoadConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry) {}
    virtual void WINAPI SaveConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry) {}
    virtual void WINAPI Configuration(HWND parent) {}

    virtual void WINAPI Connect(HWND parent, CSalamanderConnectAbstract* salamander);

    virtual void WINAPI ReleasePluginDataInterface(CPluginDataInterfaceAbstract* pluginData);

    virtual CPluginInterfaceForArchiverAbstract* WINAPI GetInterfaceForArchiver() { return NULL; }
    virtual CPluginInterfaceForViewerAbstract* WINAPI GetInterfaceForViewer() { return NULL; }
    virtual CPluginInterfaceForMenuExtAbstract* WINAPI GetInterfaceForMenuExt() { return NULL; }
    virtual CPluginInterfaceForFSAbstract* WINAPI GetInterfaceForFS();
    virtual CPluginInterfaceForThumbLoaderAbstract* WINAPI GetInterfaceForThumbLoader() { return NULL; }

    virtual void WINAPI Event(int event, DWORD param);
    virtual void WINAPI ClearHistory(HWND parent) {}
    virtual void WINAPI AcceptChangeOnPathNotification(const char* path, BOOL includingSubdirs) {}

    virtual void WINAPI PasswordManagerEvent(HWND parent, int event) {}
};

// ****************************************************************************
//
// CPluginDataInterface
//

class CPluginDataInterface : public CPluginDataInterfaceAbstract
{
public:
    virtual BOOL WINAPI CallReleaseForFiles() { return FALSE; }
    virtual BOOL WINAPI CallReleaseForDirs() { return TRUE; }

    virtual void WINAPI ReleasePluginData(CFileData& file, BOOL isDir);

    virtual void WINAPI GetFileDataForUpDir(const char* archivePath, CFileData& upDir) {}
    virtual BOOL WINAPI GetFileDataForNewDir(const char* dirName, CFileData& dir) { return FALSE; }

    virtual HIMAGELIST WINAPI GetSimplePluginIcons(int iconSize);
    virtual BOOL WINAPI HasSimplePluginIcon(CFileData& file, BOOL isDir) { return FALSE; }
    virtual HICON WINAPI GetPluginIcon(const CFileData* file, int iconSize, BOOL& destroyIcon);
    virtual int WINAPI CompareFilesFromFS(const CFileData* file1, const CFileData* file2);

    virtual void WINAPI SetupView(BOOL leftPanel, CSalamanderViewAbstract* view, const char* archivePath,
                                  const CFileData* upperDir);
    virtual void WINAPI ColumnFixedWidthShouldChange(BOOL leftPanel, const CColumn* column, int newFixedWidth) {}
    virtual void WINAPI ColumnWidthWasChanged(BOOL leftPanel, const CColumn* column, int newWidth) {}

    virtual BOOL WINAPI GetInfoLineContent(int panel, const CFileData* file, BOOL isDir, int selectedFiles,
                                           int selectedDirs, BOOL displaySize, const CQuadWord& selectedSize,
                                           char* buffer, DWORD* hotTexts, int& hotTextsCount);

    virtual BOOL WINAPI CanBeCopiedToClipboard() { return TRUE; }

    virtual BOOL WINAPI GetByteSize(const CFileData* file, BOOL isDir, CQuadWord* size) { return FALSE; }
    virtual BOOL WINAPI GetLastWriteDate(const CFileData* file, BOOL isDir, SYSTEMTIME* date) { return FALSE; }
    virtual BOOL WINAPI GetLastWriteTime(const CFileData* file, BOOL isDir, SYSTEMTIME* time) { return FALSE; }
};

// ****************************************************************************
//
// CPluginFSInterface
//

class CPluginFSInterface : public CPluginFSInterfaceAbstract
{
public:
    BOOL FatalError; // ListCurrentPath failed (out of memory), ChangePath must return FALSE

public:
    CPluginFSInterface() { FatalError = FALSE; }

    virtual BOOL WINAPI GetCurrentPath(char* userPart);
    virtual BOOL WINAPI GetFullName(CFileData& file, int isDir, char* buf, int bufSize);
    virtual BOOL WINAPI GetFullFSPath(HWND parent, const char* fsName, char* path, int pathSize, BOOL& success);
    virtual BOOL WINAPI GetRootPath(char* userPart);

    virtual BOOL WINAPI IsCurrentPath(int currentFSNameIndex, int fsNameIndex, const char* userPart);
    virtual BOOL WINAPI IsOurPath(int currentFSNameIndex, int fsNameIndex, const char* userPart);

    virtual BOOL WINAPI ChangePath(int currentFSNameIndex, char* fsName, int fsNameIndex, const char* userPart,
                                   char* cutFileName, BOOL* pathWasCut, BOOL forceRefresh, int mode);
    virtual BOOL WINAPI ListCurrentPath(CSalamanderDirectoryAbstract* dir,
                                        CPluginDataInterfaceAbstract*& pluginData,
                                        int& iconsType, BOOL forceRefresh);

    virtual BOOL WINAPI TryCloseOrDetach(BOOL forceClose, BOOL canDetach, BOOL& detach, int reason);

    virtual void WINAPI Event(int event, DWORD param) {}

    virtual void WINAPI ReleaseObject(HWND parent) {}

    virtual DWORD WINAPI GetSupportedServices();

    virtual BOOL WINAPI GetChangeDriveOrDisconnectItem(const char* fsName, char*& title, HICON& icon, BOOL& destroyIcon) { return FALSE; }
    virtual HICON WINAPI GetFSIcon(BOOL& destroyIcon);
    virtual void WINAPI GetDropEffect(const char* srcFSPath, const char* tgtFSPath,
                                      DWORD allowedEffects, DWORD keyState,
                                      DWORD* dropEffect) {}
    virtual void WINAPI GetFSFreeSpace(CQuadWord* retValue) { *retValue = CQuadWord(-1, -1); }
    virtual BOOL WINAPI GetNextDirectoryLineHotPath(const char* text, int pathLen, int& offset) { return FALSE; }
    virtual void WINAPI CompleteDirectoryLineHotPath(char* path, int pathBufSize) {}
    virtual BOOL WINAPI GetPathForMainWindowTitle(const char* fsName, int mode, char* buf, int bufSize);
    virtual void WINAPI ShowInfoDialog(const char* fsName, HWND parent) {}
    virtual BOOL WINAPI ExecuteCommandLine(HWND parent, char* command, int& selFrom, int& selTo) { return FALSE; }
    virtual BOOL WINAPI QuickRename(const char* fsName, int mode, HWND parent, CFileData& file, BOOL isDir,
                                    char* newName, BOOL& cancel)
    {
        cancel = TRUE;
        return FALSE;
    }
    virtual void WINAPI AcceptChangeOnPathNotification(const char* fsName, const char* path, BOOL includingSubdirs) {}
    virtual BOOL WINAPI CreateDir(const char* fsName, int mode, HWND parent, char* newName, BOOL& cancel)
    {
        cancel = TRUE;
        return FALSE;
    }
    virtual void WINAPI ViewFile(const char* fsName, HWND parent,
                                 CSalamanderForViewFileOnFSAbstract* salamander,
                                 CFileData& file) {}
    virtual BOOL WINAPI Delete(const char* fsName, int mode, HWND parent, int panel,
                               int selectedFiles, int selectedDirs, BOOL& cancelOrError)
    {
        cancelOrError = TRUE;
        return FALSE;
    }
    virtual BOOL WINAPI CopyOrMoveFromFS(BOOL copy, int mode, const char* fsName, HWND parent,
                                         int panel, int selectedFiles, int selectedDirs,
                                         char* targetPath, BOOL& operationMask,
                                         BOOL& cancelOrHandlePath, HWND dropTarget)
    {
        cancelOrHandlePath = TRUE;
        return FALSE;
    }
    virtual BOOL WINAPI CopyOrMoveFromDiskToFS(BOOL copy, int mode, const char* fsName, HWND parent,
                                               const char* sourcePath, SalEnumSelection2 next,
                                               void* nextParam, int sourceFiles, int sourceDirs,
                                               char* targetPath, BOOL* invalidPathOrCancel)
    {
        if (invalidPathOrCancel != NULL)
            *invalidPathOrCancel = TRUE;
        return FALSE;
    }
    virtual BOOL WINAPI ChangeAttributes(const char* fsName, HWND parent, int panel,
                                         int selectedFiles, int selectedDirs) { return FALSE; }
    virtual void WINAPI ShowProperties(const char* fsName, HWND parent, int panel,
                                       int selectedFiles, int selectedDirs);
    virtual void WINAPI ContextMenu(const char* fsName, HWND parent, int menuX, int menuY, int type,
                                    int panel, int selectedFiles, int selectedDirs) {}
    virtual BOOL WINAPI HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plResult) { return FALSE; }
    virtual BOOL WINAPI OpenFindDialog(const char* fsName, int panel) { return FALSE; }
    virtual void WINAPI OpenActiveFolder(const char* fsName, HWND parent);
    virtual void WINAPI GetAllowedDropEffects(int mode, const char* tgtFSPath, DWORD* allowedEffects) {}
    virtual BOOL WINAPI GetNoItemsInPanelText(char* textBuf, int textBufSize) { return FALSE; }
    virtual void WINAPI ShowSecurityInfo(HWND parent) {}
};
