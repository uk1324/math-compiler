#pragma once

#include "ast.hpp"
#include "input.hpp"
#include "utils/result.hpp"
#include <span>
#include <string>

Result<Real, std::string> evaluateAst(
	const Expr* expr, 
	std::span<const Variable> parameters, 
	std::span<const FunctionInfo> functions, 
	std::span<const float> arguments);