#pragma once

#include <string_view>

struct FunctionParameter {
	std::string_view name;

	bool operator==(const FunctionParameter&) const = default;
};