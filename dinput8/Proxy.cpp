// Entry points of the fake dinput8.dll. Windows looks for a DLL in the game's folder before the
// system folder, so a dinput8.dll placed next to the exe is loaded instead of the real one. Every
// function here loads the real dinput8.dll from the system folder and forwards to it; only
// DirectInput8Create adds something: it installs the WMI shortcut (WmiShortcut.h) and wraps the object
// it returns, to cache and log the enumerations (DirectInputProxy.h, EnumCache.h).

#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>

#include <string>

#include "Config.h"
#include "DirectInputProxy.h"
#include "Log.h"
#include "WmiShortcut.h"

namespace {

using DirectInput8CreateFn = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
using NoArgsFn = HRESULT(WINAPI*)();
using DllGetClassObjectFn = HRESULT(WINAPI*)(REFCLSID, REFIID, LPVOID*);
using GetdfDIJoystickFn = const DIDATAFORMAT*(WINAPI*)();

struct RealDInput8 {
    HMODULE module = nullptr;
    DirectInput8CreateFn directInput8Create = nullptr;
    NoArgsFn dllCanUnloadNow = nullptr;
    DllGetClassObjectFn dllGetClassObject = nullptr;
    NoArgsFn dllRegisterServer = nullptr;
    NoArgsFn dllUnregisterServer = nullptr;
    GetdfDIJoystickFn getdfDIJoystick = nullptr;
};

RealDInput8 loadRealDInput8() {
    // For a 32-bit game on 64-bit Windows, "System32" is silently redirected to SysWOW64, where the
    // 32-bit dinput8.dll lives. A full path is required: just "dinput8.dll" would find this proxy again.
    wchar_t systemDir[MAX_PATH] = {};
    const UINT length = GetSystemDirectoryW(systemDir, MAX_PATH);
    const std::wstring path = std::wstring(systemDir, length) + L"\\dinput8.dll";

    RealDInput8 real;
    real.module = LoadLibraryW(path.c_str());
    if (!real.module) {
        logLine("ERROR: could not load the real %ls (error %lu)", path.c_str(), GetLastError());
        return real;
    }
    // GetProcAddress returns a generic pointer; we know each function's real signature.
    real.directInput8Create =
        reinterpret_cast<DirectInput8CreateFn>(GetProcAddress(real.module, "DirectInput8Create"));
    real.dllCanUnloadNow = reinterpret_cast<NoArgsFn>(GetProcAddress(real.module, "DllCanUnloadNow"));
    real.dllGetClassObject =
        reinterpret_cast<DllGetClassObjectFn>(GetProcAddress(real.module, "DllGetClassObject"));
    real.dllRegisterServer = reinterpret_cast<NoArgsFn>(GetProcAddress(real.module, "DllRegisterServer"));
    real.dllUnregisterServer = reinterpret_cast<NoArgsFn>(GetProcAddress(real.module, "DllUnregisterServer"));
    real.getdfDIJoystick = reinterpret_cast<GetdfDIJoystickFn>(GetProcAddress(real.module, "GetdfDIJoystick"));
    logLine("Loaded the real %ls", path.c_str());
    return real;
}

// Loaded on first use, never in DllMain: loading another DLL while Windows holds the loader lock
// (as it does during DllMain) can deadlock. A function-local static is initialized exactly once,
// even if several threads arrive at the same time (guaranteed since C++11).
const RealDInput8& realDInput8() {
    static const RealDInput8 real = loadRealDInput8();
    return real;
}

// One device-list cache per flavour (A/W), shared by every DirectInput object the game creates. It gets
// a second DirectInput object of its own, for its background refreshes. Null if that object cannot be
// created: then the proxy simply does not cache.
template <class T>
EnumCache<T>* enumCache(HINSTANCE instance, DWORD version) {
    static EnumCache<T>* const cache = [&]() -> EnumCache<T>* {
        if (!proxyConfig().enumCache) return nullptr;
        void* refresher = nullptr;
        if (FAILED(realDInput8().directInput8Create(instance, version, T::iid(), &refresher, nullptr))) {
            logLine("Enum cache: could not create its DirectInput object, NOT caching");
            return nullptr;
        }
        return new EnumCache<T>(static_cast<typename T::Interface*>(refresher));  // never deleted, see EnumCache
    }();
    return cache;
}

}  // namespace

// The function every DirectInput 8 game calls first. We let the real one create the object, then hand
// the game our wrapper around it instead.
extern "C" HRESULT WINAPI DirectInput8Create(HINSTANCE instance, DWORD version, REFIID riid, LPVOID* out,
                                             LPUNKNOWN outer) {
    const RealDInput8& real = realDInput8();
    if (!real.directInput8Create) return E_FAIL;

    // Here and not in DllMain: by now the game's import table is filled in, and the game has not
    // enumerated any device yet. The static makes it run once, even if the game creates several objects.
    static const bool shortcutInstalled =
        proxyConfig().wmiShortcut && installWmiShortcut(proxyConfig().realWmi);
    (void)shortcutInstalled;

    const HRESULT hr = real.directInput8Create(instance, version, riid, out, outer);
    if (FAILED(hr) || !out || !*out) return hr;

    if (riid == IID_IDirectInput8A) {
        *out = static_cast<IDirectInput8A*>(new DirectInputProxy<AnsiTypes>(
            static_cast<IDirectInput8A*>(*out), enumCache<AnsiTypes>(instance, version)));
        logLine("DirectInput8Create: wrapped %s", AnsiTypes::name());
    } else if (riid == IID_IDirectInput8W) {
        *out = static_cast<IDirectInput8W*>(new DirectInputProxy<WideTypes>(
            static_cast<IDirectInput8W*>(*out), enumCache<WideTypes>(instance, version)));
        logLine("DirectInput8Create: wrapped %s", WideTypes::name());
    } else {
        logLine("DirectInput8Create: unknown interface requested, passed through unwrapped");
    }
    return hr;
}

// The rest of the real DLL's exports, forwarded as-is so nothing that expects them breaks.
STDAPI DllCanUnloadNow() {
    const RealDInput8& real = realDInput8();
    return real.dllCanUnloadNow ? real.dllCanUnloadNow() : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID clsid, REFIID riid, LPVOID* out) {
    const RealDInput8& real = realDInput8();
    return real.dllGetClassObject ? real.dllGetClassObject(clsid, riid, out) : CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllRegisterServer() {
    const RealDInput8& real = realDInput8();
    return real.dllRegisterServer ? real.dllRegisterServer() : E_FAIL;
}

STDAPI DllUnregisterServer() {
    const RealDInput8& real = realDInput8();
    return real.dllUnregisterServer ? real.dllUnregisterServer() : E_FAIL;
}

extern "C" const DIDATAFORMAT* WINAPI GetdfDIJoystick() {
    const RealDInput8& real = realDInput8();
    return real.getdfDIJoystick ? real.getdfDIJoystick() : nullptr;
}
