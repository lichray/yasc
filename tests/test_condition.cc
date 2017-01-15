#include "doctest.h"

#include <yasc/yasc.h>

TEST_CASE("query with conditions")
{
	auto q = yasc::parse_query(R"q(
        select * from A, B
         where A.c = +12
	)q");

	REQUIRE(q.where.f->eta == yasc::logical::op::identity);
	REQUIRE(q.where.f->args.size() == 1);
	REQUIRE(q.where.f->args[0].f == nullptr);

	auto expr = q.where.f->args[0].test;

	REQUIRE(expr.f == yasc::xpr::op::equals);
	REQUIRE(expr.x.get<yasc::xpr::column>().id == "A.c");
	REQUIRE(expr.y.get<int64_t>() == 12);

	q = yasc::parse_query(R"q(
        select * from A, B
         where A.c = B.c and A.cnt_ > 0 or B.hg <= 4
           and not b < -3000 or title is null
	)q");

	REQUIRE(q.where.f->eta == yasc::logical::op::disjunction);
	REQUIRE(q.where.f->args.size() == 3);
	REQUIRE(q.where.f->args[0].f->eta == yasc::logical::op::conjunction);
	REQUIRE(q.where.f->args[1].f->eta == yasc::logical::op::conjunction);
	REQUIRE(q.where.f->args[2].f == nullptr);
	REQUIRE(q.where.f->args[1].f->args[1].f->eta ==
	        yasc::logical::op::negation);

	expr = q.where.f->args[0].f->args[0].test;

	REQUIRE(expr.f == yasc::xpr::op::equals);
	REQUIRE(expr.x.get<yasc::xpr::column>().id == "A.c");
	REQUIRE(expr.y.get<yasc::xpr::column>().id == "B.c");

	expr = q.where.f->args[0].f->args[1].test;

	REQUIRE(expr.f == yasc::xpr::op::greater_than);
	REQUIRE(expr.x.get<yasc::xpr::column>().id == "A.cnt_");
	REQUIRE(expr.y.get<int64_t>() == 0);

	expr = q.where.f->args[1].f->args[0].test;

	REQUIRE(expr.f == yasc::xpr::op::less_than_or_equals);
	REQUIRE(expr.x.get<yasc::xpr::column>().id == "B.hg");
	REQUIRE(expr.y.get<int64_t>() == 4);

	expr = q.where.f->args[1].f->args[1].f->args[0].test;

	REQUIRE(expr.f == yasc::xpr::op::less_than);
	REQUIRE(expr.x.get<yasc::xpr::column>().id == "b");
	REQUIRE(expr.y.get<int64_t>() == -3000);

	expr = q.where.f->args[2].test;

	REQUIRE(expr.f == yasc::xpr::op::is_null);
	REQUIRE(expr.x.get<yasc::xpr::column>().id == "title");
}

TEST_CASE("basic conditions")
{
	auto q = yasc::parse_query(R"q(
        select * from A
         where a = b or c = d and e = f or x = y
	)q");

	REQUIRE(q.where.f->eta == yasc::logical::op::disjunction);
	REQUIRE(q.where.f->args.size() == 3);
	REQUIRE(q.where.f->args[0].f == nullptr);
	REQUIRE(q.where.f->args[1].f->eta == yasc::logical::op::conjunction);
	REQUIRE(q.where.f->args[2].f == nullptr);

	q = yasc::parse_query(R"q(
        select * from A
         where a = b and c = d or e = f and x = y
	)q");

	REQUIRE(q.where.f->eta == yasc::logical::op::disjunction);
	REQUIRE(q.where.f->args.size() == 2);
	REQUIRE(q.where.f->args[0].f->eta == yasc::logical::op::conjunction);
	REQUIRE(q.where.f->args[1].f->eta == yasc::logical::op::conjunction);

	q = yasc::parse_query(R"q(
        select * from A
         where a = b and c = d and not e = f and x = y
	)q");

	REQUIRE(q.where.f->eta == yasc::logical::op::conjunction);
	REQUIRE(q.where.f->args.size() == 4);
	REQUIRE(q.where.f->args[0].f == nullptr);
	REQUIRE(q.where.f->args[1].f == nullptr);
	REQUIRE(q.where.f->args[2].f->eta == yasc::logical::op::negation);
	REQUIRE(q.where.f->args[3].f == nullptr);
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
