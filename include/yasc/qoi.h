#pragma once

#include <stdex/string_view.h>

#if defined(_MSC_VER)
#include <ciso646>
#endif

namespace yasc
{
using stdex::string_view;
using stdex::wstring_view;
using stdex::u16string_view;
using stdex::u32string_view;
using stdex::basic_string_view;
}
