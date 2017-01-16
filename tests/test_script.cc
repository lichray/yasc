#include "doctest.h"

#include <yasc/yasc.h>

TEST_CASE("multiple statements")
{
	std::vector<yasc::Query> v;
	yasc::parse_script("tests/test_p3.sql",
	                   [&](auto q) { v.push_back(std::move(q)); });

	REQUIRE(v.size() == 2);

	REQUIRE(v[0].where.f->eta == yasc::logical::op::conjunction);
	REQUIRE(v[0].where.f->args[0].test.x.get<yasc::xpr::column>().id ==
	        "T1.a");
	REQUIRE(v[0].where.f->args[1].test.f == yasc::xpr::op::less_than);

	REQUIRE(v[1].select.count() == 3);
	REQUIRE(v[1].where.f->args[0].test.y.get<std::string>() ==
	        "Hello World");
}
