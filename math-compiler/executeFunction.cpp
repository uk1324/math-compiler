#include "executeFunction.hpp"
#include "os/os.hpp"
#include "utils/put.hpp"

Real executeFunction(const CodeGenerator& codeGenerator, const std::vector<u8>& code, const std::vector<u8>& data) {
	auto codeBuffer = reinterpret_cast<u8*>(allocateExecutableMemory(code.size() + data.size()));
	if (codeBuffer == nullptr) {
		put("failed to allocate executable memory");
		exit(EXIT_FAILURE);
	}

	memcpy(codeBuffer, code.data(), code.size());
	const auto dataBuffer = codeBuffer + code.size();
	memcpy(dataBuffer, data.data(), data.size());
	codeGenerator.patchExecutableCodeRipAddresses(codeBuffer, data.data());

	using Function = float (*)(void);

	const auto function = reinterpret_cast<Function>(codeBuffer);
	return function();
}