#pragma once

#include <yasc/yasc.h>

namespace yasc
{

struct states
{
	std::vector<logical::argument> pstack;
	std::vector<xpr::row_value> vstack;
	xpr::op pending;
};

}
