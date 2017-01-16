#pragma once

#include <stdex/string_view.h>
#include <stdex/functional.h>
#include <mapbox/variant.hpp>

#if defined(_MSC_VER)
#include <ciso646>
#endif

namespace yasc
{
using stdex::string_view;
using stdex::signature;
using mapbox::util::variant;
}
