#pragma once

#include <windows.h>

// Redirects one function that "module" imports from "dllName" to "replacement", by rewriting the
// module's Import Address Table (IAT). Returns the original function, or nullptr if not found.
//
// Why this works: an exe does not call DLL functions directly. Each call jumps through a table of
// addresses that Windows fills in at load time. Changing one entry of that table changes what every
// call site in that module reaches, without touching the code itself. Other modules are not affected.
void* hookImport(HMODULE module, const char* dllName, const char* functionName, void* replacement);
