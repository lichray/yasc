#pragma once

#include "qoi.h"

#include <vector>
#include <memory>

namespace yasc
{

using writeall_t = signature<void(char const*, size_t)>;

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
		return int(v_.size());
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
		return int(v_.size());
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

using row_value = oneof<int64_t, double, std::string, column>;

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

inline bool is_unary(op eta)
{
	return int(op::is_null) <= int(eta);
}

struct predicate
{
	op f;
	xpr::row_value x, y;
};

void print(predicate const&, writeall_t);
void print(row_value const&, writeall_t);

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

	void walk(signature<void(xpr::predicate&)> cb);

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

using query_handler_t = signature<void(Query)>;

Query parse_query(string_view s);
void parse_script(char const* name, query_handler_t cb);

inline void print(string_view sv, writeall_t cb)
{
	cb(sv.data(), sv.size());
}

template <typename T>
inline void print_list(T const& v, writeall_t cb)
{
	bool more = false;
	for (auto& x : v)
	{
		if (more)
			cb(", ", 2);
		else
			more = true;
		print(x, cb);
	}
}

}
