#pragma once
#include "pixie_capi.h"
#include <string>

namespace pixie::dev {
// Internal to the shared engine; not an additional C entry point.
PixieResult SetNativeError(PixieResult result, const std::string& message);
}
