#include <yasc/yasc.h>

#include <iostream>
#include <args.hxx>

struct c_file_writer
{
	void operator()(char const* p, size_t sz)
	{
		(void)fwrite(p, 1, sz, fp);
	}

	FILE* fp;
};

static void move_constants_to_right(yasc::Query& q)
{
	using namespace yasc;

	q.where.f->walk([](xpr::predicate& a) {
		if (a.f != xpr::op::is_null and a.f != xpr::op::is_not_null and
		    not a.x.is<xpr::column>() and a.y.is<xpr::column>())
		{
			using std::swap;
			swap(a.x, a.y);

			switch (a.f)
			{
			case xpr::op::less_than:
				a.f = xpr::op::greater_than;
				break;
			case xpr::op::greater_than:
				a.f = xpr::op::less_than;
				break;
			case xpr::op::less_than_or_equals:
				a.f = xpr::op::greater_than_or_equals;
				break;
			case xpr::op::greater_than_or_equals:
				a.f = xpr::op::less_than_or_equals;
				break;
			default: break;
			}
		}
	});
}

int main(int argc, char** argv)
{
	args::ArgumentParser parser("Move constants in predicates to right.");
	args::HelpFlag help(parser, "help", "show this message", { 'h' });
	args::Group group(parser, "", args::Group::Validators::Xor);
	args::Positional<std::string> fname(group, "path",
	                                    "Path to a SQL script");
	parser.helpParams.showTerminator = false;

	try
	{
		parser.ParseCLI(argc, argv);
	}
	catch (args::Help)
	{
		std::cout << parser;
		return 0;
	}
	catch (args::ParseError e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return 1;
	}
	catch (args::ValidationError e)
	{
		std::cerr << parser;
		return 1;
	}

	c_file_writer w{ stdout };
	yasc::parse_script(get(fname).data(), [&, i = 0](auto q) mutable {
		if (i++)
			putc('\n', w.fp);

		if (q.select.all_columns())
			q.select.append("*");
		move_constants_to_right(q);

		fprintf(w.fp, "Query#%d:\n", i);
		fprintf(w.fp, "select (%d): ", q.select.count());
		print_list(q.select, w);
		putc('\n', w.fp);
		fprintf(w.fp, "from   (%d): ", q.from.count());
		print_list(q.from, w);
		putc('\n', w.fp);

		int n = 0;
		q.where.f->walk([&](auto&) { ++n; });
		fprintf(w.fp, "where  (%d): ", n);
		n = 0;
		q.where.f->walk([&](auto& a) {
			using yasc::print;
			if (n)
				print(", ", w);
			else
				n = 1;
			print(a, w);
		});
		putc('\n', w.fp);
	});
}
