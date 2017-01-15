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

template <typename S, typename O>
struct left_assoc : seq<S, seps, star<if_must<O, seps, S, seps>>>
{};

template <typename S, typename O>
struct right_assoc :
	seq<S, seps, opt<if_must<O, seps, right_assoc<S, O>>>>
{};

template <char O>
struct op_one : seq<one<O>, at<not_one<O>>> {};

struct str_distinct : pegtl_istring_t("distinct") {};
struct set_quantifier : sor<str_distinct, pegtl_istring_t("all")> {};

template <char c>
using unescape_adjacent = sor<not_one<c>, two<c>>;

struct delimited_identifier_part : unescape_adjacent<'"'> {};
struct delimited_identifier_body : star<delimited_identifier_part> {};
struct delimited_identifier :
	if_must<one<'"'>, delimited_identifier_body, one<'"'>>
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

struct sign : one<'+', '-'> {};
struct nonzero_digit : ranges<'1', '9'> {};
struct decimal_literal :
	sor<one<'0'>, seq<nonzero_digit, star<digit>>>
{};
struct signed_numeric_literal : seq<opt<sign>, decimal_literal> {};

struct literal : signed_numeric_literal {};

struct column_name : identifier {};
struct column_reference :
	seq<opt<table_name, one<'.'>>, column_name>
{};
struct row_value_expression : sor<column_reference, literal> {};

struct comp_op :
	sor
	<
	    op_one<'='>,
	    string<'<', '>'>,
	    string<'<', '='>,
	    op_one<'<'>,
	    string<'>', '='>,
	    op_one<'>'>
	>
{};

struct comparison_predicate :
	seq
	<
	    row_value_expression, seps,
	    comp_op, seps,
	    row_value_expression
	>
{};

struct null_predicate :
	seq
	<
	    row_value_expression, seps,
	    pegtl_istring_t("is"), seps,
	    opt<pegtl_istring_t("not"), seps>,
	    pegtl_istring_t("null")
	>
{};

struct predicate : sor<comparison_predicate, null_predicate> {};

struct search_condition;
struct boolean_primary : predicate {};
/*
struct boolean_primary :
	sor
	<
	    predicate,
	    if_must<one<'('>, seps, search_condition, one<')'>>
	>
{};
*/

struct boolean_test : boolean_primary {};

struct negative_test : if_must<pegtl_istring_t("not"), seps, boolean_test> {};
struct boolean_factor : sor<negative_test, boolean_test> {};

struct boolean_term :
	left_assoc<boolean_factor, pegtl_istring_t("and")>
{};

struct search_condition :
	left_assoc<boolean_term, pegtl_istring_t("or")>
{};

struct where_clause :
	must<pegtl_istring_t("where"), seps, search_condition>
{};

struct table_expression : seq<from_clause, opt<where_clause>> {};

struct query_specification :
	must
	<
	    pegtl_istring_t("select"), seps,
	    opt<set_quantifier, seps>, select_list, table_expression
	>
{};

struct statement : seq<query_specification, one<';'>, seps> {};
struct grammar : must<seps, star<statement>, eof> {};

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

template <>
struct action<syntax::boolean_test>
{
	template <typename Input>
	static void apply(Input const& in, Query& q)
	{
		q.where.pstack.emplace_back(in.string());
	}
};

template <logical::op eta>
struct condition_action
{
	template <typename Input>
	static void apply(Input const& in, Query& q)
	{
		if (q.where.pstack.size() > 1)
		{
			auto g = std::make_unique<logical::lambda>(eta);
			swap(g->args, q.where.pstack);
			q.where.f->args.emplace_back(std::move(g));
		}
		else if (q.where.pstack.size() == 1)
		{
			q.where.f->args.push_back(
			    std::move(q.where.pstack.back()));
			q.where.pstack.pop_back();
		}
		else if (q.where.f->args.size() > 1)
		{
			q.where.f->eta = eta;
			auto g = std::make_unique<logical::lambda>();
			g->args.emplace_back(std::move(q.where.f));
			q.where.f = std::move(g);
		}
	}
};

template <>
struct action<syntax::search_condition> :
	condition_action<logical::op::disjunction>
{};

template <>
struct action<syntax::boolean_term> :
	condition_action<logical::op::conjunction>
{};

template <>
struct action<syntax::negative_test>
{
	template <typename Input>
	static void apply(Input const& in, Query& q)
	{
		if (not q.where.pstack.empty())
		{
			auto g = std::make_unique<logical::lambda>(
			    logical::op::negation);
			g->args.emplace_back(std::move(q.where.pstack.back()));
			q.where.pstack.pop_back();
			q.where.pstack.emplace_back(std::move(g));
		}
	}
};

template <>
struct action<syntax::where_clause>
{
	template <typename Input>
	static void apply(Input const& in, Query& q)
	{
		auto& f = q.where.f;
		if (f->args.empty())
			f.reset();
		else if (f->args.size() == 1 and
		         f->eta == logical::op::identity and
		         f->args.front().f != nullptr)
			swap(f, f->args.front().f);
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
