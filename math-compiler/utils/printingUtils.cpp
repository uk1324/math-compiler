#include "pritningUtils.hpp"
#include "put.hpp"

void highlightInText(std::ostream& output, std::string_view text, i64 textToHighlightStart, i64 textToHighlightLength) {
	i64 currentCharIndex = 0;
	i64 printedCharsCount = 0;

	auto charLength = [](char c) -> i64 {
		if (c == '\n') {
			return 2;
		}
		else {
			return 1;
		}
	};

	for (auto c : text) {
		if (c == '\n') {
			putnn(output, "\\n");
			if (currentCharIndex < textToHighlightStart) {
				currentCharIndex++;
				printedCharsCount += charLength('\n');
			}
		}
		else {
			output << c;
			if (currentCharIndex < textToHighlightStart) {
				currentCharIndex++;
				printedCharsCount += charLength(c);
			}
		}
	}
	putnn(output, "\n");
	for (i64 i = 0; i < printedCharsCount; i++) {
		putnn(output, " ");
	}
	for (i64 i = 0; i < textToHighlightLength; i++) {
		const auto currentCharIndex = textToHighlightStart + i;
		ASSERT_NOT_NEGATIVE(currentCharIndex);
		for (i64 j = 0; j < charLength(text[static_cast<usize>(currentCharIndex)]); j++) {
			putnn(output, "^");
		}
	}
}