#pragma once

#include <string_view>
#include "utils/ints.hpp"

struct Variable {
	std::string_view name;

	bool operator==(const Variable&) const = default;
};

// I think it might be simpler to have a single function info type that is used by all the parts of the compiler even though parts like the compiler don't need arity or addres information. Making different representations for all the components would make more sense if they were unrelated like for example Parser and MachineCode, do it in this case is probably just pointless overcomplicating.
struct FunctionInfo {
	std::string_view name;
	i64 arity;
	void* address;
};