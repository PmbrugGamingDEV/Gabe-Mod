#ifndef VGUI_GBKVEDITOR_H
#define VGUI_GBKVEDITOR_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/Frame.h"
#include "tier1/utlstring.h"

namespace vgui
{
    class Label;
    class ListPanel;
    class TextEntry;
}

class CGBKVTextEntry;

class CGBKVEditor : public vgui::Frame
{
    DECLARE_CLASS_SIMPLE(CGBKVEditor, vgui::Frame);

    friend class CGBKVTextEntry;

public:
    explicit CGBKVEditor(vgui::VPANEL parent);
    ~CGBKVEditor() override;

    void OnCommand(const char* command) override;
    void OnClose() override;
    void OnThink() override;

private:
    void RefreshFileList();

    bool LoadFile(const char* filename);
    bool SaveFile(const char* filename);
    bool DeleteFile(const char* filename);

    const char* GetSelectedFilename();

    void SetStatus(const char* format, ...);
    void UpdateTitle();
    void UpdateCursorStatus();

    void MarkDirty(bool dirty);

    void InsertTab();
    void InsertNewLineWithIndent();

    void HandleEditorKeyCode(vgui::KeyCode code);

    MESSAGE_FUNC(OnEditorTextChanged, "TextChanged");

private:
    vgui::ListPanel* m_pFileList;
    CGBKVTextEntry* m_pEditor;
    vgui::Label* m_pStatusLabel;
    vgui::Label* m_pCursorLabel;

    CUtlString m_CurrentFilename;

    bool m_bDirty;
    bool m_bLoadingText;

    int m_iLastCursorPosition;
};

#endif // VGUI_GBKVEDITOR_H