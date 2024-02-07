#include "valueNumbering.hpp"
#include "floatingPoint.hpp"
#include "utils/asserts.hpp"
#include <bit>

using namespace Lvn;

void LocalValueNumbering::initialize(std::span<const FunctionParameter> parameters) {
	regToValueNumberMap.clear();
	allocatedValueNumbersCount = 0;
	for (Register i = 0; i < Register(parameters.size()); i++) {
		regToValueNumberMap[i] = i;
	}
	allocatedValueNumbersCount = parameters.size();
	valToValueNumber.clear();
	valueNumberToVal.clear();
}

/*
Things to consider:
NaNs, Infinities, signs of zeros
Constant propagation might remove signaling of NaN and Infinities.
https://en.wikipedia.org/wiki/NaN
https://people.eecs.berkeley.edu/~wkahan/ieee754status/IEEE754.PDF
Look at the output of compilers
*/

std::vector<IrOp> LocalValueNumbering::run(const std::vector<IrOp>& irCode, std::span<const FunctionParameter> parameters) {
	initialize(parameters);

	std::vector<IrOp> output;
	bool performUnsafeOptimizations = false;

	for (const auto& irOp : irCode) {
		

		std::optional<Computed> computed = std::visit(overloaded{
			[this](const LoadConstantOp& op) -> std::optional<Computed> {
				return Computed{
					.destinationRegister = op.destination,
					.destinationValueNumber = regToValueNumber(op.destination),
					.value = ConstantVal{ op.constant }
				};
			},
			[this](const AddOp& op) -> std::optional<Computed> {
				const auto d = getBinaryOpData(op.destination, op.lhs, op.rhs);

				if (d.lhsConst != nullptr && d.rhsConst != nullptr) {
					const auto value = d.lhsConst->value + d.rhsConst->value;
					return Computed{
						.destinationRegister = op.destination,
						.destinationValueNumber = d.destinationVn,
						.value = ConstantVal{ value }
					};
				}

				const auto e = getCommutativeOpWithOneConstantData(d, op.lhs, op.rhs);

				const Computed computed{
					.destinationRegister = op.destination,
					.destinationValueNumber = d.destinationVn,
					.value = AddVal(d.lhsVn, d.rhsVn)
				};

				if (!e.has_value()) {
					return computed;
				}

				if (e->b == 0.0f) {
					regToValueNumberMap[op.destination] = e->aRegister;
					return std::nullopt;
				}

				// TODO: a + -x => a - x is used by compilers.
				// Don't have a negation val could create it or just check it is a xor val and has the right operands.
				/*const auto rhsValueIt = valueNumberToVal.find(d.rhsVn);
				if (rhsValueIt != valueNumberToVal.end()) {
					if (const auto negation = std::get_if<NegateOp)
				}*/

				return computed;
			},
			[this, &output, &performUnsafeOptimizations](const SubtractOp& op) -> std::optional<Computed> {
				const auto d = getBinaryOpData(op.destination, op.lhs, op.rhs);
				if (d.lhsConst != nullptr && d.rhsConst != nullptr) {
					return Computed{
						.destinationRegister = op.destination,
						.destinationValueNumber = d.destinationVn,
						.value = ConstantVal{ d.lhsConst->value - d.rhsConst->value }
					};
				} 
				if (d.rhsConst != nullptr && d.rhsConst->value == 0.0f) {
					regToValueNumberMap[op.destination] = d.lhsVn;
					return std::nullopt;
				} else if (performUnsafeOptimizations && d.lhsConst != nullptr && d.lhsConst->value == 0.0f) {
					// Negation means flipping the sign bit https://en.wikipedia.org/wiki/IEEE_754-1985
					// 0 - 0 = 0 but -(0) = -0
					return computeNegation(output, op.rhs, d.destinationVn, op.destination);
				} else if (performUnsafeOptimizations && d.lhsVn == d.rhsVn) {
					// infty - infty = NaN
					return Computed{
						.destinationRegister = op.destination,
						.destinationValueNumber = d.destinationVn,
						.value = ConstantVal{ 0.0f }
					};
				}

				return Computed{
					.destinationRegister = op.destination,
					.destinationValueNumber = d.destinationVn,
					.value = SubtractVal(d.lhsVn, d.rhsVn)
				};
			},
			[this, &performUnsafeOptimizations](const MultiplyOp& op) -> std::optional<Computed> {
				const auto d = getBinaryOpData(op.destination, op.lhs, op.rhs);

				if (d.lhsConst != nullptr && d.rhsConst != nullptr) {
					return Computed{
						.destinationRegister = op.destination,
						.destinationValueNumber = d.destinationVn,
						.value = ConstantVal{ d.lhsConst->value * d.rhsConst->value }
					};
				} 

				const auto e = getCommutativeOpWithOneConstantData(d, op.lhs, op.rhs);

				const Computed computed{
					.destinationRegister = op.destination,
					.destinationValueNumber = d.destinationVn,
					.value = MultiplyVal(d.lhsVn, d.rhsVn)
				};

				if (!e.has_value()) {
					return computed;
				}

				if (e->b == 1.0f) {
					return computeIdentity(op.destination, e->a);
				} else if (performUnsafeOptimizations && e->b == 0.0f) {
					// NaN * 0 = NaN
					// 0 * Infinity = NaN
					return Computed{
						.destinationRegister = op.destination,
						.destinationValueNumber = d.destinationVn,
						.value = ConstantVal{ 0.0f }
					};
				} else if (e->b == 2.0f) {
					// This might not be worth it.
					// Based on https://www.agner.org/optimize/instruction_tables.pdf these instruction take around the same time, but based on this https://stackoverflow.com/questions/1146455/whats-the-relative-speed-of-floating-point-add-vs-floating-point-multiply depending on the sourounding context this might be beinficial or not.
					return Computed{
						.destinationRegister = op.destination,
						.destinationValueNumber = d.destinationVn,
						.value = AddVal(e->a, e->a)
					};
				}

				return computed;
			},
			[this, &performUnsafeOptimizations](const DivideOp& op) -> std::optional<Computed> {
				const auto d = getBinaryOpData(op.destination, op.lhs, op.rhs);

				if (d.lhsConst != nullptr && d.rhsConst != nullptr) {
					return Computed{
						.destinationRegister = op.destination,
						.destinationValueNumber = d.destinationVn,
						.value = ConstantVal{ d.lhsConst->value / d.rhsConst->value }
					};
				}

				if (d.rhsConst != nullptr && d.rhsConst->value == 1.0f) {
					return computeIdentity(op.destination, d.lhsVn);
				} else if (performUnsafeOptimizations && d.lhsVn == d.rhsVn) {
					// 0 / 0 = NaN
					// Infinity / Infinity = Nan
					// Also rounding.
					return Computed{
						.destinationRegister = op.destination,
						.destinationValueNumber = d.destinationVn,
						.value = ConstantVal{ 1.0f }
					};
				}

				return Computed{
					.destinationRegister = op.destination,
					.destinationValueNumber = d.destinationVn,
					.value = DivideVal{ d.lhsVn, d.rhsVn }
				};
			},
			[this](const XorOp& op) -> std::optional<Computed> {
				const auto d = getBinaryOpData(op.destination, op.lhs, op.rhs);

				if (d.lhsConst != nullptr && d.rhsConst != nullptr) {
					const u32 a = std::bit_cast<u32>(d.lhsConst->value);
					const u32 b = std::bit_cast<u32>(d.rhsConst->value);
					const u32 result = a xor b;
					const auto value = std::bit_cast<float>(result);
					return Computed{
						.destinationRegister = op.destination,
						.destinationValueNumber = d.destinationVn,
						.value = ConstantVal{ value }
					};
				}

				const auto e = getCommutativeOpWithOneConstantData(d, op.lhs, op.rhs);

				const Computed computed{
					.destinationRegister = op.destination,
					.destinationValueNumber = d.destinationVn,
					.value = AddVal(d.lhsVn, d.rhsVn)
				};

				if (!e.has_value()) {
					return computed;
				}
				return computed;
			},
			[this, &output](const NegateOp& op) -> std::optional<Computed> {
				const auto destinationVn = regToValueNumber(op.destination);
				const auto operandVn = regToValueNumber(op.operand);
				const auto operandConst = tryGetConstant(operandVn);
				if (operandConst != nullptr) {
					return Computed{
						.destinationRegister = op.destination,
						.destinationValueNumber = destinationVn,
						.value = ConstantVal{ -operandConst->value }
					};
				}
				
				return computeNegation(output, operandVn, destinationVn, op.destination);
			},
			[this, &output](const ReturnOp& op) -> std::optional<Computed> {
				const ReturnOp newOp{ .returnedRegister = regToValueNumber(op.returnedRegister) };
				output.push_back(newOp);
				return std::nullopt;
			}
		}, irOp);

		if (!computed.has_value()) {
			continue;
		}

		const auto vnIt = valToValueNumber.find(computed->value);
		if (vnIt != valToValueNumber.end()) {
			regToValueNumberMap[computed->destinationRegister] = vnIt->second;
			continue;
		}

		valueNumberToVal.emplace(computed->destinationValueNumber, computed->value);
		valToValueNumber.emplace(computed->value, computed->destinationValueNumber);
		
		std::visit(overloaded{
			[&output, &computed](const AddVal& val) {
				output.push_back(AddOp{ 
					.destination = computed->destinationValueNumber,
					.lhs = val.lhs,
					.rhs = val.rhs
				});
			},
			[&output, &computed](const SubtractVal& val) {
				output.push_back(SubtractOp{
					.destination = computed->destinationValueNumber,
					.lhs = val.lhs,
					.rhs = val.rhs
				});
			},
			[&output, &computed](const MultiplyVal& val) {
				output.push_back(MultiplyOp{
					.destination = computed->destinationValueNumber,
					.lhs = val.lhs,
					.rhs = val.rhs
				});
			},
			[&output, &computed](const DivideVal& val) {
				output.push_back(DivideOp{
					.destination = computed->destinationValueNumber,
					.lhs = val.lhs,
					.rhs = val.rhs
				});
			},
			[&output, &computed](const XorVal& val) {
				output.push_back(XorOp{
					.destination = computed->destinationValueNumber,
					.lhs = val.lhs,
					.rhs = val.rhs
				});
			},
			[&output, &computed](const ConstantVal& val) {
				output.push_back(LoadConstantOp{ 
					.destination = computed->destinationValueNumber, 
					.constant = val.value 
				});
			}
		}, computed->value);
	}

	return output;
}

ValueNumber LocalValueNumbering::regToValueNumber(Register reg) {
	const auto valueNumberIt = regToValueNumberMap.find(reg);
	if (valueNumberIt != regToValueNumberMap.end()) {
		return valueNumberIt->second;
	}
	ValueNumber valueNumber = allocateValueNumber();
	regToValueNumberMap[reg] = valueNumber;
	return valueNumber;
}

const ConstantVal* LocalValueNumbering::tryGetConstant(ValueNumber vn) const{
	const auto it = valueNumberToVal.find(vn);
	if (it == valueNumberToVal.end()) {
		return nullptr;
	}

	if (const auto constant = std::get_if<ConstantVal>(&it->second)) {
		return constant;
	}
	return nullptr;
}

LocalValueNumbering::BinaryOpData LocalValueNumbering::getBinaryOpData(Register destination, Register lhs, Register rhs) {
	const auto destinationVn = regToValueNumber(destination);
	const auto lhsVn = regToValueNumber(lhs);
	const auto rhsVn = regToValueNumber(rhs);

	const auto lhsConst = tryGetConstant(lhsVn);
	const auto rhsConst = tryGetConstant(rhsVn);

	return BinaryOpData{ destinationVn, lhsVn, rhsVn, lhsConst, rhsConst };
}

std::optional<LocalValueNumbering::CommutativeOpWithOneConstantData> LocalValueNumbering::getCommutativeOpWithOneConstantData(const BinaryOpData& d, Register lhsReg, Register rhsReg){
	if (d.lhsConst == nullptr && d.rhsConst == nullptr) {
		return std::nullopt;
	}

	Register aRegister;
	ValueNumber a;
	float b;

	if (d.lhsConst != nullptr) {
		aRegister = rhsReg;
		a = d.rhsVn;
		b = d.lhsConst->value;
	} else {
		ASSERT(d.rhsConst != nullptr);
		aRegister = lhsReg;
		a = d.lhsVn;
		b = d.rhsConst->value;
	}

	return CommutativeOpWithOneConstantData{
		.aRegister = aRegister,
		.a = a,
		.b = b,
	};
}

LocalValueNumbering::Computed LocalValueNumbering::computeNegation(std::vector<IrOp>& output, ValueNumber operand, ValueNumber destinationVn, Register destinationRegister) {
	const auto maskConstant = std::bit_cast<float>(F32_SIGN_BIT_MASK);
	const auto maskVn = getConstantValueNumber(output, maskConstant);
	return Computed{
		.destinationRegister = destinationRegister,
		.destinationValueNumber = destinationVn,
		.value = XorVal(operand, maskVn)
	};
}

Lvn::ValueNumber LocalValueNumbering::getConstantValueNumber(std::vector<IrOp>& output, float constant) {
	const auto val = ConstantVal{ .value = constant };
	const auto valIt = valToValueNumber.find(val);
	if (valIt != valToValueNumber.end()) {
		const auto vn = valIt->second;
		return vn;
	}

	const auto vn = allocateValueNumber();
	output.push_back(LoadConstantOp{ .destination = vn, .constant = constant });
	valToValueNumber.emplace(val, vn);
	return vn;
}

std::nullopt_t LocalValueNumbering::computeIdentity(Register destinationRegister, Lvn::ValueNumber value) {
	// If you refer to destinationRegister then you will be refering to value.
	regToValueNumberMap[destinationRegister] = value;
	return std::nullopt;
}

Lvn::ValueNumber LocalValueNumbering::allocateValueNumber() {
	const auto vn = allocatedValueNumbersCount;
	allocatedValueNumbersCount++;
	return vn;
}

#define INITIALIZE_COMMUTATIVE_BINARY_OP() \
	if (lhs <= rhs) { \
		this->lhs = lhs; \
		this->rhs = rhs; \
	} \
	else { \
		this->lhs = rhs; \
		this->rhs = lhs; \
	}

Lvn::AddVal::AddVal(ValueNumber lhs, ValueNumber rhs) {
	INITIALIZE_COMMUTATIVE_BINARY_OP();
}

Lvn::MultiplyVal::MultiplyVal(ValueNumber lhs, ValueNumber rhs) {
	INITIALIZE_COMMUTATIVE_BINARY_OP();
}

Lvn::XorVal::XorVal(ValueNumber lhs, ValueNumber rhs) {
	INITIALIZE_COMMUTATIVE_BINARY_OP();
}