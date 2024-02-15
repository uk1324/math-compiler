#pragma once

#include <unordered_map>
#include <string>
#include <span>
#include "assemblyCode.hpp"
#include "input.hpp"

std::unordered_map<AddressLabel, void*> mapFunctionLabelsToAddresses(
	const std::unordered_map<std::string, AddressLabel>& functionNameToLabel,
	std::span<const FunctionInfo> functions);