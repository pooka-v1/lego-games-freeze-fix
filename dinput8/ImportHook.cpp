#include "ImportHook.h"

#include <cstring>

void* hookImport(HMODULE module, const char* dllName, const char* functionName, void* replacement) {
    // Every field of the PE format is an offset (RVA) from the module's base address.
    auto* base = reinterpret_cast<BYTE*>(module);
    const auto* dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto* ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dosHeader->e_lfanew);
    const IMAGE_DATA_DIRECTORY& imports = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (imports.VirtualAddress == 0) return nullptr;

    // One descriptor per imported DLL; the list ends with an all-zero descriptor.
    for (auto* dll = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + imports.VirtualAddress); dll->Name; ++dll) {
        if (_stricmp(reinterpret_cast<const char*>(base + dll->Name), dllName) != 0) continue;
        // Two parallel arrays: OriginalFirstThunk holds the function names (never changes),
        // FirstThunk is the IAT itself (the addresses Windows filled in).
        if (dll->OriginalFirstThunk == 0) return nullptr;  // no names to match against
        auto* name = reinterpret_cast<const IMAGE_THUNK_DATA*>(base + dll->OriginalFirstThunk);
        auto* address = reinterpret_cast<IMAGE_THUNK_DATA*>(base + dll->FirstThunk);

        for (; name->u1.AddressOfData != 0; ++name, ++address) {
            if (IMAGE_SNAP_BY_ORDINAL(name->u1.Ordinal)) continue;  // imported by number, not by name
            const auto* byName = reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(base + name->u1.AddressOfData);
            if (strcmp(reinterpret_cast<const char*>(byName->Name), functionName) != 0) continue;

            void* original = reinterpret_cast<void*>(address->u1.Function);
            // The IAT usually lives in read-only memory: unlock it, write, restore the protection.
            DWORD oldProtection = 0;
            if (!VirtualProtect(&address->u1.Function, sizeof(address->u1.Function), PAGE_READWRITE,
                                &oldProtection)) {
                return nullptr;
            }
            address->u1.Function = reinterpret_cast<decltype(address->u1.Function)>(replacement);
            VirtualProtect(&address->u1.Function, sizeof(address->u1.Function), oldProtection, &oldProtection);
            return original;
        }
    }
    return nullptr;
}
