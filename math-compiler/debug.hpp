#pragma once

#include <vector>
#include "token.hpp"


void debugOutputToken(const Token& token, std::string_view originalSource);
void debugOutputTokens(const std::vector<Token>& tokens, std::string_view source);