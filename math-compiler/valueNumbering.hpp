#pragma once

#include "ir.hpp"
#include "input.hpp"
#include "utils/overloaded.hpp"
#include "utils/hashCombine.hpp"
#include <unordered_map>
#include <optional>
#include <span>

namespace Lvn {
	using ValueNumber = Register;
	struct AddVal {
		AddVal(ValueNumber lhs, ValueNumber rhs);
		ValueNumber lhs;
		ValueNumber rhs;

		bool operator==(const AddVal&) const = default;
	};

	struct SubtractVal {
		ValueNumber lhs;
		ValueNumber rhs;

		bool operator==(const SubtractVal&) const = default;
	};

	struct MultiplyVal {
		MultiplyVal(ValueNumber lhs, ValueNumber rhs);
		ValueNumber lhs;
		ValueNumber rhs;

		bool operator==(const MultiplyVal&) const = default;
	};

	struct DivideVal {
		ValueNumber lhs;
		ValueNumber rhs;

		bool operator==(const DivideVal&) const = default;
	};

	struct XorVal {
		XorVal(ValueNumber lhs, ValueNumber rhs);
		ValueNumber lhs;
		ValueNumber rhs;

		bool operator==(const XorVal&) const = default;
	};

	struct ConstantVal {
		Real value;

		/*bool operator==(const ConstantVal&) const = default;*/
		bool operator==(const ConstantVal& other) const;
	};

	struct VariableVal {
		i64 variableIndex;

		bool operator==(const VariableVal&) const = default;
	};

	using Val = std::variant<AddVal, SubtractVal, MultiplyVal, DivideVal, XorVal, ConstantVal, VariableVal>;

	// TODO: Why not use the index from std::variant for hashing. 
	enum class OpType {
		ADD, SUBTRACT, MULTIPLY, DIVIDE, XOR,
	};
}

namespace std {
	template <> 
	struct hash<Lvn::Val> {
		usize operator()(const Lvn::Val& x) const {
			using namespace Lvn;
			#define HASH_BINARY_OP(Type, OP_TYPE) \
				[](const Type& e) -> usize { \
					usize h = hash<usize>()(usize(OpType::OP_TYPE)); \
					h = hashCombine(h, hash<ValueNumber>()(e.lhs)); \
					h = hashCombine(h, hash<ValueNumber>()(e.rhs)); \
					return h; \
				}

			return std::visit(overloaded{
				HASH_BINARY_OP(AddVal, ADD),
				HASH_BINARY_OP(SubtractVal, SUBTRACT),
				HASH_BINARY_OP(MultiplyVal, MULTIPLY),
				HASH_BINARY_OP(DivideVal, DIVIDE),
				HASH_BINARY_OP(XorVal, XOR),
				[](const ConstantVal& e) -> usize {
					return hash<Real>()(e.value);
				},
				[](const VariableVal& e) -> usize {
					return hash<Real>()(e.variableIndex);
				}
			}, x);

			#undef HASH_BINARY_OP
		}
	};
}

struct LocalValueNumbering {	
	void initialize(std::span<const FunctionParameter> parameters);

	std::vector<IrOp> run(const std::vector<IrOp>& irCode, std::span<const FunctionParameter> parameters);

	Lvn::ValueNumber regToValueNumber(Register reg);
	const Lvn::ConstantVal* tryGetConstant(Lvn::ValueNumber vn) const;

	struct BinaryOpData {
		Lvn::ValueNumber destinationVn;
		Lvn::ValueNumber lhsVn;
		Lvn::ValueNumber rhsVn;
		const Lvn::ConstantVal* lhsConst;
		const Lvn::ConstantVal* rhsConst;
	};

	BinaryOpData getBinaryOpData(Register destination, Register lhs, Register rhs);

	struct CommutativeOpWithOneConstantData {
		Register aRegister;
		Lvn::ValueNumber a;
		float b;
	};
	std::optional<CommutativeOpWithOneConstantData> getCommutativeOpWithOneConstantData(const BinaryOpData& d, Register lhsReg, Register rhsReg);

	struct Computed {
		Lvn::ValueNumber destinationRegister;
		Lvn::ValueNumber destinationValueNumber;
		Lvn::Val value;
	};
	Lvn::ValueNumber getConstantValueNumber(std::vector<IrOp>& output, float constant);
	Computed computeNegation(std::vector<IrOp>& output, Lvn::ValueNumber operand, Lvn::ValueNumber destinationVn, Register destinationRegister);
	std::nullopt_t computeIdentity(Register destinationRegister, Lvn::ValueNumber value);

	Lvn::ValueNumber allocateValueNumber();
	Lvn::ValueNumber allocatedValueNumbersCount = 0;
	std::unordered_map<Register, Lvn::ValueNumber> regToValueNumberMap;
	std::unordered_map<Lvn::Val, Lvn::ValueNumber> valToValueNumber;
	std::unordered_map<Lvn::ValueNumber, Lvn::Val> valueNumberToVal;
};