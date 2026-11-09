#pragma once

// Makes the game's "is this pad an Xbox pad?" check fast. The game (Microsoft's IsXInputDevice sample)
// asks WMI for every device in the PC (Win32_PNPEntity, about 1.7 s per question) only to read each
// device ID and look for "IG_". This installs a fake WMI in the game's exe that answers that one
// question instantly. With useRealWmi, the answer is the real WMI's, read once on a background thread
// and kept; without it, the devices are read from SetupAPI on every request.
// Returns false if the game does not import CoCreateInstance the expected way (nothing is changed).
bool installWmiShortcut(bool useRealWmi);

// Reads the real WMI's list again (~1.7 s, on the calling thread), after a device was plugged in or
// removed. EnumCache calls it before refreshing its DirectInput lists, so a new pad is already in the
// WMI list (with its "IG_") when the game first sees it; otherwise the game would take an Xbox pad for
// a plain DirectInput one, and one pad would count as two players. Never call it on the game's thread.
// Does nothing without the real WMI list (RealWmi=0 reads fresh devices anyway, or no shortcut).
void refreshXInputDeviceList();
