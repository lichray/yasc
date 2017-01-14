#include <yasc/yasc.h>

#include <pegtl.hh>
#include <pegtl/trace.hh>

namespace yasc
{
namespace syntax
{

using namespace pegtl;

struct simple_comment : seq<two<'-'>, until<eolf>> {};
struct bracketed_comment :
	if_must<string<'/', '*'>, until<string<'*', '/'>>>
{};
struct comment : disable<sor<simple_comment, bracketed_comment>> {};

struct separator : sor<ascii::space, comment> {};
struct seps : star<separator> {};

struct str_distinct : pegtl_istring_t("distinct") {};
struct set_quantifier : sor<str_distinct, pegtl_istring_t("all")> {};

template <char c>
using unescape_adjacent = sor<not_one<c>, two<c>>;

struct delimited_identifier_part : unescape_adjacent<'"'> {};
struct delimited_identifier_body : star<delimited_identifier_part> {};
struct delimited_identifier :
	must<one<'"'>, delimited_identifier_body, one<'"'>>
{};

struct identifier : sor<ascii::identifier, delimited_identifier> {};
struct asterisk : one<'*'> {};
struct derived_column : identifier {};
struct select_sublist : derived_column {};
struct select_list :
	sor
	<
	    seq<asterisk, seps>,
	    list_must<seq<select_sublist, seps>, seq<one<','>, seps>>
	>
{};

struct table_name : identifier {};
struct table_reference : table_name {};

struct from_clause :
	must
	<
	    pegtl_istring_t("from"), seps,
	    list_must<seq<table_reference, seps>, seq<one<','>, seps>>
	>
{};

struct table_expression : seq<from_clause> {};

struct query_specification :
	must
	<
	    pegtl_istring_t("select"), seps,
	    opt<set_quantifier, seps>, select_list, table_expression
	>
{};

struct sign : one<'+', '-'> {};
struct nonzero_digit : ranges<'1', '9'> {};
struct decimal_literal :
	sor<one<'0'>, seq<nonzero_digit, star<digit>>>
{};
struct signed_numeric_literal : seq<opt<sign>, decimal_literal> {};
struct grammar : must<signed_numeric_literal, eof> {};

struct query_grammar : must<seps, query_specification, eof> {};

}

template <typename Rule>
struct action : pegtl::nothing<Rule> {};

template <>
struct action<syntax::str_distinct>
{
	template <typename Input>
	static void apply(Input const&, Query& q)
	{
		q.distinct = true;
	}
};

template <>
struct action<syntax::select_sublist>
{
	template <typename Input>
	static void apply(Input const& in, Query& q)
	{
		q.select.append(in.string());
	}
};

template <>
struct action<syntax::table_reference>
{
	template <typename Input>
	static void apply(Input const& in, Query& q)
	{
		q.from.append(in.string());
	}
};

Query parse_query(string_view s)
{
	Query q;
	pegtl::parse_memory<syntax::query_grammar, action>(
	    s.data(), s.data() + s.size(), "<input>", q);
	return q;
}

}
