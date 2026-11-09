#pragma once

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <windows.h>
#include <cfgmgr32.h>
#include <dinput.h>

#include <cstdint>
#include <map>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "Log.h"
#include "WmiShortcut.h"

// Remembers the device lists DirectInput returns, so the game gets them instantly instead of waiting
// about 160 ms per EnumDevices call while DirectInput walks every HID device again.
// The lists stay up to date: Windows notifies us when a HID device is added or removed, and a
// background thread then reads the WMI shortcut's list again (WmiShortcut.h) and enumerates again with
// its own DirectInput object. The game never waits; a new pad shows up about 2-3 s after plugging it.
//
// It is created once and lives until the game exits (the background thread uses it until then), so
// it is never deleted on purpose.
template <class T>
class EnumCache {
public:
    using Devices = std::vector<typename T::DeviceInstance>;

    // "refresher" is a DirectInput object of our own, used only by the background thread, so the
    // game's object is never used from two threads at once.
    explicit EnumCache(typename T::Interface* refresher) : m_refresher(refresher) {
        m_wakeUp = CreateEventW(nullptr, FALSE, FALSE, nullptr);  // auto-reset: one wake-up per SetEvent
        if (!watchHidChanges()) m_waitMs = kFallbackRefreshMs;
        std::thread([this] { refreshLoop(); }).detach();
    }

    // Copies the remembered list for this (type, flags) request; false if it was never enumerated.
    bool lookup(DWORD devType, DWORD flags, Devices& out) {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto found = m_lists.find(key(devType, flags));
        if (found == m_lists.end()) return false;
        out = found->second;
        return true;
    }

    void store(DWORD devType, DWORD flags, Devices devices) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lists[key(devType, flags)] = std::move(devices);
    }

private:
    // The game asks with a device type and flags; each combination is a separate list.
    static uint64_t key(DWORD devType, DWORD flags) { return (static_cast<uint64_t>(devType) << 32) | flags; }
    static DWORD devTypeOf(uint64_t listKey) { return static_cast<DWORD>(listKey >> 32); }
    static DWORD flagsOf(uint64_t listKey) { return static_cast<DWORD>(listKey); }

    static BOOL CALLBACK collect(const typename T::DeviceInstance* device, LPVOID ref) {
        static_cast<Devices*>(ref)->push_back(*device);
        return DIENUM_CONTINUE;
    }

    // Called by Windows on one of its own threads: just wake our refresh thread.
    static DWORD CALLBACK onHidChange(HCMNOTIFICATION, PVOID context, CM_NOTIFY_ACTION action,
                                      PCM_NOTIFY_EVENT_DATA, DWORD) {
        if (action == CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL || action == CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL) {
            SetEvent(static_cast<EnumCache*>(context)->m_wakeUp);
        }
        return ERROR_SUCCESS;
    }

    bool watchHidChanges() {
        // GUID_DEVINTERFACE_HID, written out to avoid linking hid.lib just for this constant.
        static const GUID hidInterface = {0x4D1E55B2, 0xF16F, 0x11CF, {0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30}};
        CM_NOTIFY_FILTER filter{};
        filter.cbSize = sizeof(filter);
        filter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
        filter.u.DeviceInterface.ClassGuid = hidInterface;
        HCMNOTIFICATION notification = nullptr;
        const CONFIGRET result = CM_Register_Notification(&filter, this, &EnumCache::onHidChange, &notification);
        if (result != CR_SUCCESS) {
            logLine("Enum cache: cannot watch device changes (CONFIGRET %lu), refreshing every %lu s instead",
                    result, kFallbackRefreshMs / 1000);
            return false;
        }
        return true;
    }

    void refreshLoop() {
        for (;;) {
            const bool deviceChanged = WaitForSingleObject(m_wakeUp, m_waitMs) == WAIT_OBJECT_0;
            // Plugging a pad fires several notifications in a row; let them pass, then enumerate once.
            if (deviceChanged) Sleep(kSettleMs);

            std::vector<uint64_t> keys;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                for (const auto& entry : m_lists) keys.push_back(entry.first);
            }
            // Order matters (bug seen 02/11, Pro 3 plugged mid-game). The game checks each pad it gets
            // from these lists against the WMI list, so a pad must never reach it before that list has it.
            // Reading WMI first is not enough: a pad that appears during the ~1.7 s WMI read ends up in
            // the DirectInput lists but not in the WMI one, and the game takes it for a non-Xbox pad.
            // So: 1) enumerate DirectInput into new lists, kept aside; 2) read WMI, which then surely has
            // every pad of those lists; 3) hand both to the game. A pad that appears after step 1 fires a
            // notification of its own (m_wakeUp is set again), so the next round brings it in.
            // Enumerate outside the lock: the game keeps reading the old lists meanwhile.
            std::vector<std::pair<uint64_t, Devices>> fresh;
            for (const uint64_t listKey : keys) {
                Devices devices;
                if (SUCCEEDED(m_refresher->EnumDevices(devTypeOf(listKey), &EnumCache::collect, &devices,
                                                       flagsOf(listKey)))) {
                    fresh.emplace_back(listKey, std::move(devices));
                }
            }
            // Also on the fallback timer: without notifications, that is the only way to see a new pad.
            refreshXInputDeviceList();
            for (auto& list : fresh) store(devTypeOf(list.first), flagsOf(list.first), std::move(list.second));
            if (deviceChanged) logLine("Enum cache: device change, %zu list(s) refreshed", fresh.size());
        }
    }

    static constexpr DWORD kSettleMs = 500;
    static constexpr DWORD kFallbackRefreshMs = 30000;  // only if Windows refuses the notifications

    typename T::Interface* m_refresher;
    HANDLE m_wakeUp = nullptr;
    DWORD m_waitMs = INFINITE;
    std::mutex m_mutex;
    std::map<uint64_t, Devices> m_lists;
};
