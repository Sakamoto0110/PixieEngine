#pragma once

#include "pixie/core/Export.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace pixie {

class Telemetry;
enum class MetricKind { Timing, Gauge, Counter };

struct TimingStats {
    double lastMs = 0.0, meanMs = 0.0, peakMs = 0.0, totalMs = 0.0;
    std::uint64_t samples = 0;
};

// A handle belongs to one collector and remains valid through Reset and new registrations.
// Handles, references to timing stats and active scopes must not outlive their collector.
class MetricHandle {
public:
    bool operator==(const MetricHandle&) const = default;
private:
    friend class Telemetry;
    const Telemetry* owner_ = nullptr;
    std::size_t index_ = 0;
};

struct MetricSnapshot {
    std::string name, unit;
    MetricKind kind = MetricKind::Timing;
    TimingStats timing;
    double gauge = 0.0;
    std::uint64_t counter = 0;
};

// Main-thread-only CPU telemetry, independent of rendering and simulation time. No global collector.
// Register stable names outside hot loops. Re-registering the same name/kind/unit returns its handle;
// conflicting or empty registrations throw invalid_argument. Storage grows only during registration.
class Telemetry {
public:
    using TimePoint = std::chrono::steady_clock::time_point;
    using Clock = TimePoint (*)();
    PIXIE_CORE_API explicit Telemetry(Clock clock = &std::chrono::steady_clock::now);
    Telemetry(const Telemetry&) = delete;
    Telemetry& operator=(const Telemetry&) = delete;

    PIXIE_CORE_API MetricHandle RegisterTiming(std::string_view name);
    PIXIE_CORE_API MetricHandle RegisterGauge(std::string_view name, std::string_view unit = {});
    PIXIE_CORE_API MetricHandle RegisterCounter(std::string_view name, std::string_view unit = "count");

    // Disabled by default. Visibility never changes this setting. Transitions and Reset invalidate
    // active timing scopes, so partial samples cannot span a collection/reset boundary.
    PIXIE_CORE_API void SetEnabled(bool enabled);
    bool Enabled() const { return enabled_; }
    PIXIE_CORE_API void Reset();

    // False on disabled collection, foreign/wrong-kind handles, nonfinite/negative durations or
    // overflow. Gauges may be negative; counters are exact uint64 values and never wrap.
    PIXIE_CORE_API bool RecordTiming(MetricHandle handle, double milliseconds);
    PIXIE_CORE_API bool SetGauge(MetricHandle handle, double value);
    PIXIE_CORE_API bool Increment(MetricHandle handle, std::uint64_t amount = 1);
    PIXIE_CORE_API const TimingStats& Timing(MetricHandle handle) const;

    // Copies in registration order. Snapshot/CSV work without a window, even after disabling.
    PIXIE_CORE_API std::vector<MetricSnapshot> Snapshot() const;
    PIXIE_CORE_API void WriteCsv(std::ostream& output) const;

private:
    friend class TelemetryScope;
    PIXIE_CORE_API MetricHandle Register(std::string_view name, MetricKind kind, std::string_view unit);
    PIXIE_CORE_API bool Owns(MetricHandle handle, MetricKind kind) const;
    std::deque<MetricSnapshot> metrics_; // End insertion keeps Timing() references stable.
    Clock clock_;
    std::uint64_t epoch_ = 0;
    bool enabled_ = false;
};

// Inclusive wall-clock CPU duration. Nested scopes overlap; do not sum parent and child timings.
// Disabled/invalid scopes read no clock. Enabled sampling does not allocate or resolve a name.
class TelemetryScope {
public:
    PIXIE_CORE_API TelemetryScope(Telemetry& telemetry, MetricHandle timing);
    PIXIE_CORE_API ~TelemetryScope();
    TelemetryScope(const TelemetryScope&) = delete;
    TelemetryScope& operator=(const TelemetryScope&) = delete;
private:
    Telemetry* telemetry_ = nullptr;
    MetricHandle timing_;
    Telemetry::TimePoint start_{};
    std::uint64_t epoch_ = 0;
};

} // namespace pixie
