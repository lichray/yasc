#include "doctest.h"

#include <yasc/yasc.h>

TEST_CASE("query without filter")
{
	auto q = yasc::parse_query("SELECT ALL * FROM T1, T2");

	REQUIRE(q.distinct == false);
	REQUIRE(q.select.all_columns() == true);
	REQUIRE(q.from.count() == 2);
	REQUIRE(q.from[0] == "T1");
	REQUIRE(q.from[1] == "T2");

	q = yasc::parse_query(R"s(select foo, """b ar" from meow)s");

	REQUIRE(q.distinct == false);
	REQUIRE(q.select.all_columns() == false);
	REQUIRE(q.select.count() == 2);
	REQUIRE(q.select[0] == "foo");
	REQUIRE(q.select[1] == R"s("""b ar")s");
	REQUIRE(q.from.count() == 1);
	REQUIRE(*q.from.begin() == "meow");

	q = yasc::parse_query(R"q(
        select distinct foo  -- short
          from a /* middle */, /**
            long
            */  b, c
	)q");
	REQUIRE(q.distinct == true);
	REQUIRE(q.select.count() == 1);
	REQUIRE(q.select[0] == "foo");
	REQUIRE(q.from.count() == 3);
}

TEST_CASE("syntax errors in query")
{
	REQUIRE_THROWS(yasc::parse_query("select"));
	REQUIRE_THROWS(yasc::parse_query("select a, b, from c"));
	REQUIRE_THROWS(yasc::parse_query("select a, b where 1"));
	REQUIRE_THROWS(yasc::parse_query("select a from c garbage"));
	REQUIRE_THROWS(yasc::parse_query("select a from c /* "));
}
