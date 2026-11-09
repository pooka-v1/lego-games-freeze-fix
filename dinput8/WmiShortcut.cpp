#include "WmiShortcut.h"

#include <windows.h>
#include <cfgmgr32.h>  // MAX_DEVICE_ID_LEN
#include <setupapi.h>
#include <wbemidl.h>

#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "ImportHook.h"
#include "Log.h"
#include "RealWmi.h"

namespace {

using DeviceIds = std::vector<std::wstring>;

// Fallback only, if the real WMI cannot be asked (see WmiCache). IDs of the present devices whose ID
// contains "IG_" (XInput-capable pads), from every enumerator (HID\..., USB\..., ...). Same IDs and
// order as WMI's DeviceID, except what Steam Input adds to WMI from inside the game (its virtual pad).
std::shared_ptr<const DeviceIds> readXInputDeviceIds() {
    auto ids = std::make_shared<DeviceIds>();
    HDEVINFO devices = SetupDiGetClassDevsW(nullptr, nullptr, nullptr, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (devices == INVALID_HANDLE_VALUE) return ids;

    SP_DEVINFO_DATA info{};
    info.cbSize = sizeof(info);
    for (DWORD index = 0; SetupDiEnumDeviceInfo(devices, index, &info); ++index) {
        wchar_t id[MAX_DEVICE_ID_LEN] = {};
        if (SetupDiGetDeviceInstanceIdW(devices, &info, id, MAX_DEVICE_ID_LEN, nullptr) && wcsstr(id, L"IG_")) {
            ids->emplace_back(id);
        }
    }
    SetupDiDestroyDeviceInfoList(devices);
    return ids;
}

// IUnknown for our fake COM objects: a reference count, and QueryInterface for one interface.
// COM objects delete themselves when the last reference is released.
template <class Interface>
class ComObject : public Interface {
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** out) override {
        if (!out) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(Interface)) {
            AddRef();
            *out = static_cast<Interface*>(this);
            return S_OK;
        }
        *out = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_references; }
    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG remaining = --m_references;
        if (remaining == 0) delete this;
        return remaining;
    }

protected:
    virtual ~ComObject() = default;

private:
    std::atomic<ULONG> m_references{1};
};

// One "device" of the answer. The game only calls Get(L"DeviceID"); everything else says "not supported".
class FakeDevice final : public ComObject<IWbemClassObject> {
public:
    explicit FakeDevice(std::wstring deviceId) : m_deviceId(std::move(deviceId)) {}

    HRESULT STDMETHODCALLTYPE Get(LPCWSTR name, long, VARIANT* value, CIMTYPE* type, long* flavor) override {
        if (!name || _wcsicmp(name, L"DeviceID") != 0) return WBEM_E_NOT_FOUND;
        if (value) {
            value->vt = VT_BSTR;
            value->bstrVal = SysAllocString(m_deviceId.c_str());  // the caller frees it (VariantClear)
        }
        if (type) *type = CIM_STRING;
        if (flavor) *flavor = WBEM_FLAVOR_ORIGIN_LOCAL;
        return WBEM_S_NO_ERROR;
    }

    HRESULT STDMETHODCALLTYPE GetQualifierSet(IWbemQualifierSet**) override { return WBEM_E_NOT_SUPPORTED; }
    HRESULT STDMETHODCALLTYPE Put(LPCWSTR, long, VARIANT*, CIMTYPE) override { return WBEM_E_NOT_SUPPORTED; }
    HRESULT STDMETHODCALLTYPE Delete(LPCWSTR) override { return WBEM_E_NOT_SUPPORTED; }
    HRESULT STDMETHODCALLTYPE GetNames(LPCWSTR, long, VARIANT*, SAFEARRAY**) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE BeginEnumeration(long) override { return WBEM_E_NOT_SUPPORTED; }
    HRESULT STDMETHODCALLTYPE Next(long, BSTR*, VARIANT*, CIMTYPE*, long*) override { return WBEM_E_NOT_SUPPORTED; }
    HRESULT STDMETHODCALLTYPE EndEnumeration() override { return WBEM_E_NOT_SUPPORTED; }
    HRESULT STDMETHODCALLTYPE GetPropertyQualifierSet(LPCWSTR, IWbemQualifierSet**) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE Clone(IWbemClassObject** copy) override {
        if (!copy) return WBEM_E_INVALID_PARAMETER;
        *copy = new FakeDevice(m_deviceId);
        return WBEM_S_NO_ERROR;
    }
    HRESULT STDMETHODCALLTYPE GetObjectText(long, BSTR*) override { return WBEM_E_NOT_SUPPORTED; }
    HRESULT STDMETHODCALLTYPE SpawnDerivedClass(long, IWbemClassObject**) override { return WBEM_E_NOT_SUPPORTED; }
    HRESULT STDMETHODCALLTYPE SpawnInstance(long, IWbemClassObject**) override { return WBEM_E_NOT_SUPPORTED; }
    HRESULT STDMETHODCALLTYPE CompareTo(long, IWbemClassObject*) override { return WBEM_E_NOT_SUPPORTED; }
    HRESULT STDMETHODCALLTYPE GetPropertyOrigin(LPCWSTR, BSTR*) override { return WBEM_E_NOT_SUPPORTED; }
    HRESULT STDMETHODCALLTYPE InheritsFrom(LPCWSTR) override { return WBEM_E_NOT_SUPPORTED; }
    HRESULT STDMETHODCALLTYPE GetMethod(LPCWSTR, long, IWbemClassObject**, IWbemClassObject**) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE PutMethod(LPCWSTR, long, IWbemClassObject*, IWbemClassObject*) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE DeleteMethod(LPCWSTR) override { return WBEM_E_NOT_SUPPORTED; }
    HRESULT STDMETHODCALLTYPE BeginMethodEnumeration(long) override { return WBEM_E_NOT_SUPPORTED; }
    HRESULT STDMETHODCALLTYPE NextMethod(long, BSTR*, IWbemClassObject**, IWbemClassObject**) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE EndMethodEnumeration() override { return WBEM_E_NOT_SUPPORTED; }
    HRESULT STDMETHODCALLTYPE GetMethodQualifierSet(LPCWSTR, IWbemQualifierSet**) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetMethodOrigin(LPCWSTR, BSTR*) override { return WBEM_E_NOT_SUPPORTED; }

private:
    std::wstring m_deviceId;
};

// Hands out the devices in batches, as the game asks for them (Next(timeout, 20, ...) in a loop).
class FakeDeviceEnum final : public ComObject<IEnumWbemClassObject> {
public:
    FakeDeviceEnum(std::shared_ptr<const DeviceIds> ids, size_t position)
        : m_ids(std::move(ids)), m_position(position) {}

    HRESULT STDMETHODCALLTYPE Next(long, ULONG count, IWbemClassObject** objects, ULONG* returned) override {
        if (!objects || !returned) return WBEM_E_INVALID_PARAMETER;
        ULONG given = 0;
        while (given < count && m_position < m_ids->size()) {
            objects[given++] = new FakeDevice((*m_ids)[m_position++]);
        }
        *returned = given;
        // WBEM_S_FALSE = "fewer than you asked for": the list is finished.
        return given == count ? WBEM_S_NO_ERROR : WBEM_S_FALSE;
    }

    HRESULT STDMETHODCALLTYPE Reset() override {
        m_position = 0;
        return WBEM_S_NO_ERROR;
    }

    HRESULT STDMETHODCALLTYPE Skip(long, ULONG count) override {
        m_position = (std::min)(m_position + static_cast<size_t>(count), m_ids->size());
        return m_position < m_ids->size() ? WBEM_S_NO_ERROR : WBEM_S_FALSE;
    }

    HRESULT STDMETHODCALLTYPE Clone(IEnumWbemClassObject** copy) override {
        if (!copy) return WBEM_E_INVALID_PARAMETER;
        *copy = new FakeDeviceEnum(m_ids, m_position);
        return WBEM_S_NO_ERROR;
    }

    HRESULT STDMETHODCALLTYPE NextAsync(ULONG, IWbemObjectSink*) override { return WBEM_E_NOT_SUPPORTED; }

private:
    std::shared_ptr<const DeviceIds> m_ids;
    size_t m_position;
};

CoCreateInstanceFn g_realCoCreateInstance = nullptr;

// The answer the game gets: the real WMI's list, read on a thread of our own and kept here. The real
// one is needed because Steam Input adds its virtual pad to it (#HID#VID_28DE&PID_11FF&IG_00) from
// inside the game's process: SetupAPI does not know that pad, and without it the game takes the pad
// for a plain DirectInput one. Reading it ourselves gets Steam's addition too.
// The game waits only once, for the very first list (about 1.2-2 s, during the loading screen).
//
// How the game waits matters. It asks from its main thread, the one that owns its window (a COM "STA"
// thread). That thread must keep handling its window messages while it waits, as it does when COM
// itself waits for the real WMI: blocked without them for ~1.2 s, the GOG Star Wars lost its pads for
// the whole session (fixed 01/11). Hence CoWaitForMultipleHandles below, not a plain C++ wait.
class WmiCache {
public:
    WmiCache() {
        // Manual-reset event (stays signaled once set): "the first list is here". Never closed: this
        // object lives until the game exits, and Windows closes the handle then.
        m_firstListReady = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    }

    void startBackgroundRead() {
        m_started = true;
        std::thread([this] { readAndStore("first list"); }).detach();  // lives until the game exits, like us
    }

    // Reads the list again and replaces the kept one, on the calling thread (~1.7 s). For device changes:
    // a pad plugged in mid-game must be in this list before the game sees it through DirectInput.
    // Does nothing if the background read never started (RealWmi=0, or the shortcut is not installed).
    void refresh() {
        if (m_started) readAndStore("refresh");
    }

    // The latest list. Waits for the first one; null if it never came (see kFirstListTimeoutMs).
    std::shared_ptr<const DeviceIds> latest() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_ids) return m_ids;
        }
        waitForFirstList();  // without holding m_mutex: see waitForFirstList
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_ids;
    }

private:
    // Never on the game's thread: only on our own threads (the first read, and EnumCache's refresh thread).
    void readAndStore(const char* reason) {
        // One read at a time: otherwise an older read finishing last could overwrite a newer list.
        // A separate mutex from m_mutex, so the game is never kept waiting by a read in progress.
        std::lock_guard<std::mutex> readLock(m_readMutex);
        // COM is set up per thread; a new thread starts without it. Multithreaded: we own no window.
        // If the thread already has it (S_FALSE), this just counts one more use, undone below.
        const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        DeviceIds ids;
        const ULONGLONG start = GetTickCount64();
        if (readXInputIdsFromWmi(g_realCoCreateInstance, ids)) {
            logLine("WMI cache: real WMI (%s), %zu XInput device(s) in %llu ms", reason, ids.size(),
                    GetTickCount64() - start);
        } else {
            // Better a list without Steam's pad than a game stuck waiting for one.
            ids = *readXInputDeviceIds();
            logLine("WMI cache: real WMI failed, using SetupAPI instead, %zu XInput device(s)", ids.size());
        }
        for (const auto& id : ids) logLine("  %ls", id.c_str());
        {
            // Swapping the pointer is enough: an answer the game is still reading keeps its own
            // shared_ptr to the old list, which is freed when the last of them lets it go.
            std::lock_guard<std::mutex> lock(m_mutex);
            m_ids = std::make_shared<const DeviceIds>(std::move(ids));
        }
        if (m_firstListReady) SetEvent(m_firstListReady);  // wakes the game if it is waiting in latest()
        if (SUCCEEDED(com)) CoUninitialize();
    }

    void waitForFirstList() {
        if (!m_firstListReady) return;  // could not create the event: answer at once, from SetupAPI
        // CoWaitForMultipleHandles is COM's own way to wait: on an STA thread it runs the same "modal
        // loop" COM runs while the game waits for the real WMI, handling the thread's window messages
        // until the event is set. Flags 0 = exactly that loop. (COWAIT_DISPATCH_WINDOW_MESSAGES is
        // refused on the game's main STA thread: CO_E_NOT_SUPPORTED, 0x80004021, seen 01/11.)
        // Those messages may run game code that comes back here on this same thread; that is why no
        // lock is held while waiting (a std::mutex locked twice by one thread blocks it forever).
        DWORD index = 0;
        const HRESULT hr = CoWaitForMultipleHandles(0, kFirstListTimeoutMs, 1, &m_firstListReady, &index);
        if (FAILED(hr) && hr != RPC_S_CALLPENDING) {  // RPC_S_CALLPENDING = timed out, see latest()
            // Not expected; logged so we would know. A plain wait is still better than no list at all.
            logLine("WMI cache: CoWaitForMultipleHandles failed (0x%08lX), plain wait instead", hr);
            WaitForSingleObject(m_firstListReady, kFirstListTimeoutMs);
        }
    }

    // Safety net only: the real WMI answers in about 2 s even on a busy PC.
    static constexpr DWORD kFirstListTimeoutMs = 15000;

    std::atomic<bool> m_started{false};  // written on the game's thread, read on EnumCache's
    std::mutex m_readMutex;
    std::mutex m_mutex;
    HANDLE m_firstListReady = nullptr;
    std::shared_ptr<const DeviceIds> m_ids;
};

WmiCache g_wmiCache;
// Set once in installWmiShortcut, before the game can reach FakeWbemServices, so a plain bool is enough.
bool g_useRealWmi = true;
std::atomic<bool> g_firstAnswerLogged{false};

// The "connection" to WMI. Only the question "every Win32_PNPEntity" gets a real answer, in the two ways
// the games ask it: CreateInstanceEnum (Microsoft's sample) and ExecQuery (LEGO DC Super-Villains).
class FakeWbemServices final : public ComObject<IWbemServices> {
public:
    HRESULT STDMETHODCALLTYPE CreateInstanceEnum(const BSTR className, long, IWbemContext*,
                                                 IEnumWbemClassObject** result) override {
        if (!result) return WBEM_E_INVALID_PARAMETER;
        *result = nullptr;
        if (!className || _wcsicmp(className, L"Win32_PNPEntity") != 0) {
            logLine("WMI shortcut: unexpected class %ls requested, answered 'not supported'",
                    className ? className : L"(null)");
            return WBEM_E_NOT_SUPPORTED;
        }
        *result = answerWithXInputDevices("CreateInstanceEnum");
        return WBEM_S_NO_ERROR;
    }

    HRESULT STDMETHODCALLTYPE OpenNamespace(const BSTR, long, IWbemContext*, IWbemServices**,
                                            IWbemCallResult**) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE CancelAsyncCall(IWbemObjectSink*) override { return WBEM_E_NOT_SUPPORTED; }
    HRESULT STDMETHODCALLTYPE QueryObjectSink(long, IWbemObjectSink**) override { return WBEM_E_NOT_SUPPORTED; }
    HRESULT STDMETHODCALLTYPE GetObject(const BSTR, long, IWbemContext*, IWbemClassObject**,
                                        IWbemCallResult**) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetObjectAsync(const BSTR, long, IWbemContext*, IWbemObjectSink*) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE PutClass(IWbemClassObject*, long, IWbemContext*, IWbemCallResult**) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE PutClassAsync(IWbemClassObject*, long, IWbemContext*, IWbemObjectSink*) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE DeleteClass(const BSTR, long, IWbemContext*, IWbemCallResult**) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE DeleteClassAsync(const BSTR, long, IWbemContext*, IWbemObjectSink*) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE CreateClassEnum(const BSTR, long, IWbemContext*, IEnumWbemClassObject**) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE CreateClassEnumAsync(const BSTR, long, IWbemContext*, IWbemObjectSink*) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE PutInstance(IWbemClassObject*, long, IWbemContext*, IWbemCallResult**) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE PutInstanceAsync(IWbemClassObject*, long, IWbemContext*, IWbemObjectSink*) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE DeleteInstance(const BSTR, long, IWbemContext*, IWbemCallResult**) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE DeleteInstanceAsync(const BSTR, long, IWbemContext*, IWbemObjectSink*) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync(const BSTR, long, IWbemContext*, IWbemObjectSink*) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE ExecQuery(const BSTR language, const BSTR query, long, IWbemContext*,
                                        IEnumWbemClassObject** result) override {
        if (!result) return WBEM_E_INVALID_PARAMETER;
        *result = nullptr;
        // The exact text, not "contains Win32_PNPEntity": a query with a WHERE, or asking for other
        // properties, would need another answer. Better refused and logged than answered wrongly.
        if (!language || !query || _wcsicmp(language, L"WQL") != 0 ||
            _wcsicmp(query, L"SELECT DeviceID FROM Win32_PNPEntity") != 0) {
            logLine("WMI shortcut: unexpected query %ls requested, answered 'not supported'",
                    query ? query : L"(null)");
            return WBEM_E_NOT_SUPPORTED;
        }
        *result = answerWithXInputDevices("ExecQuery");
        return WBEM_S_NO_ERROR;
    }
    HRESULT STDMETHODCALLTYPE ExecQueryAsync(const BSTR, const BSTR, long, IWbemContext*,
                                             IWbemObjectSink*) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE ExecNotificationQuery(const BSTR, const BSTR, long, IWbemContext*,
                                                    IEnumWbemClassObject**) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync(const BSTR, const BSTR, long, IWbemContext*,
                                                         IWbemObjectSink*) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE ExecMethod(const BSTR, const BSTR, long, IWbemContext*, IWbemClassObject*,
                                         IWbemClassObject**, IWbemCallResult**) override {
        return WBEM_E_NOT_SUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE ExecMethodAsync(const BSTR, const BSTR, long, IWbemContext*, IWbemClassObject*,
                                              IWbemObjectSink*) override {
        return WBEM_E_NOT_SUPPORTED;
    }

private:
    // The answer to both ways of asking. The caller hands the returned reference over to the game.
    static IEnumWbemClassObject* answerWithXInputDevices(const char* askedWith) {
        const ULONGLONG start = GetTickCount64();
        std::shared_ptr<const DeviceIds> ids;
        if (!g_useRealWmi) {
            ids = readXInputDeviceIds();  // a few ms, fresh on every request
        } else {
            ids = g_wmiCache.latest();
            if (!ids) {
                ids = readXInputDeviceIds();
                logLine("WMI shortcut: no list from the real WMI in time, answered from SetupAPI");
            }
        }
        if (!g_firstAnswerLogged.exchange(true)) {
            logLine("WMI shortcut: first answer (%s), %zu XInput device(s), the game waited %llu ms", askedWith,
                    ids->size(), GetTickCount64() - start);
        }
        return new FakeDeviceEnum(std::move(ids), 0);
    }
};

// The object the game creates first (CoCreateInstance(CLSID_WbemLocator)), to "connect" to WMI.
class FakeWbemLocator final : public ComObject<IWbemLocator> {
public:
    HRESULT STDMETHODCALLTYPE ConnectServer(const BSTR, const BSTR, const BSTR, const BSTR, long, const BSTR,
                                            IWbemContext*, IWbemServices** services) override {
        if (!services) return WBEM_E_INVALID_PARAMETER;
        *services = new FakeWbemServices();
        return WBEM_S_NO_ERROR;
    }
};

// Diagnostic: logs each kind of COM object the game creates, once. Shows whether it reaches DirectInput
// or WMI by some other way than the one we wrap.
void logFirstRequest(REFCLSID clsid) {
    static std::mutex mutex;
    static std::vector<CLSID> seen;
    std::lock_guard<std::mutex> lock(mutex);
    if (std::find(seen.begin(), seen.end(), clsid) != seen.end()) return;
    seen.push_back(clsid);
    wchar_t text[40] = {};
    StringFromGUID2(clsid, text, 40);
    logLine("CoCreateInstance from the game: %ls", text);
}

// Replaces CoCreateInstance for the game's exe only. WMI requests get the fake; everything else goes
// to the real function untouched.
HRESULT WINAPI shortcutCoCreateInstance(REFCLSID clsid, LPUNKNOWN outer, DWORD context, REFIID riid,
                                        LPVOID* out) {
    logFirstRequest(clsid);
    if (clsid != __uuidof(WbemLocator)) return g_realCoCreateInstance(clsid, outer, context, riid, out);
    if (!out) return E_POINTER;
    auto* locator = new FakeWbemLocator();
    const HRESULT hr = locator->QueryInterface(riid, out);  // takes its own reference...
    locator->Release();                                     // ...so drop the one from "new"
    return hr;
}

// Diagnostic: the DLL the game's CoCreateInstance really lives in (combase.dll, or one of Steam's if it
// put itself in between).
void logModuleOf(const void* function) {
    HMODULE owner = nullptr;
    wchar_t path[MAX_PATH] = L"(unknown)";
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           static_cast<LPCWSTR>(function), &owner)) {
        GetModuleFileNameW(owner, path, MAX_PATH);
    }
    logLine("WMI shortcut: the game's CoCreateInstance is in %ls", path);
}

}  // namespace

bool installWmiShortcut(bool useRealWmi) {
    void* original = hookImport(GetModuleHandleW(nullptr), "ole32.dll", "CoCreateInstance",
                                reinterpret_cast<void*>(&shortcutCoCreateInstance));
    if (!original) {
        logLine("WMI shortcut: CoCreateInstance not found in the game's imports, NOT installed");
        return false;
    }
    g_realCoCreateInstance = reinterpret_cast<CoCreateInstanceFn>(original);
    logLine("WMI shortcut: installed");
    logModuleOf(original);
    g_useRealWmi = useRealWmi;
    if (useRealWmi) {
        g_wmiCache.startBackgroundRead();  // now, so the first list is (nearly) ready when the game first asks
    } else {
        logLine("WMI shortcut: answering from SetupAPI on every request (RealWmi=0)");
    }
    return true;
}

void refreshXInputDeviceList() {
    g_wmiCache.refresh();
}
