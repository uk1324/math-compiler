#include "runtime.hpp"
#include "os/os.hpp"
#include "utils/rounding.hpp"

Runtime::Runtime(ScannerMessageReporter& scannerReporter, ParserMessageReporter& parserReporter, IrCompilerMessageReporter& irCompilerReporter)
    : scannerReporter(scannerReporter)
    , parserReporter(parserReporter)
    , irCompilerReporter(irCompilerReporter) {}

std::optional<Runtime::LoopFunction> Runtime::compileFunction(
    std::string_view source, 
    std::span<const Variable> variables) {

    const auto& tokens = scanner.parse(source, functions, variables, scannerReporter);
    const auto& ast = parser.parse(tokens, source, parserReporter);
    if (!ast.has_value()) {
        return std::nullopt;
    }
    const auto& irCode = compiler.compile(*ast, variables, functions, irCompilerReporter);
    if (!irCode.has_value()) {
        return std::nullopt;
    }

    const auto& machineCode = codeGenerator.compile(*irCode, functions, variables);

    
}

Runtime::LoopFunction::LoopFunction(const MachineCode& machineCode) {
    const auto alignment = 16;
    const auto memory = reinterpret_cast<u8*>(allocateExecutableMemory(machineCode.code.size() + machineCode.data.size() + alignment));

    u8* code = memory;
    memcpy(code, machineCode.code.data(), machineCode.code.size());

    // Use the same buffer for data so the rip relative operands are not outside the i32 range.
    u8* data = roundAddressUpToMultiple(memory + machineCode.code.size(), alignment);
    memcpy(data, machineCode.data.data(), machineCode.data.size());
    machineCode.patchRipRelativeOperands(code, data);
    function = reinterpret_cast<Function>(code);
}

Runtime::LoopFunction::LoopFunction(LoopFunction&& other) noexcept
    : function(other.function) {
    other.function = nullptr;
}

Runtime::LoopFunction::~LoopFunction() {
    freeExecutableMemory(function);
}

void Runtime::LoopFunction::operator()(const __m256* input, __m256* output, i64 count) {
    function(input, output, count);
}

//LoopFunctionArray::LoopFunctionArray(i64 valuesPerBlock)
//    : valuesPerBlock(valuesPerBlock)
//    , capacity(0)
//    , length(0) 
//    , data(nullptr) {}
//
//void LoopFunctionArray::append(std::span<const float> block) {
//    const auto index = (length + 1) % (valuesPerBlock * sizeof(__m256));
//
//    const auto offsetInBlock = 
//    for (i64 i = 0; i < block.size(); i++) {
//
//    }
//}
