#include "ostreamParserMessageReporter.hpp"
#include "utils/overloaded.hpp"
#include "utils/pritningUtils.hpp"
#include "utils/put.hpp"
#include "sourceInfo.hpp"

OstreamParserMessageReporter::OstreamParserMessageReporter(std::ostream& output, std::string_view source)
	: source(source)
	, output(output) {}

static const char* tokenTypeName(TokenType type) {
	switch (type) {
	case TokenType::FLOAT: return "floating point constant";
	case TokenType::PLUS: return "'+'";
	case TokenType::MINUS: return "'-'";
	case TokenType::STAR: return "'*'";
	case TokenType::SLASH: return "'/'";
	case TokenType::LEFT_PAREN: return "'('";
	case TokenType::RIGHT_PAREN: return "')'";
	case TokenType::IDENTIFIER: return "identifier";
	case TokenType::END_OF_SOURCE: return "end of source";

	case TokenType::ERROR: 
		// Error tokens should be ignored.
		ASSERT_NOT_REACHED();
		return nullptr;
	}
	ASSERT_NOT_REACHED();
	return nullptr;
};

void OstreamParserMessageReporter::onError(const ParserError& error) {
	std::visit(overloaded{
		[&](const UnexpectedTokenParserError& e) {
			if (e.token.type == TokenType::ERROR) {
				return;
			}
			const auto& location = e.token.location;
			highlightInText(output, source, location.start, location.length);
			put(output, "unexpected %", tokenTypeName(e.token.type));
		},
		[&](const ExpectedTokenParserError& e) {
			if (e.found.type == TokenType::ERROR) {
				return;
			}
			const auto& location = e.found.location;
			highlightInText(output, source, location.start, location.length);
			put(output, "expected % found %", tokenTypeName(e.expected), tokenTypeName(e.found.type));
		},
	}, error);
}
