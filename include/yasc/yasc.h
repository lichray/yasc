#pragma once

#include "qoi.h"

#include <vector>
#include <memory>

namespace yasc
{

struct SelectList
{
	bool all_columns() const noexcept
	{
		return v_.empty();
	}

	auto begin() const
	{
		return v_.begin();
	}

	auto end() const
	{
		return v_.end();
	}

	decltype(auto) operator[](int i) const
	{
		return v_[size_t(i)];
	}

	auto count() const noexcept
	{
		return v_.size();
	}

	template <typename T>
	void append(T&& x)
	{
		v_.push_back(std::forward<T>(x));
	}

private:
	std::vector<std::string> v_;
};

struct TableReferences
{
	auto begin() const
	{
		return v_.begin();
	}

	auto end() const
	{
		return v_.end();
	}

	auto count() const noexcept
	{
		return v_.size();
	}

	decltype(auto) operator[](int i) const
	{
		return v_[size_t(i)];
	}

	template <typename T>
	void append(T&& x)
	{
		v_.push_back(std::forward<T>(x));
	}

private:
	std::vector<std::string> v_;
};

namespace xpr
{

struct column
{
	std::string id;
};

using row_value = variant<int64_t, double, std::string, column>;

enum struct op
{
	equals,
	not_equals,
	less_than,
	greater_than,
	less_than_or_equals,
	greater_than_or_equals,
	is_null,
	is_not_null,
};

struct predicate
{
	op f;
	xpr::row_value x, y;
};

}

namespace logical
{

enum struct op
{
	identity, conjunction, disjunction, negation
};

struct lambda;
struct argument
{
	explicit argument(xpr::op op, xpr::row_value&& x)
	    : test{ op, std::move(x) }
	{
	}

	explicit argument(xpr::op op, xpr::row_value&& x, xpr::row_value&& y)
	    : test{ op, std::move(x), std::move(y) }
	{
	}

	explicit argument(std::unique_ptr<lambda>&& e) : f(std::move(e)) {}

	std::unique_ptr<lambda> f;
	xpr::predicate test;
};

struct lambda
{
	lambda() = default;
	explicit lambda(op x) : eta(x) {}

	op eta;
	std::vector<argument> args;
};

}

struct SearchCondition
{
	std::unique_ptr<logical::lambda> f =
	    std::make_unique<logical::lambda>();
};

struct Query
{
	SelectList select;
	TableReferences from;
	SearchCondition where;
	bool distinct = false;
};

Query parse_query(string_view s);

}
