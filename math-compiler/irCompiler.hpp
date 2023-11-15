#pragma once

#include "ir.hpp"
#include <optional>

struct IrCompiler {
	/*struct DataType {

	};*/

	struct ExprResult {
		Register result;
	};

	IrCompiler();
	void initialize();

	std::optional<const std::vector<IrOp>*> compile(const Ast& ast);

	ExprResult compileExpression(const Expr* expr);
	ExprResult compileConstantExpr(const ConstantExpr& expr);
	ExprResult compileBinaryExpr(const BinaryExpr& expr);

	i64 allocatedRegistersCount = 0;
	Register allocateRegister();

	void addOp(const IrOp& op);

	std::vector<IrOp> generatedIrCode;
};