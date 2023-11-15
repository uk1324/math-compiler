#pragma once

#include "ast.hpp"

Real evaluateExpr(const Expr* expr);
Real evaulateBinaryOp(Real lhs, Real rhs, BinaryOpType op);