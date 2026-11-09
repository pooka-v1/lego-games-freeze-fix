#include "Config.h"

#include <windows.h>

#include <string>

#include "Log.h"

namespace {

std::wstring iniPath() {
    HMODULE self = nullptr;
    // FROM_ADDRESS: "the module that contains this function" = our DLL, not the game's exe.
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&iniPath), &self);
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(self, path, MAX_PATH);
    std::wstring result = path;
    result.erase(result.find_last_of(L'\\') + 1);
    return result + L"dinput8-proxy.ini";
}

ProxyConfig loadConfig() {
    const std::wstring path = iniPath();
    ProxyConfig config;
    // GetPrivateProfileInt returns the default (1) when the file or the key does not exist.
    config.wmiShortcut = GetPrivateProfileIntW(L"proxy", L"WmiShortcut", 1, path.c_str()) != 0;
    config.enumCache = GetPrivateProfileIntW(L"proxy", L"EnumCache", 1, path.c_str()) != 0;
    config.realWmi = GetPrivateProfileIntW(L"proxy", L"RealWmi", 1, path.c_str()) != 0;
    logLine("Config: WmiShortcut=%d EnumCache=%d RealWmi=%d", config.wmiShortcut ? 1 : 0,
            config.enumCache ? 1 : 0, config.realWmi ? 1 : 0);
    return config;
}

}  // namespace

const ProxyConfig& proxyConfig() {
    static const ProxyConfig config = loadConfig();  // read once, on first use
    return config;
}
