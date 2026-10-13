#pragma once

// A minimal test harness: TEST registers a function, CHECK records a failure and goes on. Each test
// executable runs every registered test and exits 1 if any check failed.

#include <cstdio>
#include <functional>
#include <string_view>
#include <vector>

namespace check {

struct Registry {
    struct Test {
        std::string_view name;
        std::function<void()> run;
    };
    std::vector<Test> tests;
    int failures = 0;

    static Registry& Get() {
        static Registry registry;
        return registry;
    }
};

struct Registrar {
    Registrar(std::string_view name, std::function<void()> run) {
        Registry::Get().tests.push_back({name, std::move(run)});
    }
};

inline void Fail(const char* file, int line, const char* expression) {
    std::fprintf(stderr, "%s(%d): check failed: %s\n", file, line, expression);
    ++Registry::Get().failures;
}

inline int RunAll() {
    Registry& registry = Registry::Get();
    for (const auto& test : registry.tests) {
        const int before = registry.failures;
        test.run();
        std::fprintf(stderr, "[%s] %.*s\n", registry.failures == before ? "pass" : "FAIL",
                     static_cast<int>(test.name.size()), test.name.data());
    }
    std::fprintf(stderr, "%zu tests, %d failed checks\n", registry.tests.size(), registry.failures);
    return registry.failures == 0 ? 0 : 1;
}

} // namespace check

#define CHECK_CONCAT_INNER(a, b) a##b
#define CHECK_CONCAT(a, b) CHECK_CONCAT_INNER(a, b)

#define TEST(name)                                                                                 \
    static void name();                                                                            \
    static const check::Registrar CHECK_CONCAT(registrar_, name){#name, name};                     \
    static void name()

#define CHECK(expression)                                                                          \
    do {                                                                                           \
        if (!(expression)) {                                                                       \
            check::Fail(__FILE__, __LINE__, #expression);                                          \
        }                                                                                          \
    } while (false)
