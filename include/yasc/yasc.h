#pragma once

#include "qoi.h"

#include <vector>

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

struct SearchCondition
{
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
