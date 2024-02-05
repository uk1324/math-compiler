#pragma once

#include "ir.hpp"
#include "input.hpp"
#include "utils/overloaded.hpp"
#include "utils/hashCombine.hpp"
#include <unordered_map>
#include <span>

namespace Lvn {
	using ValueNumber = Register;
	struct AddVal {
		AddVal(ValueNumber lhs, ValueNumber rhs);
		ValueNumber lhs;
		ValueNumber rhs;

		bool operator==(const AddVal&) const = default;
	};

	struct MultiplyVal {
		MultiplyVal(ValueNumber lhs, ValueNumber rhs);
		ValueNumber lhs;
		ValueNumber rhs;

		bool operator==(const MultiplyVal&) const = default;
	};

	struct ConstantVal {
		Real value;

		bool operator==(const ConstantVal&) const = default;
	};

	using Val = std::variant<AddVal, MultiplyVal, ConstantVal>;

	enum class OpType {
		ADD, MULTIPLY,
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
				HASH_BINARY_OP(MultiplyVal, MULTIPLY),
				[](const ConstantVal& e) -> usize {
					return hash<Real>()(e.value);
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
	std::unordered_map<Register, Lvn::ValueNumber> regToValueNumberMap;
	std::unordered_map<Lvn::Val, Lvn::ValueNumber> valToValueNumber;
	std::unordered_map<Lvn::ValueNumber, Lvn::Val> valueNumberToVal;
};