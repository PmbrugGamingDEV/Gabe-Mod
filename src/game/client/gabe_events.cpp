#include "cbase.h"
#include "gabe_events.h"
#include "gabe_client.h"
#include <time.h>

/// Valve Corporation, Gabe Mod Plus
/// Purpose: Dynamic holiday detection system
/// No initialization required.
/// Always computes current holiday on demand.

extern ConVar gabe_forceholiday;

static GabeHoliday ComputeHoliday()
{
	int forced = gabe_forceholiday.GetInt();
	if (forced > 0)
		return (GabeHoliday)forced;

	time_t t = time(NULL);
	tm* now = localtime(&t);

	if (!now)
		return HOLIDAY_NONE;

	// Fixed-date holidays
	if (now->tm_mon == 0 && now->tm_mday == 1)
		return HOLIDAY_NEWYEAR;

	if (now->tm_mon == 1 && now->tm_mday == 14)
		return HOLIDAY_VALENTINE;

	if (now->tm_mon == 2 && now->tm_mday == 17)
		return HOLIDAY_STPATRICK;

	if (now->tm_mon == 6 && now->tm_mday == 4)
		return HOLIDAY_JULY4;

	// Month-based seasonal modes
	if (now->tm_mon == 9)
		return HOLIDAY_HALLOWEEN;

	if (now->tm_mon == 11)
		return HOLIDAY_CHRISTMAS;

	if (now->tm_mon == 3 &&
		now->tm_mday >= 1 &&
		now->tm_mday <= 30)
	{
		return HOLIDAY_EASTER;
	}

	// Gabe Mod Anniversary (July 25th)
	if (now->tm_mon == 6 && now->tm_mday == 25)
		return HOLIDAY_GABEMODANNIVERSARY;

	// Thanksgiving (4th Thursday of November)
	if (now->tm_mon == 10 &&
		now->tm_mday >= 22 &&
		now->tm_mday <= 28 &&
		now->tm_wday == 4)
	{
		return HOLIDAY_THANKSGIVING;
	}

	return HOLIDAY_NONE;
}

GabeHoliday GabeEvents_GetHoliday()
{
	return ComputeHoliday();
}

bool GabeEvents_IsActive(GabeHoliday holiday)
{
	return (ComputeHoliday() == holiday);
}

const char* GabeEvents_GetHolidayName()
{
	switch (ComputeHoliday())
	{
	case HOLIDAY_NEWYEAR: return "New Year";
	case HOLIDAY_VALENTINE: return "Valentine's Day";
	case HOLIDAY_STPATRICK: return "St. Patrick's Day";
	case HOLIDAY_EASTER: return "Easter Season";
	case HOLIDAY_MEMORIAL: return "Memorial Season";
	case HOLIDAY_JULY4: return "Independence Day";
	case HOLIDAY_LABOR: return "Labor Season";
	case HOLIDAY_HALLOWEEN: return "Halloween";
	case HOLIDAY_THANKSGIVING: return "Thanksgiving Season";
	case HOLIDAY_CHRISTMAS: return "Christmas";
	case HOLIDAY_GABEMODANNIVERSARY: return "Anniversary of Gabe Mod";
	default: return "None";
	}
}
