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

namespace logical
{

enum struct op
{
	identity, conjunction, disjunction, negation
};

struct lambda;
struct argument
{
	explicit argument(std::string s) : test(std::move(s)) {}
	explicit argument(std::unique_ptr<lambda>&& e) : f(std::move(e)) {}

	std::unique_ptr<lambda> f;
	std::string test;
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
