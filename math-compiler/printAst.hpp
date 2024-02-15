#pragma once

#include "ast.hpp"

void printBinaryOpType(BinaryOpType type);
void printExpr(const Expr* e, bool printExtraParens);