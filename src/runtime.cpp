#include "runtime.hpp"
#include "os/os.hpp"
#include "utils/rounding.hpp"
#include "utils/asserts.hpp"
#include "simdFunctions.hpp"

Runtime::Runtime(ScannerMessageReporter& scannerReporter, ParserMessageReporter& parserReporter, IrCompilerMessageReporter& irCompilerReporter)
    : scannerReporter(scannerReporter)
    , parserReporter(parserReporter)
    , irCompilerReporter(irCompilerReporter) {
    
    functions.push_back({ .name = "exp", .arity = 1, .address = expSimd, });
    functions.push_back({ .name = "ln", .arity = 1, .address = lnSimd, });
    functions.push_back({ .name = "sin", .arity = 1, .address = sinSimd, });
    functions.push_back({ .name = "cos", .arity = 1, .address = cosSimd, });
    functions.push_back({ .name = "sqrt", .arity = 1, .address = sqrtSimd, });
}

#include "utils/fileIo.hpp"

std::optional<Runtime::LoopFunction> Runtime::compileFunction(
    std::string_view source, 
    std::span<const Variable> variables) {

    const auto ir = compileToIr(source, variables);
    if (!ir.has_value()) {
        return std::nullopt;
    }

    const auto& machineCode = codeGenerator.compile(*ir, functions, variables);
    //outputToFile("test.bin", machineCode.code);

    return LoopFunction(machineCode);
}

std::optional<std::vector<IrOp>> Runtime::compileToIr(
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

    std::vector<IrOp> a = *irCode;
    std::vector<IrOp> b;

    std::vector<IrOp>* input = &a;
    std::vector<IrOp>* output = &b;
    auto swap = [&]() -> void {
        std::swap(input, output);
    };
    auto result = [&]() -> const std::vector<IrOp>& {
        return *input;
    };

    valueNumbering.run(*input, variables, *output);
    swap();

    deadCodeElimination.run(*input, variables, *output);
    swap();

    return result();
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

void Runtime::LoopFunction::operator()(const __m256* input, __m256* output, i64 count) const {
    function(input, output, count);
}

void Runtime::LoopFunction::operator()(const LoopFunctionArray& input, LoopFunctionArray& output) const {
    if (input.blockCount() != output.blockCount()) {
        ASSERT_NOT_REACHED();
        return;
    }
    i64 dataCount = input.blockCount_ / LoopFunctionArray::ITEMS_PER_DATA;
    if (input.blockCount_ % LoopFunctionArray::ITEMS_PER_DATA != 0) {
        dataCount += 1;
    }
    operator()(input.data(), output.data(), dataCount);
}

LoopFunctionArray::LoopFunctionArray(i64 valuesPerBlock)
    : valuesPerBlock(valuesPerBlock)
    , dataCapacity(0)
    , blockCount_(0)
    , data_(nullptr) {}

LoopFunctionArray::~LoopFunctionArray() {
    delete data_;
}

void LoopFunctionArray::append(std::span<const float> block) {
    const auto dataIndex = blockCount_ / ITEMS_PER_DATA * valuesPerBlock;

    const auto requiredSize = dataIndex + valuesPerBlock;
    if (requiredSize > dataCapacity) {
        const auto newDataCapacity = std::max(requiredSize, dataCapacity * 2);
        const auto newData = new __m256[newDataCapacity];
        // TODO: Don't need to copy this much data.
        memcpy(newData, data_, dataCapacity * sizeof(__m256));
        delete data_;
        data_ = newData;
        dataCapacity = newDataCapacity;
    }

    const auto offsetInData = blockCount_ % ITEMS_PER_DATA;

    if (block.size() != valuesPerBlock) {
        ASSERT_NOT_REACHED();
        return;
    }

    for (usize i = 0; i < block.size(); i++) {
        data_[dataIndex + i].m256_f32[offsetInData] = block[i];
    }
    blockCount_++;
}

void LoopFunctionArray::clear() {
    blockCount_ = 0;
}

void LoopFunctionArray::resizeWithoutCopy(i64 newBlockCount) {
    const auto requiredSize = newBlockCount / ITEMS_PER_DATA * valuesPerBlock + valuesPerBlock;
    if (requiredSize <= dataCapacity) {
        blockCount_ = newBlockCount;
        return; 
    }

    delete data_;
    const auto newData = new __m256[requiredSize];
    data_ = newData;
    dataCapacity = requiredSize;
    blockCount_ = newBlockCount;
}

void LoopFunctionArray::reset(i64 valuesPerBlock) {
    this->valuesPerBlock = valuesPerBlock;
    blockCount_ = 0;
}