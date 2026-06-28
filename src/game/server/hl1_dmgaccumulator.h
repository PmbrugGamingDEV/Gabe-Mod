#pragma once

#include "cbase.h"
#include "takedamageinfo.h"

class CDmgAccumulator
{
public:
    CDmgAccumulator() {}

    // HL1 accumulated damage storage
    void AccumulateDamage(const CTakeDamageInfo& info)
    {
        m_Info = info;
    }

    const CTakeDamageInfo& GetDamageInfo() const
    {
        return m_Info;
    }

    void Clear()
    {
        m_Info = CTakeDamageInfo();
    }

private:
    CTakeDamageInfo m_Info;
};