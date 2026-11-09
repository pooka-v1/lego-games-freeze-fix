#pragma once

// Switches read once from dinput8-proxy.ini, next to this DLL. The file is optional; everything is on
// by default. They allow turning each part off for one game without rebuilding:
//
//   [proxy]
//   WmiShortcut=1   ; fake WMI for the game's "is this an Xbox pad?" check (WmiShortcut.h)
//   EnumCache=1     ; remembered DirectInput device lists (EnumCache.h)
//
// Developer-only, left out of the users' template and the README on purpose:
//   RealWmi=1       ; the fake WMI answers with the real WMI's list, read once on a thread of our own;
//                   ; 0 = it reads SetupAPI on every request instead (misses Steam Input's virtual pad).
//                   ; A way out if some game ever misbehaves with the real WMI.
struct ProxyConfig {
    bool wmiShortcut = true;
    bool enumCache = true;
    bool realWmi = true;
};

const ProxyConfig& proxyConfig();
