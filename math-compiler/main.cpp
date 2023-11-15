#include <iostream>
#include "scanner.hpp"
#include "parser.hpp"
#include "utils/asserts.hpp"
#include "printAst.hpp"
#include "evaluateAst.hpp"
#include "utils/pritningUtils.hpp"
#include "ostreamScannerMessageReporter.hpp"
#include "ostreamParserMessageReporter.hpp"
#include "../math-compiler/test/testingParserMessageReporter.hpp"
#include "../math-compiler/test/testingScannerMessageReporter.hpp"
#include "utils/put.hpp"
#include "irCompiler.hpp"
#include "irVm.hpp"
#include <sstream>

i64 tokenOffsetInSource(const Token& token, std::string_view originalSource) {
	const auto offsetInOriginalSource = token.source.data() - originalSource.data();
	return offsetInOriginalSource;
}

void debugOutputToken(const Token& token, std::string_view originalSource) {
	const auto offsetInOriginalSource = tokenOffsetInSource(token, originalSource);
	std::cout << offsetInOriginalSource << " " << offsetInOriginalSource + token.source.size() << " = ";
	std::cout << tokenToStr(token);
	std::cout << " '" << token.source << "'";
}

template<typename T>
std::vector<T> setDifference(const std::vector<T>& as, const std::vector<T>& bs) {
	std::vector<T> difference;
	for (const auto& a : as) {
		bool aInBs = false;
		for (const auto& b : bs) {
			if (a == b) {
				aInBs = true;
				break;
			}
		}
		if (!aInBs) {
			difference.push_back(a);
		}
	}
	return difference;
}

//#define DEBUG_PRINT_GENERATED_IR_CODE

void testMain() {
	Scanner scanner;
	Parser parser;

	IrCompiler irCompiler;
	IrVm irVm;

	std::stringstream output;

	TestingScannerErrorReporter scannerReporter(output, std::string_view());
	TestingParserMessageReporter parserReporter(output, std::string_view());

	auto printFailed = [](std::string_view name) {
		std::cout << TerminalColors::RED << "[FAILED] " << TerminalColors::RESET << name << '\n';
	};
	auto printPassed = [](std::string_view name) {
		std::cout << TerminalColors::GREEN << "[PASSED] " << TerminalColors::RESET << name << '\n';
	};

	auto runTestExpected = [&](std::string_view name, std::string_view source, Real expectedOutput) {
		scannerReporter.reporter.source = source;
		parserReporter.reporter.source = source;
		//std::cout << "RUNNING TEST " << name " ";
		auto tokens = scanner.parse(source, &scannerReporter);
		if (scannerReporter.errorHappened) {
			printFailed(name);
			std::cout << "scanner error: \n" << output.str();
			output.clear();
			scannerReporter.reset();
			return;
		}
		auto ast = parser.parse(tokens, &parserReporter);
		if (!ast.has_value() && !parserReporter.errorHappened) {
			ASSERT_NOT_REACHED();
			return;
		}

		if (parserReporter.errorHappened) {
			printFailed(name);
			std::cout << "parser error: \n" << output.str();
			output.clear();
			parserReporter.reset();
			return;
		}

		bool evaluationError = false;

		{
			const auto output = evaluateExpr(ast->root);
			if (output != expectedOutput) {
				printFailed(name);
				std::cout << "ast interpreter error: ";
				std::cout << "expected '" << expectedOutput << "' got '" << output << "'\n";
				evaluationError = true;
			}
		}

		const auto irCode = irCompiler.compile(*ast);
		if (irCode.has_value()) {
			#ifdef DEBUG_PRINT_GENERATED_IR_CODE
			printIrCode(std::cout, **irCode);
			#endif

			const auto output = irVm.execute(**irCode);
			if (!output.has_value()) {
				printFailed(name);
				put("ir vm runtime error");
			} else if (*output != expectedOutput) {
				printFailed(name);
				std::cout << "ir vm error: ";
				std::cout << "expected '" << expectedOutput << "' got '" << *output << "'\n";
				evaluationError = true;
			}
		} else {
			printFailed(name);
			std::cout << "ir compiler error: ";
			return;
		}

		if (evaluationError) {
			return;
		}

		printPassed(name);
	};

	// I choose to ignore additional errors that were reported, because things like adding parser synchronization might add additional errors.
	auto runTestExpectedErrors = [&](
		std::string_view name,
		std::string_view source,
		const std::vector<ScannerError>& expectedScannerErrors,
		const std::vector<ParserError>& expectedParserErrors) {

		scannerReporter.reporter.source = source;
		parserReporter.reporter.source = source;

		auto tokens = scanner.parse(source, &scannerReporter);
		
		const auto scannerErrorsThatWereNotReported = setDifference(expectedScannerErrors, scannerReporter.errors);
		if (scannerErrorsThatWereNotReported.size() != 0) {
			printFailed(name);
			put("% scanner errors were not reported", scannerErrorsThatWereNotReported.size());
			scannerReporter.reset();
			return;
		}

		auto ast = parser.parse(tokens, &parserReporter);
		if (!ast.has_value() && !parserReporter.errorHappened) {
			ASSERT_NOT_REACHED();
			return;
		}

		setDifference(expectedParserErrors, parserReporter.errors);
		const auto parserErrorsThatWereNotReported = setDifference(expectedParserErrors, parserReporter.errors);
		if (parserErrorsThatWereNotReported.size() != 0) {
			printFailed(name);
			put("% parser errors were not reported", parserErrorsThatWereNotReported.size());
			parserReporter.reset();
			return;
		}

		printPassed(name);
	};

	runTestExpected("constant addition", "2 + 2", 4);
	runTestExpected("constant multiplication", "17 * 22", 374);
	runTestExpected("multiplication precedence", "2 + 3 * 4", 14);
	runTestExpected("parens", "(2 + 3) * 4", 20);

	runTestExpectedErrors(
		"illegal character", 
		"?2 + 2", 
		{ IllegalCharScannerError{ .character = '?', .sourceOffset = 0 }},
		{}
	);
}

void test() {
	std::string_view source = "2 + 2";

	std::ostream& outputStream = std::cerr;

	OstreamScannerMessageReporter scannerReporter(outputStream, source);
	Scanner scanner;
	const auto tokens = scanner.parse(source, &scannerReporter);
	for (auto& token : tokens) {
		debugOutputToken(token, source);
		std::cout << "\n";
		highlightInText(std::cout, source, tokenOffsetInSource(token, source), token.source.size());
		std::cout << "\n";
	}

	OstreamParserMessageReporoter parserReporter(outputStream, source);
	Parser parser;
	const auto ast = parser.parse(tokens, &parserReporter);
	if (ast.has_value()) {
		printExpr(ast->root, true);
		std::cout << " = ";
		std::cout << evaluateExpr(ast->root) << '\n';
	}

	IrCompiler compiler;
	const auto irCode = compiler.compile(*ast);
	if (irCode.has_value()) {
		printIrCode(std::cout, **irCode);
	}
}

void f0() {
	return;
}

using F = void(*)();

#include <stdio.h>
#include <windows.h>
#include <fstream>

// Can be nullptr.
void* allocateExecutableMemory(i64 size) {
	return reinterpret_cast<u8*>(VirtualAllocEx(
		GetCurrentProcess(),
		nullptr, // No desired starting address
		size, // Rounds to page size
		MEM_COMMIT,
		PAGE_EXECUTE_READWRITE
	));
}

#include <bitset>

// https://stackoverflow.com/questions/4911993/how-to-generate-and-run-native-code-dynamically
int main(void) {

	auto buffer = reinterpret_cast<u8*>(allocateExecutableMemory(4096));

	if (buffer == nullptr) {
		put("failed to allocate executable memory");
		return 0;
	}
	
	using Function = int (*)(void);

	enum class ModRMRegisterX86 : u8 {
		AX = 0b000,
		CX = 0b001,
		DX = 0b010,
		BX = 0b011,
		SP = 0b100,
		BP = 0b101,
		SI = 0b110,
		DI = 0b111
	};



	//auto encodeModRm = [](
	//	bool registerDirectAddressing, // MODRM.mod
	//	ModRMRegisterX86 reg // MODRM.reg
	//	ModRMRegisterX86
	//) {
	//	u8 v = 0;
	//	if (registerDirectAddressing) {
	//		v |= 0b11000000;
	//	} // else use register indirect addressing.

	//	if ()

	//};

	//auto encodeRexPrefixByte = [](bool w, bool r, bool x, bool b) -> u8 {
	//	return 0b0100'0000 | (w << 3) | (r << 2) | (x << 1) | static_cast<u8>(b);
	//};

	//auto encodeModRmByte = [](u8 mod /* 2 bit */, u8 reg /* 3 bit */, u8 rm /* 3 bit */) -> u8 {
	//	return (mod << 6) | (reg << 3) | rm;
	//};

	//auto encodeModRmDirectAddressing = [&](ModRMRegisterX86 g, ModRMRegisterX86 e) {
	//	return encodeModRmByte(0b11, static_cast<u8>(g), static_cast<u8>(e));
	//};


	//u8* p = buffer;

	//auto emitXorEaxEax = [&]() {
	//	//// xor eax, eax
	//	*p++ = 0x33;
	//	*p++ = 0xC0;
	//};
	//auto prefixByte = encodeRexPrefixByte(true, false, false, false);;
	//*p++ = prefixByte;

	////std::cout << std::bitset<8>(prefixByte) << '\n';;

	////std::cout << std::hex << encodeRexPrefixByte(true, false, false, false) << '\n';
	//// Add opcode
	//*p++ = encodeModRmDirectAddressing(ModRMRegisterX86::AX, ModRMRegisterX86::CX);
	//////encodeModRmDirectAddressing(ModRMRegisterX86::AX, ModRMRegisterX86::BP);

	//*p++ = 0xC3; // ret
	const auto f = reinterpret_cast<Function>(buffer);
	const auto out = f();

	std::ofstream bin("test.txt", std::ios::out | std::ios::binary);
	bin.write(reinterpret_cast<const char*>(buffer), p - buffer);
	//funcptr func;
	//func.y = buf;

	////arg1 = 123; arg2 = 321; res1 = 0;

	//func.x(); // call generated code

	//*p++ = 0x50; // push eax
	//*p++ = 0x52; // push edx

	//*p++ = 0xA1; // mov eax, [arg2]
	//(int*&)p[0] = &arg2; p += sizeof(int*);

	//*p++ = 0x92; // xchg edx,eax

	//*p++ = 0xA1; // mov eax, [arg1]
	//(int*&)p[0] = &arg1; p += sizeof(int*);

	//*p++ = 0xF7; *p++ = 0xEA; // imul edx

	//*p++ = 0xA3; // mov [res1],eax
	//(int*&)p[0] = &res1; p += sizeof(int*);

	//*p++ = 0x5A; // pop edx
	//*p++ = 0x58; // pop eax
	//*p++ = 0xC3; // ret

	

	//printf("arg1=%i arg2=%i arg1*arg2=%i func(arg1,arg2)=%i\n", arg1, arg2, arg1 * arg2, res1);

}

//int main() {
//	
//
//	//testMain();
//	//test();
//	return 0;
//}

//#define _CRT_SECURE_NO_WARNINGS
//#include <stdio.h>

//int main() {
//	printf("Ten program sluzy do rozwiazywania rownania liniowego\n");
//	float a, b;
//	printf("a = ");
//	scanf("%f", &a);
//	printf("b = ");
//	scanf("%f", &b);
//	printf("%gx + %g = 0\n", a, b);
//	if (a == 0) {
//		if (b == 0) {
//			printf("rownanie tozsamosciowe");
//		}
//		else {
//			printf("brak rozwiazan");
//		}
//	} else {
//		printf("x = %g\n", -b / a);
//	}
//
//	//
//}

//#define _CRT_SECURE_NO_WARNINGS
//#include <stdio.h>
//#include <math.h>

//void solveLinear(float a, float b) {
//	if (a == 0.0f) {
//		if (b == 0.0f) {
//			printf("rownanie tozsamosciowe");
//		}
//		else {
//			printf("brak rozwiazan");
//		}
//	} else {
//		printf("x = %g\n", -b / a);
//	}
//}
//
//int main() {
//	printf("Ten program sluzy do rozwiazywania ax^2 + bx + c = 0\n");
//	float a, b, c;
//	printf("a = ");
//	scanf("%f", &a);
//	printf("b = ");
//	scanf("%f", &b);
//	printf("c = ");
//	scanf("%f", &c);
//	
//	printf("%gx^2 + %gx = 0 + %g\n", a, b, c);
//
//	if (a == 0.0f) {
//		solveLinear(b, c);
//		return 0;
//	}
//
//	float discriminant = b * b - 4.0f * a * c;
//	if (discriminant < 0) {
//		printf("Brak rozwiazan rzeczywistych\n");
//	} else if (discriminant == 0) {
//		float x = -b / (2.0f * a);
//		printf("x = %g", x);
//	} else {
//		float sqrtDiscriminant = sqrt(discriminant);
//		float x0 = (-b + sqrtDiscriminant) / (2.0f * a);
//		float x1 = (-b - sqrtDiscriminant) / (2.0f * a);
//		printf("x0 = %g, x1 = %g", x0, x1);
//	}
//}

//#include <bit>

//bool isPowerOf2(int x) {
//	int bitCount = 0;
//	while (x) {
//		if (x & 1) {
//			bitCount++;
//			if (bitCount > 1) {
//				break;
//			}
//		}
//		x >>= 1;
//	}
//	return bitCount <= 1;
//}
//
//int main() {
//	int x;
//	printf("x = ");
//	scanf("%d", &x);
//
//	printf(isPowerOf2(x) ? "x is a power of 2" : "x is not a power of 2");
//
//	/*for (int i = 0; i < 1000; i++) {
//		if (isPowerOf2(i)) {
//			printf("%d\n", i);
//		}
//	}*/
//}

//bool isPrime(int x) {
//	if (x < 2) {
//		return false;
//	}
//
//	for (int i = 2; i <= static_cast<int>(sqrt(x)); i++) {
//		if (x % i == 0) {
//			return false;
//		}
//	}
//	return true;
//}
//
//int main() {
//	int x;
//	printf("x = ");
//	scanf("%d", &x);
//
//	printf(isPrime(x) ? "x is prime" : "x is not prime");
//
//	for (int i = 0; i < 1000; i++) {
//		if (isPrime(i)) {
//			printf("%d\n", i);
//		}
//	}
//}