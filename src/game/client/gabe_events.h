#ifndef GABE_EVENTS_H
#define GABE_EVENTS_H

#include "cbase.h"

enum GabeHoliday
{
    HOLIDAY_NONE = 0,
    HOLIDAY_NEWYEAR, // January 1st
    HOLIDAY_VALENTINE, // February 14th
    HOLIDAY_STPATRICK, // March 17th
    HOLIDAY_EASTER, // Between March 22nd and April 25th (varies each year)
    HOLIDAY_MEMORIAL, // Last Monday in May
    HOLIDAY_JULY4, // July 4th
    HOLIDAY_LABOR, // First Monday in September
    HOLIDAY_HALLOWEEN, // October 31st
    HOLIDAY_THANKSGIVING, // Fourth Thursday in November
    HOLIDAY_CHRISTMAS, // December 25th
    HOLIDAY_GABEMODANNIVERSARY // July 25th (Gabe Mod's original release date)
};

void GabeEvents_Init();
GabeHoliday GabeEvents_GetHoliday();
const char* GabeEvents_GetHolidayName();
bool GabeEvents_IsActive(GabeHoliday holiday);

#endif
