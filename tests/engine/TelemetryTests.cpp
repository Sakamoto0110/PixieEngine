#include "Check.hpp"
#include "pixie/core/Telemetry.hpp"
#include "pixie/app/RunOptions.hpp"
#include "DebugConsole.hpp"
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {
pixie::Telemetry::TimePoint now;
int reads = 0;
pixie::Telemetry::TimePoint FakeClock() { ++reads; return now; }
void Advance(int ms) { now += std::chrono::milliseconds(ms); }
}

TEST(TelemetryUsesExactAggregationStableRegistrationAndIndependentCollectors) {
    pixie::Telemetry a,b;
    const auto first = a.RegisterTiming("work"), other = b.RegisterTiming("work");
    const auto& stable = a.Timing(first);
    CHECK(first == a.RegisterTiming("work"));
    for (int i=0;i<1000;++i) a.RegisterGauge("gauge"+std::to_string(i));
    CHECK(&stable == &a.Timing(first));
    CHECK(!a.RecordTiming(first,2));
    a.SetEnabled(true); b.SetEnabled(true);
    CHECK(a.RecordTiming(first,2) && a.RecordTiming(first,8));
    CHECK(stable.samples==2 && stable.lastMs==8 && stable.meanMs==5 && stable.totalMs==10 && stable.peakMs==8);
    CHECK(!a.RecordTiming(other,1) && b.Timing(other).samples==0);
    bool rejected=false;
    try { a.RegisterGauge("work"); } catch (const std::invalid_argument&) { rejected=true; }
    CHECK(rejected);
    rejected=false;
    try { a.RegisterGauge(""); } catch (const std::invalid_argument&) { rejected=true; }
    CHECK(rejected);
    CHECK(!a.RecordTiming(first,-1) && !a.RecordTiming(first,std::numeric_limits<double>::infinity()));
}

TEST(TelemetryScopesAreInclusiveAndDisabledScopesReadNoClock) {
    pixie::Telemetry t(&FakeClock);
    const auto outer=t.RegisterTiming("outer"),inner=t.RegisterTiming("inner");
    reads=0;now={};
    { const pixie::TelemetryScope ignored(t,outer); Advance(10); }
    CHECK(reads==0 && t.Timing(outer).samples==0);
    t.SetEnabled(true);
    { const pixie::TelemetryScope scope(t,outer); Advance(2);
      { const pixie::TelemetryScope nested(t,inner); Advance(3); }
      Advance(4); }
    CHECK(reads==4 && t.Timing(outer).lastMs==9 && t.Timing(inner).lastMs==3);
    { const pixie::TelemetryScope ignored(t,outer); t.Reset(); Advance(7); }
    CHECK(t.Timing(outer).samples==0 && reads==5);
    { const pixie::TelemetryScope ignored(t,outer); t.SetEnabled(false);t.SetEnabled(true);Advance(9); }
    CHECK(t.Timing(outer).samples==0 && reads==6);
}

TEST(TelemetryGaugesCountersAndCsvKeepUnitsPrecisionAndSnapshotIsolation) {
    pixie::Telemetry t;
    const auto timing=t.RegisterTiming("cpu\"time"),gauge=t.RegisterGauge("temperature","C"),counter=t.RegisterCounter("events");
    t.SetEnabled(true);
    CHECK(t.SetGauge(gauge,-12.5) && !t.SetGauge(gauge,std::nan("")));
    CHECK(t.Increment(counter,std::numeric_limits<std::uint64_t>::max()));
    CHECK(!t.Increment(counter));
    CHECK(!t.RecordTiming(gauge,2) && !t.SetGauge(counter,2) && !t.Increment(timing));
    CHECK(t.RecordTiming(timing,4));
    const auto snapshot=t.Snapshot();
    t.Reset();
    CHECK(snapshot.size()==3 && snapshot[2].counter==std::numeric_limits<std::uint64_t>::max());
    CHECK(snapshot[0].timing.samples==1 && t.Timing(timing).samples==0);
    t.RecordTiming(timing,3);t.Increment(counter,std::numeric_limits<std::uint64_t>::max());
    std::ostringstream csv;t.WriteCsv(csv);
    CHECK(csv.str().find("\"cpu\"\"time\",timing,\"ms\",3,1,3,3,3")!=std::string::npos);
    CHECK(csv.str().find("18446744073709551615")!=std::string::npos);
    t.SetEnabled(false);
    CHECK(!t.SetGauge(gauge,2) && !t.Increment(counter,0));
    CHECK(t.Snapshot()[2].counter==std::numeric_limits<std::uint64_t>::max());
}

TEST(TelemetryCollectionControlsDoNotChangeDisplayOrRateSettings) {
    pixie::DebugConsole console(true,60);
    CHECK(!console.CurrentSettings().telemetry.has_value());
    CHECK(console.Execute("-telemetry true") && console.CurrentSettings().telemetry.value());
    CHECK(!console.ShowsRates() && console.CurrentSettings().vsync && console.CurrentSettings().frameRateCap==60);
    CHECK(!console.Execute("-telemetry maybe") && console.CurrentSettings().telemetry.value());
    CHECK(console.Execute("-telemetry false") && !console.CurrentSettings().telemetry.value());
    pixie::RunConfig config;
    char name[]="test",option[]="--telemetry",value[]="on";char* args[]={name,option,value};
    CHECK(pixie::ParseRunOptions(3,args,"test",config) && config.telemetry.value());
}
