#include "RealWmi.h"

#include <wbemidl.h>
#include <wrl/client.h>  // Microsoft::WRL::ComPtr

#include "Log.h"

namespace {

// ComPtr releases the COM object when it goes out of scope (like try-with-resources in Java), so the
// early returns below cannot leak a reference.
using Microsoft::WRL::ComPtr;

// WMI takes its names as BSTR (a length-prefixed string allocated by COM), not as plain wchar_t*.
// Same idea as ComPtr: freed when it goes out of scope. Not copyable, so it cannot be freed twice.
class Bstr {
public:
    explicit Bstr(const wchar_t* text) : m_value(SysAllocString(text)) {}
    ~Bstr() { SysFreeString(m_value); }
    Bstr(const Bstr&) = delete;
    Bstr& operator=(const Bstr&) = delete;
    BSTR get() const { return m_value; }

private:
    BSTR m_value;
};

constexpr ULONG kBatchSize = 20;      // devices per Next() call, as in Microsoft's sample
constexpr long kNextTimeoutMs = 10000;

}  // namespace

bool readXInputIdsFromWmi(CoCreateInstanceFn coCreateInstance, std::vector<std::wstring>& ids) {
    ids.clear();
    ComPtr<IWbemLocator> locator;
    HRESULT hr = coCreateInstance(__uuidof(WbemLocator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&locator));
    if (FAILED(hr)) {
        logLine("Real WMI: cannot create the locator (0x%08lX)", hr);
        return false;
    }

    const Bstr nameSpace(L"\\\\.\\root\\cimv2");
    ComPtr<IWbemServices> services;
    hr = locator->ConnectServer(nameSpace.get(), nullptr, nullptr, nullptr, 0, nullptr, nullptr, &services);
    if (FAILED(hr)) {
        logLine("Real WMI: cannot connect (0x%08lX)", hr);
        return false;
    }
    // Same security settings as the sample the game copied. Its result is ignored there too.
    CoSetProxyBlanket(services.Get(), RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL,
                      RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

    const Bstr className(L"Win32_PNPEntity");
    ComPtr<IEnumWbemClassObject> devices;
    hr = services->CreateInstanceEnum(className.get(), 0, nullptr, &devices);
    if (FAILED(hr)) {
        logLine("Real WMI: Win32_PNPEntity query failed (0x%08lX)", hr);
        return false;
    }

    for (;;) {
        IWbemClassObject* batch[kBatchSize] = {};
        ULONG returned = 0;
        hr = devices->Next(kNextTimeoutMs, kBatchSize, batch, &returned);
        for (ULONG i = 0; i < returned; ++i) {
            VARIANT value;
            VariantInit(&value);
            if (SUCCEEDED(batch[i]->Get(L"DeviceID", 0, &value, nullptr, nullptr)) && value.vt == VT_BSTR &&
                value.bstrVal && wcsstr(value.bstrVal, L"IG_")) {
                ids.emplace_back(value.bstrVal);
            }
            VariantClear(&value);  // frees the BSTR WMI allocated for us
            batch[i]->Release();
        }
        if (hr == WBEM_S_FALSE) return true;  // "fewer than asked for": the list is finished
        if (hr != WBEM_S_NO_ERROR) {          // an error, or WBEM_S_TIMEDOUT: the list is incomplete
            logLine("Real WMI: enumeration stopped halfway (0x%08lX)", hr);
            return false;
        }
    }
}
