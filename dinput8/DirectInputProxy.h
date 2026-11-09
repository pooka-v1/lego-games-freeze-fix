#pragma once

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>

#include "EnumCache.h"
#include "Log.h"

// IDirectInput8 exists in two flavours with the same shape: ANSI (A, char strings) and Unicode
// (W, wchar_t strings). The game asks for one of them in DirectInput8Create. These traits hold the
// types that differ, so a single template below implements both.
struct AnsiTypes {
    using Interface = IDirectInput8A;
    using Device = IDirectInputDevice8A;
    using DeviceInstance = DIDEVICEINSTANCEA;
    using EnumCallback = LPDIENUMDEVICESCALLBACKA;
    using SemanticsCallback = LPDIENUMDEVICESBYSEMANTICSCBA;
    using ActionFormat = DIACTIONFORMATA;
    using ConfigureParams = DICONFIGUREDEVICESPARAMSA;
    using Text = LPCSTR;
    static const IID& iid() { return IID_IDirectInput8A; }
    static const char* name() { return "IDirectInput8A"; }
};

struct WideTypes {
    using Interface = IDirectInput8W;
    using Device = IDirectInputDevice8W;
    using DeviceInstance = DIDEVICEINSTANCEW;
    using EnumCallback = LPDIENUMDEVICESCALLBACKW;
    using SemanticsCallback = LPDIENUMDEVICESBYSEMANTICSCBW;
    using ActionFormat = DIACTIONFORMATW;
    using ConfigureParams = DICONFIGUREDEVICESPARAMSW;
    using Text = LPCWSTR;
    static const IID& iid() { return IID_IDirectInput8W; }
    static const char* name() { return "IDirectInput8W"; }
};

// Joysticks, gamepads, wheels, flight sticks and the like (as opposed to keyboards and mice).
inline bool isGameController(DWORD devType) {
    const BYTE type = GET_DIDEVICE_TYPE(devType);
    return (type >= DI8DEVTYPE_JOYSTICK && type <= DI8DEVTYPE_1STPERSON) || type == DI8DEVTYPE_SUPPLEMENTAL;
}

// Diagnostic: one log line per device. For a HID device, DirectInput packs the VID (low word) and the PID
// (high word) into the first field of guidProduct; the game's Xbox check compares exactly that.
inline void logDevice(const char* prefix, const DIDEVICEINSTANCEA& device) {
    logLine("%s\"%s\" type 0x%08lX VID_%04X PID_%04X", prefix, device.tszProductName, device.dwDevType,
            LOWORD(device.guidProduct.Data1), HIWORD(device.guidProduct.Data1));
}

inline void logDevice(const char* prefix, const DIDEVICEINSTANCEW& device) {
    logLine("%s\"%ls\" type 0x%08lX VID_%04X PID_%04X", prefix, device.tszProductName, device.dwDevType,
            LOWORD(device.guidProduct.Data1), HIWORD(device.guidProduct.Data1));
}

// Stands in for the real IDirectInput8 object and forwards every method unchanged, except EnumDevices:
// after the first real enumeration, it answers from EnumCache (instant instead of ~160 ms).
// It also logs each enumeration: how many devices the game got and how long the game took with them.
// The game runs its slow device check inside the enumeration callback, so that time also shows whether
// the WMI shortcut (WmiShortcut.h) works.
// (Hiding the pads from the game was tried first and dropped: the game then does not see the pads at
// all, and it runs the same check for keyboards and mice too.)
template <class T>
class DirectInputProxy final : public T::Interface {
public:
    // "cache" may be null: then every enumeration goes to the real DirectInput.
    DirectInputProxy(typename T::Interface* real, EnumCache<T>* cache) : m_real(real), m_cache(cache) {}

    // --- IUnknown ---
    // The reference count lives in the real object; we only follow it, and delete ourselves when it
    // reaches zero. QueryInterface for any other interface returns the real object directly.
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** out) override {
        if (!out) return E_POINTER;
        if (riid == IID_IUnknown || riid == T::iid()) {
            AddRef();
            *out = this;
            return S_OK;
        }
        return m_real->QueryInterface(riid, out);
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return m_real->AddRef(); }

    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG remaining = m_real->Release();
        if (remaining == 0) delete this;
        return remaining;
    }

    // --- IDirectInput8: the two watched enumerations ---
    HRESULT STDMETHODCALLTYPE EnumDevices(DWORD devType, typename T::EnumCallback callback, LPVOID ref,
                                          DWORD flags) override {
        if (!callback) return DIERR_INVALIDPARAM;
        const ULONGLONG start = GetTickCount64();

        typename EnumCache<T>::Devices cached;
        if (m_cache && m_cache->lookup(devType, flags, cached)) {
            int gameControllers = 0;
            for (const auto& device : cached) {
                if (isGameController(device.dwDevType)) ++gameControllers;
                if (callback(&device, ref) == DIENUM_STOP) break;
            }
            logLine("EnumDevices(type 0x%02X, cached): %zu device(s), %d of them game controller(s), %llu ms",
                    devType, cached.size(), gameControllers, GetTickCount64() - start);
            return DI_OK;
        }

        // First time for this kind of request: enumerate for real. DirectInput calls our counter for
        // each device, and the counter calls the game's callback. "ref" is how a C callback carries
        // context.
        EnumCounter counter{callback, ref};
        const HRESULT hr = m_real->EnumDevices(devType, &EnumCounter::onDevice, &counter, flags);
        logLine("EnumDevices(type 0x%02X): %d device(s), %d of them game controller(s), %llu ms", devType,
                counter.devices, counter.gameControllers, GetTickCount64() - start);
        for (const auto& device : counter.seen) logDevice("  ", device);
        // A list the game cut short (DIENUM_STOP) is incomplete: do not remember it.
        if (m_cache && SUCCEEDED(hr) && !counter.stopped) m_cache->store(devType, flags, std::move(counter.seen));
        return hr;
    }

    HRESULT STDMETHODCALLTYPE EnumDevicesBySemantics(typename T::Text userName,
                                                     typename T::ActionFormat* actionFormat,
                                                     typename T::SemanticsCallback callback, LPVOID ref,
                                                     DWORD flags) override {
        SemanticsCounter counter{callback, ref};
        const ULONGLONG start = GetTickCount64();
        const HRESULT hr = m_real->EnumDevicesBySemantics(userName, actionFormat, &SemanticsCounter::onDevice,
                                                          &counter, flags);
        logLine("EnumDevicesBySemantics: %d device(s), %d of them game controller(s), %llu ms", counter.devices,
                counter.gameControllers, GetTickCount64() - start);
        return hr;
    }

    // --- IDirectInput8: plain forwarding ---
    HRESULT STDMETHODCALLTYPE CreateDevice(REFGUID guid, typename T::Device** device, LPUNKNOWN outer) override {
        return m_real->CreateDevice(guid, device, outer);
    }

    HRESULT STDMETHODCALLTYPE GetDeviceStatus(REFGUID guid) override { return m_real->GetDeviceStatus(guid); }

    HRESULT STDMETHODCALLTYPE RunControlPanel(HWND owner, DWORD flags) override {
        return m_real->RunControlPanel(owner, flags);
    }

    HRESULT STDMETHODCALLTYPE Initialize(HINSTANCE instance, DWORD version) override {
        return m_real->Initialize(instance, version);
    }

    HRESULT STDMETHODCALLTYPE FindDevice(REFGUID classGuid, typename T::Text name, LPGUID instanceGuid) override {
        return m_real->FindDevice(classGuid, name, instanceGuid);
    }

    HRESULT STDMETHODCALLTYPE ConfigureDevices(LPDICONFIGUREDEVICESCALLBACK callback,
                                               typename T::ConfigureParams* params, DWORD flags,
                                               LPVOID ref) override {
        return m_real->ConfigureDevices(callback, params, flags, ref);
    }

private:
    struct EnumCounter {
        typename T::EnumCallback gameCallback;
        LPVOID gameRef;
        int devices = 0;
        int gameControllers = 0;
        bool stopped = false;
        typename EnumCache<T>::Devices seen;

        static BOOL CALLBACK onDevice(const typename T::DeviceInstance* device, LPVOID ref) {
            auto* self = static_cast<EnumCounter*>(ref);
            ++self->devices;
            if (isGameController(device->dwDevType)) ++self->gameControllers;
            self->seen.push_back(*device);
            const BOOL next = self->gameCallback(device, self->gameRef);  // DIENUM_STOP from the game ends it
            if (next == DIENUM_STOP) self->stopped = true;
            return next;
        }
    };

    struct SemanticsCounter {
        typename T::SemanticsCallback gameCallback;
        LPVOID gameRef;
        int devices = 0;
        int gameControllers = 0;

        static BOOL CALLBACK onDevice(const typename T::DeviceInstance* device, typename T::Device* deviceObject,
                                      DWORD flags, DWORD remaining, LPVOID ref) {
            auto* self = static_cast<SemanticsCounter*>(ref);
            ++self->devices;
            if (isGameController(device->dwDevType)) ++self->gameControllers;
            return self->gameCallback(device, deviceObject, flags, remaining, self->gameRef);
        }
    };

    typename T::Interface* m_real;
    EnumCache<T>* m_cache;
};
