#pragma once

// Native consumers share the compiler, standard library ABI and dynamic C++ runtime.
#if defined(_WIN32)
#if defined(PIXIE_APP_BUILD)
#define PIXIE_APP_API __declspec(dllexport)
#else
#define PIXIE_APP_API __declspec(dllimport)
#endif
#else
#define PIXIE_APP_API __attribute__((visibility("default")))
#endif
