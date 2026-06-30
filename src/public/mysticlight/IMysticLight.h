#pragma once

//============================================================
// Mystic Light RGB SDK
// Copyright (c) Micro-Star International Co., Ltd. All rights reserved.
// Copyright (c) Valve Corporation. All rights reserved
//======================================================================================

#include "basetypes.h"
#include "dbgflag.h"
#include "platform.h"
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <oleauto.h>

//============================================================
// Purpose: Mystic Light SDK
//============================================================

class IMysticLight
{
public:
    virtual ~IMysticLight() {}

    // Initializes the Mystic Light SDK/runtime.
    // This must be called before any other Mystic Light function.
    virtual int Initialize() = 0;

    // Retrieves information about all compatible Mystic Light device types.
    // pDevType receives an array of device type names (e.g. motherboard, GPU, RAM).
    // pLedCount receives an array containing the number of controllable LEDs/zones
    // for each corresponding device type.
    virtual int GetDeviceInfo(SAFEARRAY** pDevType, SAFEARRAY** pLedCount) = 0;

    // Retrieves the names of all devices belonging to the specified device type.
    // Example: all RGB fans connected to the motherboard.
    virtual int GetDeviceName(BSTR type, SAFEARRAY** pDevName) = 0;

    // Retrieves the name of a specific device by its index within the specified
    // device type. This is the extended version of GetDeviceName().
    virtual int GetDeviceNameEx(BSTR type, DWORD index, BSTR* pDevName) = 0;

    // Retrieves a human-readable description for the specified Mystic Light
    // SDK error code.
    virtual int GetErrorMessage(int errorCode, BSTR* pDesc) = 0;

    // Retrieves the names of all LEDs or lighting zones for the specified
    // device type.
    virtual int GetLedName(BSTR type, SAFEARRAY** pLedName) = 0;

    // Retrieves detailed information about a specific LED or lighting zone.
    // pName receives the LED/zone name.
    // pLedStyles receives the list of supported lighting effects/styles.
    virtual int GetLedInfo(BSTR type, DWORD index, BSTR* pName, SAFEARRAY** pLedStyles) = 0;

    // Retrieves the current RGB color of the specified LED or lighting zone.
    virtual int GetLedColor(BSTR type, DWORD index, DWORD* r, DWORD* g, DWORD* b) = 0;

    // Retrieves the currently active lighting effect/style for the specified
    // LED or lighting zone (such as Static, Breathing, Rainbow, etc.).
    virtual int GetLedStyle(BSTR type, DWORD index, BSTR* style) = 0;

    // Retrieves the maximum brightness value supported by the specified
    // LED or lighting zone.
    virtual int GetLedMaxBright(BSTR type, DWORD index, DWORD* maxLevel) = 0;

    // Retrieves the current brightness level of the specified LED or
    // lighting zone.
    virtual int GetLedBright(BSTR type, DWORD index, DWORD* currentLevel) = 0;

    // Retrieves the maximum animation speed supported by the specified
    // LED or lighting zone.
    virtual int GetLedMaxSpeed(BSTR type, DWORD index, DWORD* maxLevel) = 0;

    // Retrieves the current animation/effect speed of the specified
    // LED or lighting zone.
    virtual int GetLedSpeed(BSTR type, DWORD index, DWORD* currentLevel) = 0;

    // Sets the RGB color of the specified LED or lighting zone.
    virtual int SetLedColor(BSTR type, DWORD index, DWORD r, DWORD g, DWORD b) = 0;

    // Sets the RGB colors of multiple LEDs or lighting zones at once.
    // pLedName contains the LED names to modify.
    // r, g, and b contain the corresponding RGB values.
    virtual int SetLedColors(BSTR type, DWORD areaIndex, SAFEARRAY** pLedName, DWORD* r, DWORD* g, DWORD* b) = 0;

    // Extended version of SetLedColor().
    // Allows setting a specific LED by name and includes an additional
    // SDK-specific parameter whose purpose is currently undocumented.
    virtual int SetLedColorEx(BSTR type, DWORD areaIndex, BSTR ledName,
        DWORD r, DWORD g, DWORD b, DWORD unknown) = 0;

    // Synchronized version of SetLedColorEx().
    // Applies the color change using Mystic Light's synchronization system
    // so multiple devices or zones update together.
    virtual int SetLedColorSync(BSTR type, DWORD areaIndex, BSTR ledName,
        DWORD r, DWORD g, DWORD b, DWORD unknown) = 0;

    // Sets the lighting effect/style for the specified LED or lighting zone.
    // Examples include Static, Breathing, Flashing, Rainbow, etc.
    virtual int SetLedStyle(BSTR type, DWORD index, BSTR style) = 0;

    // Sets the brightness level of the specified LED or lighting zone.
    virtual int SetLedBright(BSTR type, DWORD index, DWORD level) = 0;

    // Sets the animation/effect speed of the specified LED or lighting zone.
    virtual int SetLedSpeed(BSTR type, DWORD index, DWORD level) = 0;
};

#define INTERFACEVERSION_IMYSTICLIGHT "IMysticLight001"

// Factory functions implemented in MysticRGB.lib
IMysticLight* CreateMysticLight();
void DestroyMysticLight(IMysticLight* pLight);