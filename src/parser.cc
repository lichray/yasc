#include "parser.h"

#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include <system_error>

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
using unescape_adjacent = star<sor<not_one<c>, two<c>>>;

struct delimited_identifier_body : unescape_adjacent<'"'> {};
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
struct digit_sequence : star<digit> {};

struct decimal_literal : sor<one<'0'>, seq<nonzero_digit, digit_sequence>> {};
struct integer_literal : seq<opt<sign>, decimal_literal> {};

struct fractional_constant :
	sor
	<
	    seq<opt<digit_sequence>, one<'.'>, digit_sequence>,
	    seq<digit_sequence, one<'.'>>
	>
{};
struct exponent_part : seq<one<'e', 'E'>, opt<sign>, digit_sequence> {};
struct decimal_floating_literal :
	sor
	<
	    seq<fractional_constant, opt<exponent_part>>,
	    seq<digit_sequence, exponent_part>
	>
{};
struct floating_literal : seq<opt<sign>, decimal_floating_literal> {};

struct characters : unescape_adjacent<'\''> {};
struct character_string_literal :
	list<if_must<one<'\''>, characters, one<'\''>>, plus<separator>>
{};

struct column_name : identifier {};
struct column_reference :
	seq<opt<table_name, one<'.'>>, column_name>
{};

struct row_value_expression :
	sor
	<
	    column_reference,
	    character_string_literal,
	    floating_literal,
	    integer_literal
	>
{};

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

struct not_null : pegtl_istring_t("not") {};
struct null_predicate :
	seq
	<
	    row_value_expression, seps,
	    pegtl_istring_t("is"), seps,
	    opt<not_null, seps>,
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
	static void apply(Input const&, states& st, Query& q)
	{
		q.distinct = true;
	}
};

template <>
struct action<syntax::select_sublist>
{
	template <typename Input>
	static void apply(Input const& in, states& st, Query& q)
	{
		q.select.append(in.string());
	}
};

template <>
struct action<syntax::table_reference>
{
	template <typename Input>
	static void apply(Input const& in, states& st, Query& q)
	{
		q.from.append(in.string());
	}
};

template <>
struct action<syntax::comp_op>
{
	template <typename Input>
	static void apply(Input const& in, states& st, Query& q)
	{
		auto c = in.peek_char();
		if (c == '=')
			st.pending = xpr::op::equals;
		else if (in.size() == 2)
		{
			auto c2 = in.peek_char(1);
			if (c2 == '>')
				st.pending = xpr::op::not_equals;
			else if (c == '<')
				st.pending = xpr::op::less_than_or_equals;
			else if (c == '>')
				st.pending = xpr::op::greater_than_or_equals;
		}
		else if (c == '<')
			st.pending = xpr::op::less_than;
		else if (c == '>')
			st.pending = xpr::op::greater_than;
	}
};


template <>
struct action<syntax::not_null>
{
	template <typename Input>
	static void apply(Input const& in, states& st, Query&)
	{
		st.pending = xpr::op::is_not_null;
	}
};

template <>
struct action<syntax::null_predicate>
{
	template <typename Input>
	static void apply(Input const& in, states& st, Query&)
	{
		if (st.pending != xpr::op::is_not_null)
			st.pending = xpr::op::is_null;
	}
};

template <>
struct action<syntax::integer_literal>
{
	template <typename Input>
	static void apply(Input const& in, states& st, Query&)
	{
		auto p = const_cast<char*>(in.end());
		int64_t v = strtoll(in.begin(), &p, 10);
		st.vstack.emplace_back(v);
	}
};

template <>
struct action<syntax::floating_literal>
{
	template <typename Input>
	static void apply(Input const& in, states& st, Query&)
	{
		auto p = const_cast<char*>(in.end());
		double v = strtod(in.begin(), &p);
		st.vstack.emplace_back(v);
	}
};

template <>
struct action<syntax::characters>
{
	template <typename Input>
	static void apply(Input const& in, states& st, Query&)
	{
		auto b = in.begin();
		for (;;)
		{
			auto e = std::find(b, in.end(), '\'');
			if (e == in.end())
			{
				st.unescaped.append(b, e);
				break;
			}

			st.unescaped.append(b, ++e);
			b = ++e;
		}
	}
};

template <>
struct action<syntax::character_string_literal>
{
	template <typename Input>
	static void apply(Input const& in, states& st, Query&)
	{
		st.vstack.emplace_back(std::move(st.unescaped));
	}
};

template <>
struct action<syntax::column_reference>
{
	template <typename Input>
	static void apply(Input const& in, states& st, Query&)
	{
		st.vstack.emplace_back(xpr::column{ in.string() });
	}
};

template <>
struct action<syntax::boolean_test>
{
	template <typename Input>
	static void apply(Input const& in, states& st, Query& q)
	{
		if (st.pending == xpr::op::is_null or
		    st.pending == xpr::op::is_not_null)
			st.pstack.emplace_back(st.pending,
			                       std::move(st.vstack.back()));
		else
		{
			auto y = std::move(st.vstack.back());
			st.vstack.pop_back();
			st.pstack.emplace_back(st.pending,
			                       std::move(st.vstack.back()),
			                       std::move(y));
		}
		st.vstack.pop_back();
	}
};

template <logical::op eta>
struct condition_action
{
	template <typename Input>
	static void apply(Input const& in, states& st, Query& q)
	{
		if (st.pstack.size() > 1)
		{
			auto g = std::make_unique<logical::lambda>(eta);
			swap(g->args, st.pstack);
			q.where.f->args.emplace_back(std::move(g));
		}
		else if (st.pstack.size() == 1)
		{
			q.where.f->args.push_back(
			    std::move(st.pstack.back()));
			st.pstack.pop_back();
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
	static void apply(Input const& in, states& st, Query& q)
	{
		if (not st.pstack.empty())
		{
			auto g = std::make_unique<logical::lambda>(
			    logical::op::negation);
			g->args.emplace_back(std::move(st.pstack.back()));
			st.pstack.pop_back();
			st.pstack.emplace_back(std::move(g));
		}
	}
};

template <>
struct action<syntax::where_clause>
{
	template <typename Input>
	static void apply(Input const& in, states& st, Query& q)
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

template <>
struct action<syntax::statement>
{
	template <typename Input>
	static void apply(Input const& in, states& st, Query& q)
	{
		st.callback(std::move(q));
		q = {};
	}
};

Query parse_query(string_view s)
{
	states st;
	Query q;
	pegtl::parse_memory<syntax::query_grammar, action>(
	    s.data(), s.data() + s.size(), "<input>", st, q);
	return q;
}

void parse_script(char const* name, query_handler_t cb)
{
	auto file_open = [](char const* fn, char const* mode) {
		FILE* in;
#if defined(WIN32)
		if (fopen_s(&in, fn, mode) != 0)
#else
		if ((in = fopen(fn, mode)) == nullptr)
#endif
			throw std::system_error(errno, std::system_category());

		return std::unique_ptr<FILE, decltype(&fclose)>(in, fclose);
	};

	Query q;
	states st = { cb };

	if (name[0] == '-' and name[1] == '\0')
		pegtl::parse_cstream<syntax::grammar, action>(
		    stdin, "<stdin>", 2048, st, q);
	else
	{
		auto fp = file_open(name, "r");
		pegtl::parse_cstream<syntax::grammar, action>(
		    fp.get(), name, 2048, st, q);
	}
}

void logical::lambda::walk(signature<void(xpr::predicate&)> cb)
{
	for (auto& a : args)
	{
		if (a.f == nullptr)
			cb(a.test);
		else
			a.f->walk(cb);
	}
}

}
