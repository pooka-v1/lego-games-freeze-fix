#pragma once

#include <windows.h>
#include <objbase.h>  // COM basics (LPUNKNOWN...): WIN32_LEAN_AND_MEAN keeps them out of windows.h

#include <string>
#include <vector>

using CoCreateInstanceFn = HRESULT(WINAPI*)(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID*);

// Asks the real WMI for every device (Win32_PNPEntity), exactly as the game's check does, and returns
// the DeviceIDs that contain "IG_" (XInput-capable pads), in WMI's order. Slow (about 1.7 s): call it
// from a thread of our own, never from the game's.
// "coCreateInstance" is the one the game would have used (the original from its import table), so
// whatever sits inside it (Steam, for example) answers us as it would answer the game.
// The calling thread must have COM initialized (CoInitializeEx).
// Returns false, and logs why, if WMI could not be asked or stopped halfway.
bool readXInputIdsFromWmi(CoCreateInstanceFn coCreateInstance, std::vector<std::wstring>& ids);
