#pragma once

#include <yasc/yasc.h>

namespace yasc
{

struct states
{
	states() : callback([](auto&&) { throw std::bad_function_call{}; }) {}
	explicit states(query_handler_t cb) : callback(cb) {}

	std::vector<logical::argument> pstack;
	std::vector<xpr::row_value> vstack;
	xpr::op pending;
	std::string unescaped;
	query_handler_t callback;
};

}
