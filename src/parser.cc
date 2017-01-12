#include <yasc/yasc.h>

#include <pegtl.hh>
#include <yasc/yasc.h>

namespace yasc
{
namespace syntax
{

using namespace pegtl;

struct sign : one<'+', '-'> {};
struct nonzero_digit : ranges<'1', '9'> {};
struct decimal_literal : sor<one<'0'>, seq<nonzero_digit, star<digit>>> {};
struct signed_numeric_literal : seq<opt<sign>, decimal_literal> {};
struct grammar : must<signed_numeric_literal, eof> {};

}

template <typename Rule>
struct action : pegtl::nothing<Rule> {};

template<>
struct action<syntax::signed_numeric_literal>
{
	template <typename Input>
	static void apply(Input const& in, std::string& out)
	{
		out = in.string();
	}
};

std::string parse_query(string_view s)
{
	std::string r;
	pegtl::parse_memory<syntax::grammar, action>(
	    s.data(), s.data() + s.size(), "<input>", r);
	return r;
}

}
