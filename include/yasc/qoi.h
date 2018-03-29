#pragma once

#include <stdex/string_view.h>
#include <stdex/functional.h>
#include <stdex/oneof.h>

#if defined(_MSC_VER)
#include <ciso646>
#endif

namespace yasc
{
using stdex::string_view;
using stdex::signature;
using stdex::oneof;
}
