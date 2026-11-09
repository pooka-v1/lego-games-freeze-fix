#include "Log.h"

#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <string>

namespace {

// Full path of the log: same folder as this DLL (the game folder), so it is easy to find.
std::wstring logPath() {
    HMODULE self = nullptr;
    // FROM_ADDRESS: "the module that contains this function" = our DLL, not the game's exe.
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&logPath), &self);
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(self, path, MAX_PATH);
    std::wstring result = path;
    result.erase(result.find_last_of(L'\\') + 1);
    return result + L"dinput8-proxy.log";
}

std::mutex g_logMutex;  // DirectInput may call us from several game threads

}  // namespace

void logLine(const char* format, ...) {
    char message[512];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    SYSTEMTIME now;
    GetLocalTime(&now);

    std::lock_guard<std::mutex> lock(g_logMutex);
    static const std::wstring path = logPath();
    FILE* file = nullptr;
    if (_wfopen_s(&file, path.c_str(), L"a") != 0 || !file) return;
    fprintf(file, "%04u-%02u-%02u %02u:%02u:%02u.%03u  %s\n", now.wYear, now.wMonth, now.wDay, now.wHour,
            now.wMinute, now.wSecond, now.wMilliseconds, message);
    fclose(file);
}
