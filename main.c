#define UNICODE
#define _UNICODE

#include <windows.h>
#include <commdlg.h>
#include <stdlib.h>
#include <wchar.h>

/*
    ============================================================
                         WinPad v0.1
                        Kaiman18 Studios

                    Pure C + Win32 API
    ============================================================
*/

/* ============================================================
   COMMAND IDS
   ============================================================ */

#define ID_FILE_NEW        1001
#define ID_FILE_OPEN       1002
#define ID_FILE_SAVE       1003
#define ID_FILE_SAVEAS     1004
#define ID_FILE_EXIT       1005

#define ID_EDIT_UNDO       1101
#define ID_EDIT_CUT        1102
#define ID_EDIT_COPY       1103
#define ID_EDIT_PASTE      1104
#define ID_EDIT_SELECTALL  1105

#define ID_HELP_ABOUT      1201

#define ID_EDITOR          2001

#define ID_TOOL_NEW        3001
#define ID_TOOL_OPEN       3002
#define ID_TOOL_SAVE       3003


/* ============================================================
   COLORS
   ============================================================ */

#define COLOR_WINDOW_BG    RGB(22, 25, 31)
#define COLOR_TOPBAR       RGB(23, 80, 150)
#define COLOR_EDITOR_BG    RGB(30, 33, 40)
#define COLOR_EDITOR_TEXT  RGB(235, 239, 245)
#define COLOR_STATUS_BG    RGB(18, 21, 27)
#define COLOR_STATUS_TEXT  RGB(170, 180, 195)
#define COLOR_WHITE        RGB(255, 255, 255)


/* ============================================================
   GLOBALS
   ============================================================ */

HWND g_mainWindow = NULL;

HWND g_brandLabel = NULL;

HWND g_btnNew = NULL;
HWND g_btnOpen = NULL;
HWND g_btnSave = NULL;

HWND g_editor = NULL;
HWND g_statusBar = NULL;

HFONT g_brandFont = NULL;
HFONT g_buttonFont = NULL;
HFONT g_editorFont = NULL;
HFONT g_statusFont = NULL;

HBRUSH g_windowBrush = NULL;
HBRUSH g_toolbarBrush = NULL;
HBRUSH g_editorBrush = NULL;
HBRUSH g_statusBrush = NULL;

WCHAR g_currentFile[MAX_PATH] = L"";

BOOL g_modified = FALSE;

const WCHAR WINPAD_CLASS[] =
    L"Kaiman18StudiosWinPad";

const WCHAR WINPAD_VERSION[] =
    L"0.1";


/* ============================================================
   CURRENT FILE NAME
   ============================================================ */

const WCHAR *WinPadGetFileName(void)
{
    WCHAR *slash;

    if (g_currentFile[0] == L'\0')
        return L"Untitled";

    slash = wcsrchr(
        g_currentFile,
        L'\\'
    );

    if (slash != NULL)
        return slash + 1;

    return g_currentFile;
}


/* ============================================================
   TITLE
   ============================================================ */

void WinPadUpdateTitle(void)
{
    WCHAR title[MAX_PATH + 128];

    if (g_modified)
    {
        swprintf(
            title,
            MAX_PATH + 128,
            L"*%ls - WinPad",
            WinPadGetFileName()
        );
    }
    else
    {
        swprintf(
            title,
            MAX_PATH + 128,
            L"%ls - WinPad",
            WinPadGetFileName()
        );
    }

    SetWindowTextW(
        g_mainWindow,
        title
    );
}


/* ============================================================
   WORD COUNT
   ============================================================ */

int WinPadCountWords(void)
{
    int length;
    int words = 0;

    BOOL insideWord = FALSE;

    WCHAR *text;

    length =
        GetWindowTextLengthW(
            g_editor
        );

    if (length <= 0)
        return 0;

    text =
        (WCHAR *)malloc(
            (length + 1) *
            sizeof(WCHAR)
        );

    if (text == NULL)
        return 0;

    GetWindowTextW(
        g_editor,
        text,
        length + 1
    );

    for (int i = 0; i < length; i++)
    {
        WCHAR c = text[i];

        BOOL whitespace =
            (
                c == L' '  ||
                c == L'\t' ||
                c == L'\r' ||
                c == L'\n'
            );

        if (!whitespace)
        {
            if (!insideWord)
            {
                words++;
                insideWord = TRUE;
            }
        }
        else
        {
            insideWord = FALSE;
        }
    }

    free(text);

    return words;
}


/* ============================================================
   STATUS BAR
   ============================================================ */

void WinPadUpdateStatus(void)
{
    DWORD selectionStart = 0;
    DWORD selectionEnd = 0;

    int line;
    int lineStart;
    int column;
    int words;

    WCHAR status[512];

    if (
        g_editor == NULL ||
        g_statusBar == NULL
    )
    {
        return;
    }

    SendMessageW(
        g_editor,
        EM_GETSEL,
        (WPARAM)&selectionStart,
        (LPARAM)&selectionEnd
    );

    line =
        (int)SendMessageW(
            g_editor,
            EM_LINEFROMCHAR,
            selectionStart,
            0
        );

    lineStart =
        (int)SendMessageW(
            g_editor,
            EM_LINEINDEX,
            line,
            0
        );

    column =
        (int)selectionStart -
        lineStart;

    words =
        WinPadCountWords();

    swprintf(
        status,
        512,
        L"  Ln %d   Col %d     |     Words %d     |     UTF-8     |     WinPad v%ls     |     Kaiman18 Studios",
        line + 1,
        column + 1,
        words,
        WINPAD_VERSION
    );

    SetWindowTextW(
        g_statusBar,
        status
    );
}


/* ============================================================
   MODIFIED STATE
   ============================================================ */

void WinPadSetModified(
    BOOL modified
)
{
    g_modified =
        modified;

    WinPadUpdateTitle();
}


/* ============================================================
   WRITE FILE
   ============================================================ */

BOOL WinPadWriteFile(
    LPCWSTR filename
)
{
    int textLength;
    int utf8Length;

    WCHAR *wideText;
    char *utf8Text = NULL;

    HANDLE file;

    BOOL success = TRUE;

    textLength =
        GetWindowTextLengthW(
            g_editor
        );

    wideText =
        (WCHAR *)malloc(
            (textLength + 1) *
            sizeof(WCHAR)
        );

    if (wideText == NULL)
    {
        MessageBoxW(
            g_mainWindow,
            L"Not enough memory to save the file.",
            L"WinPad",
            MB_OK |
            MB_ICONERROR
        );

        return FALSE;
    }

    GetWindowTextW(
        g_editor,
        wideText,
        textLength + 1
    );

    utf8Length =
        WideCharToMultiByte(
            CP_UTF8,
            0,
            wideText,
            textLength,
            NULL,
            0,
            NULL,
            NULL
        );

    if (utf8Length > 0)
    {
        utf8Text =
            (char *)malloc(
                utf8Length
            );

        if (utf8Text == NULL)
        {
            free(wideText);

            return FALSE;
        }

        WideCharToMultiByte(
            CP_UTF8,
            0,
            wideText,
            textLength,
            utf8Text,
            utf8Length,
            NULL,
            NULL
        );
    }

    file =
        CreateFileW(
            filename,
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

    if (file == INVALID_HANDLE_VALUE)
    {
        if (utf8Text != NULL)
            free(utf8Text);

        free(wideText);

        MessageBoxW(
            g_mainWindow,
            L"WinPad could not save this file.",
            L"Save Error",
            MB_OK |
            MB_ICONERROR
        );

        return FALSE;
    }

    if (utf8Length > 0)
    {
        DWORD bytesWritten = 0;

        success =
            WriteFile(
                file,
                utf8Text,
                utf8Length,
                &bytesWritten,
                NULL
            );

        if (
            !success ||
            bytesWritten !=
            (DWORD)utf8Length
        )
        {
            success = FALSE;
        }
    }

    CloseHandle(file);

    if (utf8Text != NULL)
        free(utf8Text);

    free(wideText);

    if (!success)
    {
        MessageBoxW(
            g_mainWindow,
            L"WinPad could not finish writing the file.",
            L"Save Error",
            MB_OK |
            MB_ICONERROR
        );

        return FALSE;
    }

    wcscpy(
        g_currentFile,
        filename
    );

    SendMessageW(
        g_editor,
        EM_SETMODIFY,
        FALSE,
        0
    );

    g_modified = FALSE;

    WinPadUpdateTitle();
    WinPadUpdateStatus();

    return TRUE;
}


/* ============================================================
   SAVE AS
   ============================================================ */

BOOL WinPadSaveAs(void)
{
    OPENFILENAMEW ofn;

    WCHAR filename[MAX_PATH] =
        L"";

    if (g_currentFile[0] != L'\0')
    {
        wcscpy(
            filename,
            g_currentFile
        );
    }

    ZeroMemory(
        &ofn,
        sizeof(ofn)
    );

    ofn.lStructSize =
        sizeof(ofn);

    ofn.hwndOwner =
        g_mainWindow;

    ofn.lpstrFile =
        filename;

    ofn.nMaxFile =
        MAX_PATH;

    ofn.lpstrFilter =
        L"Text Files (*.txt)\0*.txt\0"
        L"C Source Files (*.c)\0*.c\0"
        L"Header Files (*.h)\0*.h\0"
        L"All Files (*.*)\0*.*\0";

    ofn.nFilterIndex = 1;

    ofn.lpstrDefExt =
        L"txt";

    ofn.Flags =
        OFN_OVERWRITEPROMPT |
        OFN_PATHMUSTEXIST;

    if (!GetSaveFileNameW(&ofn))
        return FALSE;

    return WinPadWriteFile(
        filename
    );
}


/* ============================================================
   SAVE
   ============================================================ */

BOOL WinPadSave(void)
{
    if (g_currentFile[0] == L'\0')
    {
        return WinPadSaveAs();
    }

    return WinPadWriteFile(
        g_currentFile
    );
}


/* ============================================================
   CONFIRM UNSAVED CHANGES
   ============================================================ */

BOOL WinPadConfirmChanges(void)
{
    WCHAR message[512];

    int result;

    if (!g_modified)
        return TRUE;

    swprintf(
        message,
        512,
        L"Save changes to %ls?",
        WinPadGetFileName()
    );

    result =
        MessageBoxW(
            g_mainWindow,
            message,
            L"WinPad",
            MB_YESNOCANCEL |
            MB_ICONWARNING
        );

    if (result == IDCANCEL)
        return FALSE;

    if (result == IDNO)
        return TRUE;

    if (result == IDYES)
    {
        return WinPadSave();
    }

    return TRUE;
}


/* ============================================================
   NEW FILE
   ============================================================ */

void WinPadNewFile(void)
{
    if (!WinPadConfirmChanges())
        return;

    SetWindowTextW(
        g_editor,
        L""
    );

    SendMessageW(
        g_editor,
        EM_SETMODIFY,
        FALSE,
        0
    );

    g_currentFile[0] =
        L'\0';

    g_modified =
        FALSE;

    WinPadUpdateTitle();
    WinPadUpdateStatus();

    SetFocus(g_editor);
}


/* ============================================================
   LOAD FILE
   ============================================================ */

BOOL WinPadLoadFile(
    LPCWSTR filename
)
{
    HANDLE file;

    DWORD size;
    DWORD bytesRead = 0;

    char *buffer;
    WCHAR *wideBuffer;

    int wideLength;

    BOOL success;

    file =
        CreateFileW(
            filename,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

    if (file == INVALID_HANDLE_VALUE)
    {
        MessageBoxW(
            g_mainWindow,
            L"WinPad could not open this file.",
            L"Open Error",
            MB_OK |
            MB_ICONERROR
        );

        return FALSE;
    }

    size =
        GetFileSize(
            file,
            NULL
        );

    if (size == INVALID_FILE_SIZE)
    {
        CloseHandle(file);

        return FALSE;
    }

    buffer =
        (char *)malloc(
            size + 1
        );

    if (buffer == NULL)
    {
        CloseHandle(file);

        return FALSE;
    }

    success =
        ReadFile(
            file,
            buffer,
            size,
            &bytesRead,
            NULL
        );

    CloseHandle(file);

    if (!success)
    {
        free(buffer);

        return FALSE;
    }

    buffer[bytesRead] =
        '\0';

    wideLength =
        MultiByteToWideChar(
            CP_UTF8,
            0,
            buffer,
            bytesRead,
            NULL,
            0
        );

    if (
        wideLength == 0 &&
        bytesRead > 0
    )
    {
        free(buffer);

        MessageBoxW(
            g_mainWindow,
            L"WinPad could not decode this file as UTF-8.",
            L"Open Error",
            MB_OK |
            MB_ICONERROR
        );

        return FALSE;
    }

    wideBuffer =
        (WCHAR *)malloc(
            (wideLength + 1) *
            sizeof(WCHAR)
        );

    if (wideBuffer == NULL)
    {
        free(buffer);

        return FALSE;
    }

    if (wideLength > 0)
    {
        MultiByteToWideChar(
            CP_UTF8,
            0,
            buffer,
            bytesRead,
            wideBuffer,
            wideLength
        );
    }

    wideBuffer[wideLength] =
        L'\0';

    SetWindowTextW(
        g_editor,
        wideBuffer
    );

    SendMessageW(
        g_editor,
        EM_SETMODIFY,
        FALSE,
        0
    );

    free(wideBuffer);
    free(buffer);

    wcscpy(
        g_currentFile,
        filename
    );

    g_modified =
        FALSE;

    WinPadUpdateTitle();
    WinPadUpdateStatus();

    SetFocus(g_editor);

    return TRUE;
}


/* ============================================================
   OPEN FILE
   ============================================================ */

void WinPadOpenFile(void)
{
    OPENFILENAMEW ofn;

    WCHAR filename[MAX_PATH] =
        L"";

    if (!WinPadConfirmChanges())
        return;

    ZeroMemory(
        &ofn,
        sizeof(ofn)
    );

    ofn.lStructSize =
        sizeof(ofn);

    ofn.hwndOwner =
        g_mainWindow;

    ofn.lpstrFile =
        filename;

    ofn.nMaxFile =
        MAX_PATH;

    ofn.lpstrFilter =
        L"Text Files (*.txt)\0*.txt\0"
        L"C Source Files (*.c)\0*.c\0"
        L"Header Files (*.h)\0*.h\0"
        L"All Files (*.*)\0*.*\0";

    ofn.nFilterIndex =
        1;

    ofn.Flags =
        OFN_FILEMUSTEXIST |
        OFN_PATHMUSTEXIST |
        OFN_HIDEREADONLY;

    if (GetOpenFileNameW(&ofn))
    {
        WinPadLoadFile(
            filename
        );
    }
}


/* ============================================================
   MENU
   ============================================================ */

HMENU WinPadCreateMenu(void)
{
    HMENU menuBar;
    HMENU fileMenu;
    HMENU editMenu;
    HMENU helpMenu;

    menuBar =
        CreateMenu();

    fileMenu =
        CreatePopupMenu();

    editMenu =
        CreatePopupMenu();

    helpMenu =
        CreatePopupMenu();


    AppendMenuW(
        fileMenu,
        MF_STRING,
        ID_FILE_NEW,
        L"&New\tCtrl+N"
    );

    AppendMenuW(
        fileMenu,
        MF_STRING,
        ID_FILE_OPEN,
        L"&Open...\tCtrl+O"
    );

    AppendMenuW(
        fileMenu,
        MF_STRING,
        ID_FILE_SAVE,
        L"&Save\tCtrl+S"
    );

    AppendMenuW(
        fileMenu,
        MF_STRING,
        ID_FILE_SAVEAS,
        L"Save &As...\tCtrl+Shift+S"
    );

    AppendMenuW(
        fileMenu,
        MF_SEPARATOR,
        0,
        NULL
    );

    AppendMenuW(
        fileMenu,
        MF_STRING,
        ID_FILE_EXIT,
        L"E&xit"
    );


    AppendMenuW(
        editMenu,
        MF_STRING,
        ID_EDIT_UNDO,
        L"&Undo\tCtrl+Z"
    );

    AppendMenuW(
        editMenu,
        MF_SEPARATOR,
        0,
        NULL
    );

    AppendMenuW(
        editMenu,
        MF_STRING,
        ID_EDIT_CUT,
        L"Cu&t\tCtrl+X"
    );

    AppendMenuW(
        editMenu,
        MF_STRING,
        ID_EDIT_COPY,
        L"&Copy\tCtrl+C"
    );

    AppendMenuW(
        editMenu,
        MF_STRING,
        ID_EDIT_PASTE,
        L"&Paste\tCtrl+V"
    );

    AppendMenuW(
        editMenu,
        MF_SEPARATOR,
        0,
        NULL
    );

    AppendMenuW(
        editMenu,
        MF_STRING,
        ID_EDIT_SELECTALL,
        L"Select &All\tCtrl+A"
    );


    AppendMenuW(
        helpMenu,
        MF_STRING,
        ID_HELP_ABOUT,
        L"&About WinPad"
    );


    AppendMenuW(
        menuBar,
        MF_POPUP,
        (UINT_PTR)fileMenu,
        L"&File"
    );

    AppendMenuW(
        menuBar,
        MF_POPUP,
        (UINT_PTR)editMenu,
        L"&Edit"
    );

    AppendMenuW(
        menuBar,
        MF_POPUP,
        (UINT_PTR)helpMenu,
        L"&Help"
    );

    return menuBar;
}


/* ============================================================
   CREATE TOOLBAR CONTROLS

   IMPORTANT:
   These are children of the MAIN WINDOW.

   That means their WM_COMMAND notifications go directly
   to WinPadWindowProc.
   ============================================================ */

void WinPadCreateToolbar(
    HWND parent
)
{
    g_brandLabel =
        CreateWindowExW(
            0,
            L"STATIC",
            L"WinPad",
            WS_CHILD |
            WS_VISIBLE |
            SS_CENTERIMAGE,
            15,
            7,
            120,
            48,
            parent,
            NULL,
            GetModuleHandleW(NULL),
            NULL
        );


    g_btnNew =
        CreateWindowExW(
            0,
            L"BUTTON",
            L"New",
            WS_CHILD |
            WS_VISIBLE |
            BS_PUSHBUTTON,
            155,
            15,
            75,
            32,
            parent,
            (HMENU)(INT_PTR)ID_TOOL_NEW,
            GetModuleHandleW(NULL),
            NULL
        );


    g_btnOpen =
        CreateWindowExW(
            0,
            L"BUTTON",
            L"Open",
            WS_CHILD |
            WS_VISIBLE |
            BS_PUSHBUTTON,
            238,
            15,
            75,
            32,
            parent,
            (HMENU)(INT_PTR)ID_TOOL_OPEN,
            GetModuleHandleW(NULL),
            NULL
        );


    g_btnSave =
        CreateWindowExW(
            0,
            L"BUTTON",
            L"Save",
            WS_CHILD |
            WS_VISIBLE |
            BS_PUSHBUTTON,
            321,
            15,
            75,
            32,
            parent,
            (HMENU)(INT_PTR)ID_TOOL_SAVE,
            GetModuleHandleW(NULL),
            NULL
        );


    SendMessageW(
        g_brandLabel,
        WM_SETFONT,
        (WPARAM)g_brandFont,
        TRUE
    );

    SendMessageW(
        g_btnNew,
        WM_SETFONT,
        (WPARAM)g_buttonFont,
        TRUE
    );

    SendMessageW(
        g_btnOpen,
        WM_SETFONT,
        (WPARAM)g_buttonFont,
        TRUE
    );

    SendMessageW(
        g_btnSave,
        WM_SETFONT,
        (WPARAM)g_buttonFont,
        TRUE
    );
}


/* ============================================================
   LAYOUT
   ============================================================ */

void WinPadLayout(
    HWND hwnd
)
{
    RECT rect;

    int width;
    int height;

    const int toolbarHeight =
        62;

    const int statusHeight =
        30;

    GetClientRect(
        hwnd,
        &rect
    );

    width =
        rect.right -
        rect.left;

    height =
        rect.bottom -
        rect.top;


    if (g_brandLabel != NULL)
    {
        MoveWindow(
            g_brandLabel,
            15,
            7,
            120,
            48,
            TRUE
        );
    }


    if (g_btnNew != NULL)
    {
        MoveWindow(
            g_btnNew,
            155,
            15,
            75,
            32,
            TRUE
        );
    }


    if (g_btnOpen != NULL)
    {
        MoveWindow(
            g_btnOpen,
            238,
            15,
            75,
            32,
            TRUE
        );
    }


    if (g_btnSave != NULL)
    {
        MoveWindow(
            g_btnSave,
            321,
            15,
            75,
            32,
            TRUE
        );
    }


    if (g_statusBar != NULL)
    {
        MoveWindow(
            g_statusBar,
            0,
            height - statusHeight,
            width,
            statusHeight,
            TRUE
        );
    }


    if (g_editor != NULL)
    {
        int editorWidth =
            width - 24;

        int editorHeight =
            height -
            toolbarHeight -
            statusHeight -
            24;

        if (editorWidth < 0)
            editorWidth = 0;

        if (editorHeight < 0)
            editorHeight = 0;

        MoveWindow(
            g_editor,
            12,
            toolbarHeight + 12,
            editorWidth,
            editorHeight,
            TRUE
        );
    }
}


/* ============================================================
   ABOUT
   ============================================================ */

void WinPadAbout(void)
{
    MessageBoxW(
        g_mainWindow,
        L"WinPad v0.1\n\n"
        L"A native Windows text editor.\n\n"
        L"Written in pure C using the Win32 API.\n\n"
        L"A Kaiman18 Studios Application\n\n"
        L"Fast. Native. Lightweight.",
        L"About WinPad",
        MB_OK |
        MB_ICONINFORMATION
    );
}


/* ============================================================
   WINDOW PROCEDURE
   ============================================================ */

LRESULT CALLBACK WinPadWindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (message)
    {
        /* ----------------------------------------------------
           CREATE
           ---------------------------------------------------- */

        case WM_CREATE:
        {
            g_mainWindow =
                hwnd;


            /* Brushes */

            g_windowBrush =
                CreateSolidBrush(
                    COLOR_WINDOW_BG
                );

            g_toolbarBrush =
                CreateSolidBrush(
                    COLOR_TOPBAR
                );

            g_editorBrush =
                CreateSolidBrush(
                    COLOR_EDITOR_BG
                );

            g_statusBrush =
                CreateSolidBrush(
                    COLOR_STATUS_BG
                );


            /* Fonts */

            g_brandFont =
                CreateFontW(
                    -28,
                    0,
                    0,
                    0,
                    FW_BOLD,
                    FALSE,
                    FALSE,
                    FALSE,
                    DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS,
                    CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY,
                    DEFAULT_PITCH |
                    FF_DONTCARE,
                    L"Segoe UI"
                );


            g_buttonFont =
                CreateFontW(
                    -16,
                    0,
                    0,
                    0,
                    FW_SEMIBOLD,
                    FALSE,
                    FALSE,
                    FALSE,
                    DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS,
                    CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY,
                    DEFAULT_PITCH |
                    FF_DONTCARE,
                    L"Segoe UI"
                );


            g_editorFont =
                CreateFontW(
                    -19,
                    0,
                    0,
                    0,
                    FW_NORMAL,
                    FALSE,
                    FALSE,
                    FALSE,
                    DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS,
                    CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY,
                    FIXED_PITCH |
                    FF_MODERN,
                    L"Consolas"
                );


            g_statusFont =
                CreateFontW(
                    -14,
                    0,
                    0,
                    0,
                    FW_NORMAL,
                    FALSE,
                    FALSE,
                    FALSE,
                    DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS,
                    CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY,
                    DEFAULT_PITCH |
                    FF_DONTCARE,
                    L"Segoe UI"
                );


            /* Toolbar */

            WinPadCreateToolbar(
                hwnd
            );


            /* Editor */

            g_editor =
                CreateWindowExW(
                    WS_EX_CLIENTEDGE,
                    L"EDIT",
                    L"",
                    WS_CHILD |
                    WS_VISIBLE |
                    WS_VSCROLL |
                    WS_HSCROLL |
                    ES_LEFT |
                    ES_MULTILINE |
                    ES_AUTOVSCROLL |
                    ES_AUTOHSCROLL |
                    ES_WANTRETURN,
                    0,
                    0,
                    0,
                    0,
                    hwnd,
                    (HMENU)(INT_PTR)ID_EDITOR,
                    GetModuleHandleW(NULL),
                    NULL
                );


            if (g_editor == NULL)
                return -1;


            SendMessageW(
                g_editor,
                WM_SETFONT,
                (WPARAM)g_editorFont,
                TRUE
            );


            SendMessageW(
                g_editor,
                EM_SETMARGINS,
                EC_LEFTMARGIN |
                EC_RIGHTMARGIN,
                MAKELPARAM(
                    12,
                    12
                )
            );


            /* Status bar */

            g_statusBar =
                CreateWindowExW(
                    0,
                    L"STATIC",
                    L"",
                    WS_CHILD |
                    WS_VISIBLE |
                    SS_LEFT |
                    SS_CENTERIMAGE,
                    0,
                    0,
                    0,
                    0,
                    hwnd,
                    NULL,
                    GetModuleHandleW(NULL),
                    NULL
                );


            SendMessageW(
                g_statusBar,
                WM_SETFONT,
                (WPARAM)g_statusFont,
                TRUE
            );


            WinPadLayout(
                hwnd
            );

            WinPadUpdateTitle();
            WinPadUpdateStatus();

            SetFocus(
                g_editor
            );

            return 0;
        }


        /* ----------------------------------------------------
           RESIZE
           ---------------------------------------------------- */

        case WM_SIZE:
        {
            WinPadLayout(
                hwnd
            );

            InvalidateRect(
                hwnd,
                NULL,
                TRUE
            );

            return 0;
        }


        /* ----------------------------------------------------
           PAINT BACKGROUND + BLUE TOOLBAR

           No STATIC container behind the buttons.
           We paint the toolbar directly.
           ---------------------------------------------------- */

        case WM_PAINT:
        {
            PAINTSTRUCT ps;

            HDC hdc;

            RECT clientRect;
            RECT toolbarRect;
            RECT bodyRect;

            hdc =
                BeginPaint(
                    hwnd,
                    &ps
                );

            GetClientRect(
                hwnd,
                &clientRect
            );


            toolbarRect.left =
                0;

            toolbarRect.top =
                0;

            toolbarRect.right =
                clientRect.right;

            toolbarRect.bottom =
                62;


            FillRect(
                hdc,
                &toolbarRect,
                g_toolbarBrush
            );


            bodyRect.left =
                0;

            bodyRect.top =
                62;

            bodyRect.right =
                clientRect.right;

            bodyRect.bottom =
                clientRect.bottom;


            FillRect(
                hdc,
                &bodyRect,
                g_windowBrush
            );


            EndPaint(
                hwnd,
                &ps
            );

            return 0;
        }


        /* ----------------------------------------------------
           STATIC CONTROL COLORS
           ---------------------------------------------------- */

        case WM_CTLCOLORSTATIC:
        {
            HDC hdc =
                (HDC)wParam;

            HWND control =
                (HWND)lParam;


            if (control == g_brandLabel)
            {
                SetTextColor(
                    hdc,
                    COLOR_WHITE
                );

                SetBkColor(
                    hdc,
                    COLOR_TOPBAR
                );

                return
                    (INT_PTR)g_toolbarBrush;
            }


            if (control == g_statusBar)
            {
                SetTextColor(
                    hdc,
                    COLOR_STATUS_TEXT
                );

                SetBkColor(
                    hdc,
                    COLOR_STATUS_BG
                );

                return
                    (INT_PTR)g_statusBrush;
            }

            break;
        }


        /* ----------------------------------------------------
           EDITOR COLORS
           ---------------------------------------------------- */

        case WM_CTLCOLOREDIT:
        {
            HDC hdc =
                (HDC)wParam;

            SetTextColor(
                hdc,
                COLOR_EDITOR_TEXT
            );

            SetBkColor(
                hdc,
                COLOR_EDITOR_BG
            );

            return
                (INT_PTR)g_editorBrush;
        }


        /* ----------------------------------------------------
           COMMANDS
           ---------------------------------------------------- */

        case WM_COMMAND:
        {
            int command =
                LOWORD(wParam);


            /* Editor changed */

            if (
                command == ID_EDITOR &&
                HIWORD(wParam) ==
                EN_CHANGE
            )
            {
                BOOL modified =
                    (BOOL)SendMessageW(
                        g_editor,
                        EM_GETMODIFY,
                        0,
                        0
                    );

                WinPadSetModified(
                    modified
                );

                WinPadUpdateStatus();

                return 0;
            }


            switch (command)
            {
                /* Toolbar + File */

                case ID_TOOL_NEW:
                case ID_FILE_NEW:
                    WinPadNewFile();
                    return 0;


                case ID_TOOL_OPEN:
                case ID_FILE_OPEN:
                    WinPadOpenFile();
                    return 0;


                case ID_TOOL_SAVE:
                case ID_FILE_SAVE:
                    WinPadSave();
                    return 0;


                case ID_FILE_SAVEAS:
                    WinPadSaveAs();
                    return 0;


                case ID_FILE_EXIT:
                    SendMessageW(
                        hwnd,
                        WM_CLOSE,
                        0,
                        0
                    );

                    return 0;


                /* Edit */

                case ID_EDIT_UNDO:
                    SendMessageW(
                        g_editor,
                        WM_UNDO,
                        0,
                        0
                    );

                    return 0;


                case ID_EDIT_CUT:
                    SendMessageW(
                        g_editor,
                        WM_CUT,
                        0,
                        0
                    );

                    return 0;


                case ID_EDIT_COPY:
                    SendMessageW(
                        g_editor,
                        WM_COPY,
                        0,
                        0
                    );

                    return 0;


                case ID_EDIT_PASTE:
                    SendMessageW(
                        g_editor,
                        WM_PASTE,
                        0,
                        0
                    );

                    return 0;


                case ID_EDIT_SELECTALL:
                    SendMessageW(
                        g_editor,
                        EM_SETSEL,
                        0,
                        -1
                    );

                    return 0;


                /* Help */

                case ID_HELP_ABOUT:
                    WinPadAbout();

                    return 0;
            }

            break;
        }


        /* ----------------------------------------------------
           CLOSE
           ---------------------------------------------------- */

        case WM_CLOSE:
        {
            if (!WinPadConfirmChanges())
                return 0;

            DestroyWindow(
                hwnd
            );

            return 0;
        }


        /* ----------------------------------------------------
           DESTROY
           ---------------------------------------------------- */

        case WM_DESTROY:
        {
            if (g_brandFont != NULL)
                DeleteObject(g_brandFont);

            if (g_buttonFont != NULL)
                DeleteObject(g_buttonFont);

            if (g_editorFont != NULL)
                DeleteObject(g_editorFont);

            if (g_statusFont != NULL)
                DeleteObject(g_statusFont);

            if (g_windowBrush != NULL)
                DeleteObject(g_windowBrush);

            if (g_toolbarBrush != NULL)
                DeleteObject(g_toolbarBrush);

            if (g_editorBrush != NULL)
                DeleteObject(g_editorBrush);

            if (g_statusBrush != NULL)
                DeleteObject(g_statusBrush);

            PostQuitMessage(0);

            return 0;
        }
    }


    return DefWindowProcW(
        hwnd,
        message,
        wParam,
        lParam
    );
}


/* ============================================================
   WINMAIN
   ============================================================ */

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
)
{
    WNDCLASSEXW wc;

    HWND hwnd;

    HACCEL acceleratorTable;

    MSG msg;


    ZeroMemory(
        &wc,
        sizeof(wc)
    );


    wc.cbSize =
        sizeof(WNDCLASSEXW);

    wc.style =
        CS_HREDRAW |
        CS_VREDRAW;

    wc.lpfnWndProc =
        WinPadWindowProc;

    wc.hInstance =
        hInstance;

    wc.hCursor =
        LoadCursorW(
            NULL,
            IDC_ARROW
        );

    wc.hbrBackground =
        (HBRUSH)(
            COLOR_WINDOW + 1
        );

    wc.lpszClassName =
        WINPAD_CLASS;

    wc.hIcon =
        LoadIconW(
            NULL,
            IDI_APPLICATION
        );

    wc.hIconSm =
        LoadIconW(
            NULL,
            IDI_APPLICATION
        );


    if (!RegisterClassExW(&wc))
    {
        MessageBoxW(
            NULL,
            L"WinPad could not register its window class.",
            L"WinPad Error",
            MB_OK |
            MB_ICONERROR
        );

        return 1;
    }


    hwnd =
        CreateWindowExW(
            0,
            WINPAD_CLASS,
            L"WinPad",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            1050,
            720,
            NULL,
            WinPadCreateMenu(),
            hInstance,
            NULL
        );


    if (hwnd == NULL)
    {
        MessageBoxW(
            NULL,
            L"WinPad could not create its main window.",
            L"WinPad Error",
            MB_OK |
            MB_ICONERROR
        );

        return 1;
    }


    ShowWindow(
        hwnd,
        nCmdShow
    );

    UpdateWindow(
        hwnd
    );


    /*
        Keyboard shortcuts
    */

    ACCEL accelerators[] =
    {
        {
            FCONTROL |
            FVIRTKEY,
            'N',
            ID_FILE_NEW
        },

        {
            FCONTROL |
            FVIRTKEY,
            'O',
            ID_FILE_OPEN
        },

        {
            FCONTROL |
            FVIRTKEY,
            'S',
            ID_FILE_SAVE
        },

        {
            FCONTROL |
            FSHIFT |
            FVIRTKEY,
            'S',
            ID_FILE_SAVEAS
        }
    };


    acceleratorTable =
        CreateAcceleratorTableW(
            accelerators,
            4
        );


    while (
        GetMessageW(
            &msg,
            NULL,
            0,
            0
        ) > 0
    )
    {
        if (
            acceleratorTable == NULL ||
            !TranslateAcceleratorW(
                hwnd,
                acceleratorTable,
                &msg
            )
        )
        {
            TranslateMessage(
                &msg
            );

            DispatchMessageW(
                &msg
            );
        }
    }


    if (acceleratorTable != NULL)
    {
        DestroyAcceleratorTable(
            acceleratorTable
        );
    }


    return
        (int)msg.wParam;
}