#include "vgui_controls/CodeEditor.h"

#include <vgui/IInput.h>
#include <vgui/ISurface.h>
#include "keyvalues.h"

using namespace vgui;

CodeEditor::CodeEditor(Panel* parent, const char* panelName)
    : BaseClass(parent, panelName)
{
    SetMultiline(true);
    SetCatchEnterKey(true);
    SetAllowNonAsciiCharacters(true);

    m_bModified = false;
}

void CodeEditor::SetLanguage(const char* language)
{
    m_Language = language ? language : "";
    Repaint();
}

void CodeEditor::SetFileName(const char* fileName)
{
    m_FileName = fileName ? fileName : "";
}

bool CodeEditor::IsModified() const
{
    return m_bModified;
}

void CodeEditor::MarkSaved()
{
    m_bModified = false;
}

void CodeEditor::OnKeyCodeTyped(KeyCode code)
{
    bool ctrl =
        input()->IsKeyDown(KEY_LCONTROL) ||
        input()->IsKeyDown(KEY_RCONTROL);

    if (ctrl && code == KEY_S)
    {
        PostActionSignal(
            new KeyValues("SaveCode")
        );

        return;
    }

    if (code == KEY_TAB)
    {
        InsertString("    ");
        return;
    }

    BaseClass::OnKeyCodeTyped(code);
}

void CodeEditor::PaintBackground()
{
    int wide, tall;
    GetSize(wide, tall);

    surface()->DrawSetColor(GetBgColor());
    surface()->DrawFilledRect(0, 0, wide, tall);
}

void CodeEditor::Paint()
{
    BaseClass::Paint();
}

static vgui::Panel* Create_CodeEditor(void) {
    return new CodeEditor(0, 0);
}; static vgui::CBuildFactoryHelper g_CodeEditor_Helper("CodeEditor", Create_CodeEditor); CodeEditor* g_CodeEditorLinkerHack = 0;;