#include <yasc/yasc.h>

#include <array>

#include <rapidjson/internal/dtoa.h>
#include <rapidjson/internal/itoa.h>

namespace yasc
{

namespace internal = rapidjson::internal;

namespace xpr
{

#define cb_ntbs(s) cb(s, sizeof(s) - 1)

void print(row_value const& v, writeall_t cb)
{
	using yasc::print;
	v.match(
	    [&](int64_t n) {
		    char buf[21];
		    auto e = internal::i64toa(n, buf);
		    cb(buf, size_t(e - buf));
	    },
	    [&](double d) {
		    char buf[25];
		    auto e = internal::dtoa(d, buf, 324);
		    cb(buf, size_t(e - buf));
	    },
	    [&](string_view sv) {
		    auto half_to_hex = [](int c) {
			    return char((c > 9) ? c + 'a' - 10 : c + '0');
		    };

		    cb_ntbs("\"");

		    auto bp = sv.data();
		    auto p = bp;

		    for (; p != sv.data() + sv.size(); ++p)
		    {
			    auto c = *p;
			    if (0x1f < c and c < 0x7f and c != '"' and
			        c != '\\')
				    continue;

			    cb(bp, size_t(p - bp));
			    bp = p + 1;
			    if (c == '\t')
				    cb_ntbs("\\t");
			    else if (c == '\n')
				    cb_ntbs("\\n");
			    else if (c == '\r')
				    cb_ntbs("\\r");
			    else if (c == '"')
				    cb_ntbs("\\\"");
			    else if (c == '\\')
				    cb_ntbs("\\\\");
			    else
			    {
				    cb_ntbs("\\x");
				    std::array<char, 2> buf;
				    buf[0] = half_to_hex((c >> 4) & 0xf);
				    buf[1] = half_to_hex(c & 0xf);
				    cb(buf.data(), buf.size());
			    }
		    }
		    if (auto off = size_t(p - bp))
			    cb(bp, off);

		    cb_ntbs("\"");
	    },
	    [&](column const& cl) { print(cl.id, cb); });
}

void print(predicate const& r, writeall_t cb)
{

	print(r.x, cb);
	switch (r.f)
	{
	case xpr::op::is_null: cb_ntbs(" is null"); return;
	case xpr::op::is_not_null: cb_ntbs(" is not null"); return;
	case xpr::op::equals: cb_ntbs(" = "); break;
	case xpr::op::not_equals: cb_ntbs(" <> "); break;
	case xpr::op::less_than: cb_ntbs(" < "); break;
	case xpr::op::greater_than: cb_ntbs(" > "); break;
	case xpr::op::less_than_or_equals: cb_ntbs(" <= "); break;
	case xpr::op::greater_than_or_equals: cb_ntbs(" >= "); break;
	}
	print(r.y, cb);

}

#undef cb_ntbs

}

}
