#include "doctest.h"

#include <yasc/yasc.h>

#include <iostream>
#include <algorithm>

struct string_writer
{
	void operator()(char const* p, size_t sz)
	{
		str.append(p, sz);
	}

	std::string str;
};

TEST_CASE("print conditions")
{
	auto q = yasc::parse_query(R"q(
        select * from A, B
         where A.c = 'nice"	boat' or B.hg <= -3.4E-1
           or b < -3000 or title IS NOT NULL
	)q");

	string_writer w;
	std::vector<yasc::xpr::predicate> v;
	std::transform(begin(q.where.f->args), end(q.where.f->args),
	               std::back_inserter(v),
	               std::mem_fn(&yasc::logical::argument::test));
	yasc::print_list(v, w);

	REQUIRE(w.str ==
	        R"s(A.c = "nice\"\tboat", B.hg <= -0.34)s"
	        R"s(, b < -3000, title is not null)s");
}
