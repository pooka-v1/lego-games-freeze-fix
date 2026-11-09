#pragma once

// Appends one timestamped line to dinput8-proxy.log, next to this DLL (printf-style format).
// It is the only way to see from outside that the proxy loaded and what it hid.
void logLine(const char* format, ...);
