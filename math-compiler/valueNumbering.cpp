#include "valueNumbering.hpp"
#include "utils/asserts.hpp"

using namespace Lvn;

void LocalValueNumbering::initialize(std::span<const FunctionParameter> parameters) {
	regToValueNumberMap.clear();
	for (Register i = 0; i < Register(parameters.size()); i++) {
		regToValueNumberMap[i] = i;
	}
	valToValueNumber.clear();
	valueNumberToVal.clear();
}

/*
Things to consider:
NaNs, Infinities, signs of zeros
Constant propagation might remove signaling of NaN and Infinities.
https://en.wikipedia.org/wiki/NaN
https://people.eecs.berkeley.edu/~wkahan/ieee754status/IEEE754.PDF
*/

std::vector<IrOp> LocalValueNumbering::run(const std::vector<IrOp>& irCode, std::span<const FunctionParameter> parameters) {
	initialize(parameters);

	std::vector<IrOp> output;
	bool performUnsafeOptimizations = false;


	for (const auto& irOp : irCode) {
		struct Computed {
			ValueNumber destinationRegister;
			ValueNumber destinationValueNumber;
			Val value;
		};

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

				return computed;
			},
			[this, &performUnsafeOptimizations](const MultiplyOp& op) -> std::optional<Computed> {
				const auto d = getBinaryOpData(op.destination, op.lhs, op.rhs);

				if (d.lhsConst != nullptr && d.rhsConst != nullptr) {
					const auto value = d.lhsConst->value * d.rhsConst->value;
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
					.value = MultiplyVal(d.lhsVn, d.rhsVn)
				};

				if (!e.has_value()) {
					return computed;
				}

				if (e->b == 1.0f) {
					regToValueNumberMap[op.destination] = e->aRegister;
					return std::nullopt;
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
			[&output, &computed](const MultiplyVal& val) {
				output.push_back(MultiplyOp{
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
	ValueNumber valueNumber = regToValueNumberMap.size();
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
