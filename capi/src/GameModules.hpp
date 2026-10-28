#pragma once
#include "pixie/runtime/Project.hpp"
#include "pixie/runtime/SceneProject.hpp"
#include "pixie_capi.h"

#ifdef PIXIE_DEVELOPMENT
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

// Sessions keep this lease before their ProjectSession member, so every game-owned
// object, descriptor, callback and storage dies before its code can be unloaded.
struct PixieGameModule {
#ifdef _WIN32
    HMODULE handle = nullptr;
    ~PixieGameModule() { if (handle) FreeLibrary(handle); }
    void* Symbol(const char* name) const { return reinterpret_cast<void*>(GetProcAddress(handle, name)); }
#else
    void* handle = nullptr;
    ~PixieGameModule() { if (handle) dlclose(handle); }
    void* Symbol(const char* name) const { return dlsym(handle, name); }
#endif
    pixie::Project* (*create)() = nullptr;
};
#endif
