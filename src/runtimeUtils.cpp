#include "runtimeUtils.hpp"
#include "utils/asserts.hpp"

std::unordered_map<AddressLabel, void*> mapFunctionLabelsToAddresses(
	const std::unordered_map<std::string, AddressLabel>& functionNameToLabel, 
	std::span<const FunctionInfo> functions) {
	
	std::unordered_map<AddressLabel, void*> addressLabelToAddress;
	for (const auto& [functionName, label] : functionNameToLabel) {
		const auto function = std::find_if(
			functions.begin(),
			functions.end(),
			[&](const FunctionInfo& f) { return f.name == functionName; });

		if (function == functions.end()) {
			continue;
		}

		const auto newItemInserted = addressLabelToAddress.insert({ label, function->address }).second;
		ASSERT(newItemInserted);
	}
	return addressLabelToAddress;
}
