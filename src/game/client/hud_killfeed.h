#pragma once

#include "cbase.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include "utlvector.h"

//=========================================================
// DATA
//=========================================================

struct KillFeedLine
{
    wchar_t text[128];
    float timeAdded;
    bool isDamage;

    char weapon[32];
};



class CHudKillFeed : public CHudElement, public vgui::Panel
{
    DECLARE_CLASS_SIMPLE(CHudKillFeed, vgui::Panel);

public:
    CHudKillFeed(const char* pElementName);

    void Init() override;
    void Reset() override;
    void Paint() override;

    // usermessage hook
    void MsgFunc_KillFeed(bf_read& msg);
    void MsgFunc_DamageFeed(bf_read& msg);

private:
    CUtlVector<KillFeedLine> m_Lines;
};
