#include "cbase.h"
#include "c_vguiscreen.h"
#include <vgui_controls/HTML.h>

class CWebScreenPanel : public CVGuiScreenPanel
{
    DECLARE_CLASS_SIMPLE(CWebScreenPanel, CVGuiScreenPanel);

public:
    CWebScreenPanel(vgui::Panel* parent, const char* name)
        : BaseClass(parent, name)
    {
        m_pHTML = new vgui::HTML(this, "WebBrowser");
        m_pHTML->SetVisible(true);
        m_pHTML->SetEnabled(true);
        m_pHTML->SetMouseInputEnabled(true);
        m_pHTML->SetKeyBoardInputEnabled(true);
    }

    bool Init(KeyValues* pKeyValues,
        VGuiScreenInitData_t* pInitData) override
    {
        if (!BaseClass::Init(pKeyValues, pInitData))
            return false;

        m_pHTML->SetBounds(0, 0, GetWide(), GetTall());

        const char* pszURL = pKeyValues->GetString(
            "url",
            "https://www.youtube.com/watch?v=YE7VzlLtp-4"
        );

        m_pHTML->OpenURL(pszURL, nullptr);

        return true;
    }

    void OnMousePressed(vgui::MouseCode code) override
    {
        Msg("Web screen received mouse press: %d\n", (int)code);

        BaseClass::OnMousePressed(code);
    }

    void PerformLayout() override
    {
        BaseClass::PerformLayout();
        m_pHTML->SetBounds(0, 0, GetWide(), GetTall());
    }

private:
    vgui::HTML* m_pHTML;
};

DECLARE_VGUI_SCREEN_FACTORY(CWebScreenPanel, "vgui_web_panel");