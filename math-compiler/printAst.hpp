#pragma once

#include "ast.hpp"

void printBinaryOpType(BinaryOpType type);
void printExpr(Expr* e, bool printExtraParens);