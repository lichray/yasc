#include "catch.hpp"

#include <yasc/yasc.h>

TEST_CASE("empty")
{
	REQUIRE(yasc::parse_query("+312234") == "+312234");
	REQUIRE(yasc::parse_query("-4") == "-4");
	REQUIRE(yasc::parse_query("0") == "0");
	REQUIRE_THROWS(yasc::parse_query("00"));
}
