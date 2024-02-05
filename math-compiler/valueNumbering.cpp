#include "valueNumbering.hpp"
#include <optional>

using namespace Lvn;

void LocalValueNumbering::initialize(std::span<const FunctionParameter> parameters) {
	regToValueNumberMap.clear();
	for (Register i = 0; i < Register(parameters.size()); i++) {
		regToValueNumberMap[i] = i;
	}
	valToValueNumber.clear();
	valueNumberToVal.clear();
}

std::vector<IrOp> LocalValueNumbering::run(const std::vector<IrOp>& irCode, std::span<const FunctionParameter> parameters) {
	initialize(parameters);

	std::vector<IrOp> output;

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
				} else {
					return Computed{
						.destinationRegister = op.destination,
						.destinationValueNumber = d.destinationVn,
						.value = AddVal(d.lhsVn, d.rhsVn)
					};
				}
			},
			[this](const MultiplyOp& op) -> std::optional<Computed> {
				const auto d = getBinaryOpData(op.destination, op.lhs, op.rhs);
				if (d.lhsConst != nullptr && d.rhsConst != nullptr) {
					const auto value = d.lhsConst->value * d.rhsConst->value;
					return Computed{
						.destinationRegister = op.destination,
						.destinationValueNumber = d.destinationVn,
						.value = ConstantVal{ value }
					};
				} else {
					return Computed{
						.destinationRegister = op.destination,
						.destinationValueNumber = d.destinationVn,
						.value = MultiplyVal(d.lhsVn, d.rhsVn)
					};
				}
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
