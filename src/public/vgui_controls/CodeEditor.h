//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#pragma once

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/TextEntry.h>

namespace vgui 
{
    class CodeEditor : public TextEntry
    {
        DECLARE_CLASS_SIMPLE(CodeEditor, TextEntry);

    public:
        CodeEditor(Panel* parent, const char* panelName);

        void SetLanguage(const char* language);
        void SetFileName(const char* fileName);

        bool IsModified() const;
        void MarkSaved();                                                 

        void FindText(const char* text);
        void ReplaceText(const char* find, const char* replacement);

    protected:
        virtual void OnKeyCodeTyped(KeyCode code);
        virtual void PaintBackground();
        virtual void Paint();

    private:
        bool m_bModified;
        CUtlString m_Language;
        CUtlString m_FileName;
    };
}