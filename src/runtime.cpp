#include "runtime.hpp"
#include "os/os.hpp"
#include "utils/rounding.hpp"
#include "utils/asserts.hpp"

Runtime::Runtime(ScannerMessageReporter& scannerReporter, ParserMessageReporter& parserReporter, IrCompilerMessageReporter& irCompilerReporter)
    : scannerReporter(scannerReporter)
    , parserReporter(parserReporter)
    , irCompilerReporter(irCompilerReporter) {}

#include "utils/fileIo.hpp"

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
    //outputToFile("test.bin", machineCode.code);

    return LoopFunction(machineCode);
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

Runtime::LoopFunction& Runtime::LoopFunction::operator=(LoopFunction&& other) noexcept {
    function = other.function;
    other.function = nullptr;
    return *this;
}

Runtime::LoopFunction::~LoopFunction() {
    if (function == nullptr) {
        return;
    }
    freeExecutableMemory(function);
}

void Runtime::LoopFunction::operator()(const __m256* input, __m256* output, i64 count) {
    function(input, output, count);
}

LoopFunctionArray::LoopFunctionArray(i64 valuesPerBlock)
    : valuesPerBlock(valuesPerBlock)
    , dataCapacity(0)
    , blockCount(0)
    , data(nullptr) {}

LoopFunctionArray::~LoopFunctionArray() {
    delete data;
}

void LoopFunctionArray::append(std::span<const float> block) {
    const auto dataIndex = blockCount / ITEMS_PER_DATA * valuesPerBlock;

    const auto requiredSize = dataIndex + valuesPerBlock;
    if (requiredSize > dataCapacity) {
        const auto newDataCapacity = std::max(requiredSize, dataCapacity * 2);
        const auto newData = new __m256[newDataCapacity];
        // TODO: Don't need to copy this much data.
        memcpy(newData, data, dataCapacity * sizeof(__m256));
        delete data;
        data = newData;
        dataCapacity = newDataCapacity;
    }

    const auto offsetInData = blockCount % ITEMS_PER_DATA;

    if (block.size() != valuesPerBlock) {
        ASSERT_NOT_REACHED();
        return;
    }

    for (usize i = 0; i < block.size(); i++) {
        data[dataIndex + i].m256_f32[offsetInData] = block[i];
    }
    blockCount++;
}

void LoopFunctionArray::clear() {
    blockCount = 0;
}

void LoopFunctionArray::resizeWithoutCopy(i64 newBlockCount) {
    const auto requiredSize = newBlockCount / ITEMS_PER_DATA * valuesPerBlock + valuesPerBlock;
    if (requiredSize <= dataCapacity) {
        return; 
    }

    delete data;
    const auto newData = new __m256[requiredSize];
    data = newData;
    dataCapacity = requiredSize;
    blockCount = newBlockCount;
}

//std::span<const float> LoopFunctionArray::operator[](i64 block) const {
//
//}

float LoopFunctionArray::operator()(i64 block, i64 indexInBlock) const {
    const auto dataIndex = block / ITEMS_PER_DATA * valuesPerBlock;
    const auto offsetInData = block % ITEMS_PER_DATA;
    return data[dataIndex + indexInBlock].m256_f32[offsetInData];
}