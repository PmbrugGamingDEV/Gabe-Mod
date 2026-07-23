#include "cbase.h"
#include "vgui_keyvalue.h"

#include "filesystem.h"
#include "ienginevgui.h"

#include "tier1/utlstring.h"
#include "tier1/utlvector.h"

#include "vgui/IInput.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/ListPanel.h"
#include "vgui_controls/TextEntry.h"

#include "vgui/KeyCode.h"

#include <stdarg.h>

//-----------------------------------------------------------------------------
// Editor constants
//-----------------------------------------------------------------------------

namespace
{
    const int EDITOR_WIDTH = 960;
    const int EDITOR_HEIGHT = 680;

    const unsigned int MAX_EDITOR_FILE_SIZE = 4 * 1024 * 1024;
    const int MAX_DIRECTORY_DEPTH = 32;
    const int TAB_SIZE = 4;

    //-------------------------------------------------------------------------
    // Returns true when the file is a format that the editor should display.
    //-------------------------------------------------------------------------

    bool IsEditableExtension(const char* filename)
    {
        if (!filename || !filename[0])
            return false;

        const char* extension = Q_strrchr(filename, '.');

        if (!extension)
            return false;

        return
            !Q_stricmp(extension, ".txt") ||
            !Q_stricmp(extension, ".res") ||
            !Q_stricmp(extension, ".vdf") ||
            !Q_stricmp(extension, ".kv") ||
            !Q_stricmp(extension, ".cfg");
    }

    //-------------------------------------------------------------------------
    // Recursively searches the MOD path for editable files.
    //-------------------------------------------------------------------------

    void RecursiveSearch(
        const char* path,
        CUtlVector<CUtlString>& results,
        int depth = 0)
    {
        if (!path || !path[0])
            return;

        if (depth > MAX_DIRECTORY_DEPTH)
        {
            Warning(
                "CGBKVEditor: Maximum directory depth reached at \"%s\".\n",
                path);

            return;
        }

        char searchPath[MAX_PATH];
        Q_snprintf(searchPath, sizeof(searchPath), "%s/*", path);

        FileFindHandle_t findHandle = FILESYSTEM_INVALID_FIND_HANDLE;

        const char* filename =
            filesystem->FindFirstEx(searchPath, "MOD", &findHandle);

        if (!filename)
            return;

        do
        {
            if (!Q_stricmp(filename, ".") ||
                !Q_stricmp(filename, ".."))
            {
                continue;
            }

            char fullPath[MAX_PATH];

            if (!Q_stricmp(path, "."))
            {
                Q_snprintf(
                    fullPath,
                    sizeof(fullPath),
                    "%s",
                    filename);
            }
            else
            {
                Q_snprintf(
                    fullPath,
                    sizeof(fullPath),
                    "%s/%s",
                    path,
                    filename);
            }

            V_FixSlashes(fullPath, '/');

            if (filesystem->FindIsDirectory(findHandle))
            {
                RecursiveSearch(fullPath, results, depth + 1);
            }
            else if (IsEditableExtension(filename))
            {
                results.AddToTail(CUtlString(fullPath));
            }
        } while ((filename = filesystem->FindNext(findHandle)) != NULL);

        filesystem->FindClose(findHandle);
    }
}

//-----------------------------------------------------------------------------
// Custom TextEntry
//
// TextEntry normally consumes keyboard input itself. This subclass lets the
// editor frame handle special keys such as Tab, Enter, and Ctrl+S.
//-----------------------------------------------------------------------------

class CGBKVTextEntry : public vgui::TextEntry
{
    DECLARE_CLASS_SIMPLE(CGBKVTextEntry, vgui::TextEntry);

public:
    CGBKVTextEntry(
        CGBKVEditor* editor,
        vgui::Panel* parent,
        const char* panelName)
        : BaseClass(parent, panelName),
        m_pEditorWindow(editor)
    {}

    int GetCaretIndex() const
    {
        return GetCursorPosition();
    }

    void OnKeyCodeTyped(vgui::KeyCode code) override
    {
        if (m_pEditorWindow)
        {
            const bool controlDown =
                vgui::input()->IsKeyDown(vgui::KeyCode::KEY_LCONTROL) ||
                vgui::input()->IsKeyDown(vgui::KeyCode::KEY_RCONTROL);

            if (controlDown)
            {
                switch (code)
                {
                case vgui::KeyCode::KEY_S:
                case vgui::KeyCode::KEY_O:
                case vgui::KeyCode::KEY_A:
                case vgui::KeyCode::KEY_C:
                case vgui::KeyCode::KEY_X:
                case vgui::KeyCode::KEY_V:
                    m_pEditorWindow->HandleEditorKeyCode(code);
                    return;
                }
            }

            if (code == vgui::KeyCode::KEY_TAB ||
                code == vgui::KeyCode::KEY_ENTER)
            {
                m_pEditorWindow->HandleEditorKeyCode(code);
                return;
            }
        }

        BaseClass::OnKeyCodeTyped(code);
    }

private:
    CGBKVEditor* m_pEditorWindow;
};

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------

CGBKVEditor::CGBKVEditor(vgui::VPANEL parent)
    : BaseClass(NULL, "GBKVEditor"),
    m_pFileList(NULL),
    m_pEditor(NULL),
    m_pStatusLabel(NULL),
    m_pCursorLabel(NULL),
    m_bDirty(false),
    m_bLoadingText(false),
    m_iLastCursorPosition(-1)
{
    SetParent(parent);

    SetSize(EDITOR_WIDTH, EDITOR_HEIGHT);
    SetMinimumSize(EDITOR_WIDTH, EDITOR_HEIGHT);

    SetTitle("Gabe KeyValues Editor", true);

    SetSizeable(false);
    SetMoveable(true);

    SetMinimizeButtonVisible(true);
    SetMaximizeButtonVisible(false);
    SetCloseButtonVisible(true);

    // Keep the global editor pointer valid when the window is closed.
    SetDeleteSelfOnClose(false);

    //-------------------------------------------------------------------------
    // File list
    //-------------------------------------------------------------------------

    m_pFileList = new vgui::ListPanel(this, "FileList");

    m_pFileList->SetPos(10, 35);
    m_pFileList->SetSize(280, 570);

    m_pFileList->AddColumnHeader(
        0,
        "filename",
        "Filename",
        265,
        0);

    //-------------------------------------------------------------------------
    // Text editor
    //-------------------------------------------------------------------------

    m_pEditor = new CGBKVTextEntry(
        this,
        this,
        "Editor");

    m_pEditor->SetMultiline(true);
    m_pEditor->SetCatchEnterKey(true);
    m_pEditor->SetAllowNonAsciiCharacters(true);

    m_pEditor->SetPos(300, 35);
    m_pEditor->SetSize(650, 570);

    // Receive the TextChanged action signal.
    m_pEditor->AddActionSignalTarget(this);

    //-------------------------------------------------------------------------
    // Buttons
    //-------------------------------------------------------------------------

    vgui::Button* openButton =
        new vgui::Button(
            this,
            "OpenButton",
            "Open",
            this,
            "open");

    openButton->SetPos(10, 615);
    openButton->SetSize(70, 24);

    vgui::Button* saveButton =
        new vgui::Button(
            this,
            "SaveButton",
            "Save",
            this,
            "save");

    saveButton->SetPos(85, 615);
    saveButton->SetSize(70, 24);

    vgui::Button* deleteButton =
        new vgui::Button(
            this,
            "DeleteButton",
            "Delete",
            this,
            "delete");

    deleteButton->SetPos(160, 615);
    deleteButton->SetSize(70, 24);

    vgui::Button* refreshButton =
        new vgui::Button(
            this,
            "RefreshButton",
            "Refresh",
            this,
            "refresh");

    refreshButton->SetPos(235, 615);
    refreshButton->SetSize(70, 24);

    vgui::Button* clearButton =
        new vgui::Button(
            this,
            "ClearButton",
            "Clear",
            this,
            "clear");

    clearButton->SetPos(310, 615);
    clearButton->SetSize(70, 24);

    //-------------------------------------------------------------------------
    // Status labels
    //-------------------------------------------------------------------------

    m_pStatusLabel = new vgui::Label(
        this,
        "StatusLabel",
        "Ready.");

    m_pStatusLabel->SetPos(395, 615);
    m_pStatusLabel->SetSize(400, 24);

    m_pCursorLabel = new vgui::Label(
        this,
        "CursorLabel",
        "Line 1, Column 1");

    m_pCursorLabel->SetPos(800, 615);
    m_pCursorLabel->SetSize(150, 24);

    m_pCursorLabel->SetContentAlignment(
        vgui::Label::a_east);

    RefreshFileList();
    UpdateTitle();
    UpdateCursorStatus();

    SetVisible(true);
    MoveToCenterOfScreen();
    MoveToFront();
    RequestFocus();
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------

CGBKVEditor::~CGBKVEditor()
{}

//-----------------------------------------------------------------------------
// Close the window without deleting it.
//-----------------------------------------------------------------------------

void CGBKVEditor::OnClose()
{
    SetVisible(false);
}

//-----------------------------------------------------------------------------
// Called once per frame.
//-----------------------------------------------------------------------------

void CGBKVEditor::OnThink()
{
    BaseClass::OnThink();

    if (!m_pEditor)
        return;

    const int cursorPosition = m_pEditor->GetCaretIndex();

    if (cursorPosition != m_iLastCursorPosition)
    {
        m_iLastCursorPosition = cursorPosition;
        UpdateCursorStatus();
    }
}

//-----------------------------------------------------------------------------
// Set the status bar text.
//-----------------------------------------------------------------------------

void CGBKVEditor::SetStatus(const char* format, ...)
{
    if (!m_pStatusLabel || !format)
        return;

    char message[512];

    va_list args;
    va_start(args, format);

    Q_vsnprintf(
        message,
        sizeof(message),
        format,
        args);

    va_end(args);

    m_pStatusLabel->SetText(message);
}

//-----------------------------------------------------------------------------
// Update the window title.
//-----------------------------------------------------------------------------

void CGBKVEditor::UpdateTitle()
{
    char title[512];

    const char* filename = m_CurrentFilename.IsEmpty()
        ? "Untitled"
        : m_CurrentFilename.String();

    Q_snprintf(
        title,
        sizeof(title),
        "Gabe KeyValues Editor - %s%s",
        filename,
        m_bDirty ? " *" : "");

    SetTitle(title, true);
}

//-----------------------------------------------------------------------------
// Mark the document as modified or saved.
//-----------------------------------------------------------------------------

void CGBKVEditor::MarkDirty(bool dirty)
{
    if (m_bDirty == dirty)
        return;

    m_bDirty = dirty;

    UpdateTitle();
}

//-----------------------------------------------------------------------------
// Receive TextEntry's TextChanged action signal.
//-----------------------------------------------------------------------------

void CGBKVEditor::OnEditorTextChanged()
{
    if (m_bLoadingText)
        return;

    MarkDirty(true);
}

//-----------------------------------------------------------------------------
// Update the current line and column label.
//-----------------------------------------------------------------------------

void CGBKVEditor::UpdateCursorStatus()
{
    if (!m_pEditor || !m_pCursorLabel)
        return;

    const int textLength = m_pEditor->GetTextLength();
    const int cursorPosition = m_pEditor->GetCaretIndex();

    int line = 1;
    int column = 1;

    if (textLength > 0)
    {
        char* text = new char[textLength + 1];

        m_pEditor->GetText(
            text,
            textLength + 1);

        const int safeCursorPosition =
            clamp(cursorPosition, 0, textLength);

        for (int i = 0; i < safeCursorPosition; ++i)
        {
            if (text[i] == '\n')
            {
                ++line;
                column = 1;
            }
            else
            {
                ++column;
            }
        }

        delete[] text;
    }

    char cursorText[128];

    Q_snprintf(
        cursorText,
        sizeof(cursorText),
        "Line %d, Column %d",
        line,
        column);

    m_pCursorLabel->SetText(cursorText);
}

//-----------------------------------------------------------------------------
// Get the currently selected file.
//-----------------------------------------------------------------------------

const char* CGBKVEditor::GetSelectedFilename()
{
    if (!m_pFileList)
        return NULL;

    const int selectedItem =
        m_pFileList->GetSelectedItem(0);

    if (selectedItem < 0)
        return NULL;

    KeyValues* itemData =
        m_pFileList->GetItem(selectedItem);

    if (!itemData)
        return NULL;

    const char* filename =
        itemData->GetString("filename", "");

    if (!filename[0])
        return NULL;

    return filename;
}

//-----------------------------------------------------------------------------
// Refresh the file list.
//-----------------------------------------------------------------------------

void CGBKVEditor::RefreshFileList()
{
    if (!m_pFileList)
        return;

    m_pFileList->RemoveAll();

    CUtlVector<CUtlString> files;

    RecursiveSearch(".", files);

    for (int i = 0; i < files.Count(); ++i)
    {
        KeyValues* row =
            new KeyValues("FileRow");

        row->SetString(
            "filename",
            files[i].String());

        m_pFileList->AddItem(
            row,
            i,
            false,
            false);

        row->deleteThis();
    }

    SetStatus(
        "Found %d editable file(s).",
        files.Count());
}

//-----------------------------------------------------------------------------
// Load a file into the editor.
//-----------------------------------------------------------------------------

bool CGBKVEditor::LoadFile(const char* filename)
{
    if (!filename || !filename[0])
    {
        SetStatus("No filename was selected.");
        return false;
    }

    FileHandle_t file =
        filesystem->Open(
            filename,
            "rb",
            "MOD");

    if (file == FILESYSTEM_INVALID_HANDLE)
    {
        Warning(
            "CGBKVEditor: Could not open \"%s\".\n",
            filename);

        SetStatus(
            "Failed to open %s.",
            filename);

        return false;
    }

    const unsigned int fileSize =
        filesystem->Size(file);

    if (fileSize > MAX_EDITOR_FILE_SIZE)
    {
        filesystem->Close(file);

        Warning(
            "CGBKVEditor: \"%s\" is too large to edit (%u bytes).\n",
            filename,
            fileSize);

        SetStatus(
            "File is too large to edit.");

        return false;
    }

    char* buffer =
        new char[fileSize + 1];

    const int bytesRead =
        filesystem->Read(
            buffer,
            fileSize,
            file);

    filesystem->Close(file);

    if (bytesRead < 0)
    {
        delete[] buffer;

        Warning(
            "CGBKVEditor: Could not read \"%s\".\n",
            filename);

        SetStatus(
            "Failed to read %s.",
            filename);

        return false;
    }

    buffer[bytesRead] = '\0';

    m_bLoadingText = true;

    m_pEditor->SetText(buffer);
    m_pEditor->GotoTextStart();

    m_bLoadingText = false;

    delete[] buffer;

    m_CurrentFilename = filename;

    MarkDirty(false);
    UpdateTitle();
    UpdateCursorStatus();

    SetStatus(
        "Loaded %s.",
        filename);

    return true;
}

//-----------------------------------------------------------------------------
// Save the editor contents.
//-----------------------------------------------------------------------------

bool CGBKVEditor::SaveFile(const char* filename)
{
    if (!filename || !filename[0])
    {
        SetStatus("No filename was selected.");
        return false;
    }

    const int textLength =
        m_pEditor->GetTextLength();

    if (textLength < 0)
    {
        SetStatus(
            "Could not determine the text length.");

        return false;
    }

    char* buffer =
        new char[textLength + 1];

    m_pEditor->GetText(
        buffer,
        textLength + 1);

    FileHandle_t file =
        filesystem->Open(
            filename,
            "wb",
            "MOD");

    if (file == FILESYSTEM_INVALID_HANDLE)
    {
        delete[] buffer;

        Warning(
            "CGBKVEditor: Could not save \"%s\".\n",
            filename);

        SetStatus(
            "Failed to open %s for writing.",
            filename);

        return false;
    }

    const int bytesToWrite =
        Q_strlen(buffer);

    const int bytesWritten =
        filesystem->Write(
            buffer,
            bytesToWrite,
            file);

    filesystem->Close(file);

    delete[] buffer;

    if (bytesWritten != bytesToWrite)
    {
        Warning(
            "CGBKVEditor: Wrote %d of %d bytes to \"%s\".\n",
            bytesWritten,
            bytesToWrite,
            filename);

        SetStatus(
            "The file may not have saved completely.");

        return false;
    }

    m_CurrentFilename = filename;

    MarkDirty(false);
    UpdateTitle();

    SetStatus(
        "Saved %s.",
        filename);

    return true;
}

//-----------------------------------------------------------------------------
// Delete a file.
//-----------------------------------------------------------------------------

bool CGBKVEditor::DeleteFile(const char* filename)
{
    if (!filename || !filename[0])
    {
        SetStatus("No filename was selected.");
        return false;
    }

    if (!filesystem->FileExists(filename, "MOD"))
    {
        SetStatus(
            "%s no longer exists.",
            filename);

        RefreshFileList();
        return false;
    }

    filesystem->RemoveFile(
        filename,
        "MOD");

    if (filesystem->FileExists(filename, "MOD"))
    {
        Warning(
            "CGBKVEditor: Could not delete \"%s\".\n",
            filename);

        SetStatus(
            "Failed to delete %s.",
            filename);

        return false;
    }

    if (!Q_stricmp(
        m_CurrentFilename.String(),
        filename))
    {
        m_bLoadingText = true;

        m_pEditor->SetText("");

        m_bLoadingText = false;

        m_CurrentFilename.Clear();

        MarkDirty(false);
        UpdateTitle();
        UpdateCursorStatus();
    }

    RefreshFileList();

    SetStatus(
        "Deleted %s.",
        filename);

    return true;
}

//-----------------------------------------------------------------------------
// Insert four spaces.
//-----------------------------------------------------------------------------

void CGBKVEditor::InsertTab()
{
    if (!m_pEditor)
        return;

    char spaces[TAB_SIZE + 1];

    for (int i = 0; i < TAB_SIZE; ++i)
        spaces[i] = ' ';

    spaces[TAB_SIZE] = '\0';

    m_pEditor->InsertString(spaces);
}

//-----------------------------------------------------------------------------
// Insert a newline with indentation copied from the current line.
//
// If the previous non-whitespace character is an opening brace, another level
// of indentation is added.
//-----------------------------------------------------------------------------

void CGBKVEditor::InsertNewLineWithIndent()
{
    if (!m_pEditor)
        return;

    const int textLength =
        m_pEditor->GetTextLength();

    const int cursorPosition =
        m_pEditor->GetCaretIndex();

    if (textLength <= 0 || cursorPosition <= 0)
    {
        m_pEditor->InsertString("\n");
        return;
    }

    char* text =
        new char[textLength + 1];

    m_pEditor->GetText(
        text,
        textLength + 1);

    const int safeCursorPosition =
        clamp(cursorPosition, 0, textLength);

    int lineStart = safeCursorPosition;

    while (lineStart > 0)
    {
        if (text[lineStart - 1] == '\n')
            break;

        --lineStart;
    }

    char indentation[256];
    int indentationLength = 0;

    int position = lineStart;

    while (position < safeCursorPosition &&
        indentationLength < sizeof(indentation) - 1)
    {
        const char character = text[position];

        if (character != ' ' &&
            character != '\t')
        {
            break;
        }

        indentation[indentationLength] = character;

        ++indentationLength;
        ++position;
    }

    indentation[indentationLength] = '\0';

    bool addExtraIndent = false;

    for (int i = safeCursorPosition - 1; i >= 0; --i)
    {
        const char character = text[i];

        if (character == ' ' ||
            character == '\t' ||
            character == '\r')
        {
            continue;
        }

        if (character == '{')
            addExtraIndent = true;

        break;
    }

    delete[] text;

    CUtlString insertion;
    insertion = "\n";
    insertion += indentation;

    if (addExtraIndent)
        insertion += "    ";

    m_pEditor->InsertString(
        insertion.String());
}

//-----------------------------------------------------------------------------
// Handle keyboard shortcuts from the custom TextEntry.
//-----------------------------------------------------------------------------

void CGBKVEditor::HandleEditorKeyCode(vgui::KeyCode code)
{
    if (!m_pEditor)
        return;

    const bool controlDown =
        vgui::input()->IsKeyDown(vgui::KeyCode::KEY_LCONTROL) ||
        vgui::input()->IsKeyDown(vgui::KeyCode::KEY_RCONTROL);

    if (controlDown)
    {
        switch (code)
        {
        case vgui::KeyCode::KEY_S:
            OnCommand("save");
            return;

        case vgui::KeyCode::KEY_O:
            OnCommand("open");
            return;

        case vgui::KeyCode::KEY_A:
            m_pEditor->SelectAllText(true);
            return;

        case vgui::KeyCode::KEY_C:
            m_pEditor->CopySelected();
            return;

        case vgui::KeyCode::KEY_X:
            m_pEditor->CutSelected();
            return;

        case vgui::KeyCode::KEY_V:
            m_pEditor->Paste();
            return;
        }
    }

    if (code == vgui::KeyCode::KEY_TAB)
    {
        InsertTab();
        return;
    }

    if (code == vgui::KeyCode::KEY_ENTER)
    {
        InsertNewLineWithIndent();
        return;
    }
}

//-----------------------------------------------------------------------------
// Handle button commands.
//-----------------------------------------------------------------------------

void CGBKVEditor::OnCommand(const char* command)
{
    if (!command)
    {
        BaseClass::OnCommand(command);
        return;
    }

    if (!Q_stricmp(command, "open"))
    {
        const char* filename =
            GetSelectedFilename();

        if (!filename)
        {
            SetStatus(
                "Select a file to open.");

            return;
        }

        LoadFile(filename);
    }
    else if (!Q_stricmp(command, "save"))
    {
        const char* filename = NULL;

        if (!m_CurrentFilename.IsEmpty())
        {
            filename =
                m_CurrentFilename.String();
        }
        else
        {
            filename =
                GetSelectedFilename();
        }

        if (!filename)
        {
            SetStatus(
                "Open or select a file before saving.");

            return;
        }

        SaveFile(filename);
    }
    else if (!Q_stricmp(command, "delete"))
    {
        const char* filename =
            GetSelectedFilename();

        if (!filename)
        {
            SetStatus(
                "Select a file to delete.");

            return;
        }

        DeleteFile(filename);
    }
    else if (!Q_stricmp(command, "refresh"))
    {
        RefreshFileList();
    }
    else if (!Q_stricmp(command, "clear"))
    {
        m_bLoadingText = true;

        m_pEditor->SetText("");

        m_bLoadingText = false;

        MarkDirty(true);
        UpdateCursorStatus();

        SetStatus(
            "Editor cleared. The file has not been saved.");
    }
    else
    {
        BaseClass::OnCommand(command);
    }
}

//-----------------------------------------------------------------------------
// Global editor instance
//-----------------------------------------------------------------------------

static CGBKVEditor* g_pKVEditor = NULL;

//-----------------------------------------------------------------------------
// Console command
//-----------------------------------------------------------------------------

CON_COMMAND(
    gabe_kveditor,
    "Opens the Gabe KeyValues Editor.")
{
    if (!g_pKVEditor)
    {
        vgui::VPANEL gameUIPanel =
            enginevgui->GetPanel(PANEL_GAMEUIDLL);

        g_pKVEditor =
            new CGBKVEditor(gameUIPanel);
    }

    g_pKVEditor->SetVisible(true);
    g_pKVEditor->MoveToFront();
    g_pKVEditor->RequestFocus();
}