#pragma once

#include <yasc/yasc.h>

namespace yasc
{

struct states
{
	query_handler_t callback;
	std::vector<logical::argument> pstack;
	std::vector<xpr::row_value> vstack;
	xpr::op pending;
	std::string unescaped;
};

}
