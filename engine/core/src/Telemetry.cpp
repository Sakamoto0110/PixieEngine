#include "pixie/core/Telemetry.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace pixie {
namespace {
std::string CsvField(std::string_view text) {
    std::string result = "\"";
    for (const char c : text) {
        if (c == '"') result += '"';
        result += c;
    }
    return result + '"';
}
} // namespace

Telemetry::Telemetry(Clock clock) : clock_(clock) {
    if (!clock_) throw std::invalid_argument("Telemetry requires a clock");
}

MetricHandle Telemetry::Register(std::string_view name, MetricKind kind, std::string_view unit) {
    if (name.empty()) throw std::invalid_argument("A metric needs a name");
    for (std::size_t i = 0; i < metrics_.size(); ++i) if (metrics_[i].name == name) {
        if (metrics_[i].kind != kind || metrics_[i].unit != unit)
            throw std::invalid_argument("A metric name already has another kind or unit");
        MetricHandle handle; handle.owner_ = this; handle.index_ = i; return handle;
    }
    MetricSnapshot metric;
    metric.name = name; metric.unit = unit; metric.kind = kind;
    metrics_.push_back(std::move(metric));
    MetricHandle handle; handle.owner_ = this; handle.index_ = metrics_.size()-1; return handle;
}

MetricHandle Telemetry::RegisterTiming(std::string_view name) { return Register(name,MetricKind::Timing,"ms"); }
MetricHandle Telemetry::RegisterGauge(std::string_view name,std::string_view unit) { return Register(name,MetricKind::Gauge,unit); }
MetricHandle Telemetry::RegisterCounter(std::string_view name,std::string_view unit) { return Register(name,MetricKind::Counter,unit); }

bool Telemetry::Owns(MetricHandle handle,MetricKind kind) const {
    return handle.owner_ == this && handle.index_ < metrics_.size() && metrics_[handle.index_].kind == kind;
}

void Telemetry::SetEnabled(bool enabled) {
    if (enabled_ != enabled) { enabled_ = enabled; ++epoch_; }
}

void Telemetry::Reset() {
    ++epoch_;
    for (auto& metric : metrics_) { metric.timing = {}; metric.gauge = 0.0; metric.counter = 0; }
}

bool Telemetry::RecordTiming(MetricHandle handle,double milliseconds) {
    if (!enabled_ || !Owns(handle,MetricKind::Timing) || !std::isfinite(milliseconds) || milliseconds < 0) return false;
    auto& t = metrics_[handle.index_].timing;
    if (t.samples == std::numeric_limits<std::uint64_t>::max() || !std::isfinite(t.totalMs+milliseconds)) return false;
    t.lastMs = milliseconds;
    t.totalMs += milliseconds;
    ++t.samples;
    t.meanMs = t.totalMs / static_cast<double>(t.samples);
    t.peakMs = std::max(t.peakMs,milliseconds);
    return true;
}

bool Telemetry::SetGauge(MetricHandle handle,double value) {
    if (!enabled_ || !Owns(handle,MetricKind::Gauge) || !std::isfinite(value)) return false;
    metrics_[handle.index_].gauge = value; return true;
}

bool Telemetry::Increment(MetricHandle handle,std::uint64_t amount) {
    if (!enabled_ || !Owns(handle,MetricKind::Counter)) return false;
    auto& counter = metrics_[handle.index_].counter;
    if (amount > std::numeric_limits<std::uint64_t>::max()-counter) return false;
    counter += amount; return true;
}

const TimingStats& Telemetry::Timing(MetricHandle handle) const {
    if (!Owns(handle,MetricKind::Timing)) throw std::invalid_argument("Not a timing handle of this collector");
    return metrics_[handle.index_].timing;
}

std::vector<MetricSnapshot> Telemetry::Snapshot() const { return {metrics_.begin(),metrics_.end()}; }

void Telemetry::WriteCsv(std::ostream& output) const {
    std::ostringstream text;
    text.imbue(std::locale::classic());
    text << std::setprecision(17) << "name,kind,unit,value,samples,mean_ms,peak_ms,total_ms\n";
    for (const auto& metric : metrics_) {
        text << CsvField(metric.name) << ',';
        const auto& t = metric.timing;
        if (metric.kind == MetricKind::Timing)
            text << "timing," << CsvField(metric.unit) << ',' << t.lastMs << ',' << t.samples << ',' << t.meanMs << ',' << t.peakMs << ',' << t.totalMs;
        else if (metric.kind == MetricKind::Gauge)
            text << "gauge," << CsvField(metric.unit) << ',' << metric.gauge << ",,,,";
        else text << "counter," << CsvField(metric.unit) << ',' << metric.counter << ",,,,";
        text << '\n';
    }
    output << text.str();
}

TelemetryScope::TelemetryScope(Telemetry& telemetry,MetricHandle timing) {
    if (!telemetry.Enabled() || !telemetry.Owns(timing,MetricKind::Timing)) return;
    telemetry_ = &telemetry; timing_ = timing; epoch_ = telemetry.epoch_;
    start_ = telemetry.clock_();
}

TelemetryScope::~TelemetryScope() {
    if (telemetry_ && telemetry_->Enabled() && telemetry_->epoch_ == epoch_) {
        const auto end = telemetry_->clock_();
        telemetry_->RecordTiming(timing_,std::chrono::duration<double,std::milli>(end-start_).count());
    }
}

} // namespace pixie
