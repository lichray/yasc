#include "doctest.h"

#include <yasc/yasc.h>

TEST_CASE("query with conditions")
{
	auto q = yasc::parse_query(R"q(
        select * from A, B
         where A.c = +12
	)q");

	q = yasc::parse_query(R"q(
        select * from A, B
         where A.c = B.c and ( A.cnt_ > 0 or B.hg <= 4 )
           and not b < -3000 or title is null
	)q");
}

TEST_CASE("syntax errors in conditions")
{
	REQUIRE_THROWS(yasc::parse_query("select a from b where"));
	REQUIRE_THROWS(yasc::parse_query("select a from b where 1"));

	REQUIRE_THROWS(yasc::parse_query(R"q(
        select * from A
         where c = 00
	)q"));

	REQUIRE_THROWS(yasc::parse_query(R"q(
        select * from A
         where c == 2
	)q"));
}
