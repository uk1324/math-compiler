#include "debug.hpp"
#include "utils/pritningUtils.hpp"
#include "utils/put.hpp"

void debugOutputToken(const Token& token, std::string_view originalSource) {
	const auto end = token.location.start + token.location.length;
	const auto tokenSource = originalSource.substr(token.location.start, token.location.length);
	put("% % = % '%'", token.location.start, end, tokenTypeToStr(token.type), tokenSource);
}

void debugOutputTokens(const std::vector<Token>& tokens, std::string_view source) {
	for (auto& token : tokens) {
		debugOutputToken(token, source);
		std::cout << "\n";
		/*highlightInText(std::cout, source, tokenOffsetInSource(token, source), token.source.size());*/
		highlightInText(std::cout, source, token.location.start, token.location.length);
		std::cout << "\n";
	} 
}
