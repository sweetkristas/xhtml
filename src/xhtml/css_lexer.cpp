/*
	Copyright (C) 2003-2013 by Kristina Simpson <sweet.kristas@gmail.com>
	
	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	   1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.

	   2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.

	   3. This notice may not be removed or altered from any source
	   distribution.
*/

#include <sstream>

#include "asserts.hpp"
#include "css_lexer.hpp"
#include "utf8_to_codepoint.hpp"

namespace css
{
	namespace 
	{
		const char32_t NULL_CP = 0x0000;
		const char32_t CR = 0x000d;
		const char32_t LF = 0x000a;
		const char32_t FF = 0x000c;
		const char32_t TAB = 0x0009;
		const char32_t REPLACEMENT_CHAR = 0xfffdUL;
		const char32_t SPACE = 0x0020;
		const char32_t MAX_CODEPOINT = 0x10ffff;

		bool between(char32_t num, char32_t first, char32_t last) { return num >= first && num <= last; }
		bool digit(char32_t code) { return between(code, 0x30, 0x39); }
		bool hexdigit(char32_t code) { return digit(code) || between(code, 0x41, 0x46) || between(code, 0x61, 0x66); }
		bool newline(char32_t code) { return code == LF; }
		bool whitespace(char32_t code) { return newline(code) || code == TAB || code == SPACE; }
		bool uppercaseletter(char32_t code) { return between(code, 0x41,0x5a); }
		bool lowercaseletter(char32_t code) { return between(code, 0x61,0x7a); }
		bool letter(char32_t code) { return uppercaseletter(code) || lowercaseletter(code); }
		bool nonascii(char32_t code) { return code >= 0x80; }
		bool namestartchar(char32_t code) { return letter(code) || nonascii(code) || code == 0x5f; }
		bool namechar(char32_t code) { return namestartchar(code) || digit(code) || code == 0x2d; }
		bool nonprintable(char32_t code) { return between(code, 0,8) || code == 0xb || between(code, 0xe,0x1f) || code == 0x7f; }

		bool is_valid_escape(char32_t cp1, char32_t cp2) {
			if(cp1 != '\\') {
				return false;
			}
			return newline(cp2) ? false : true;
		}

		bool woud_start_an_identifier(char32_t cp1, char32_t cp2, char32_t cp3) {
			if(cp1 == '-') {
				return namestartchar(cp2) || cp2 == '-' || is_valid_escape(cp2, cp3);
			} else if(namestartchar(cp1)) {
				return true;
			} else if(cp1 == '\\') {
				return is_valid_escape(cp1, cp2);
			}
			return false;
		}

		bool would_start_a_number(char32_t cp1, char32_t cp2, char32_t cp3) {
			if(cp1 == '+' || cp1 == '-') {
				return digit(cp2) || (cp2 == '.' && digit(cp3));
			} else if(cp1 == '.') {
				return digit(cp2);
			} 
			return digit(cp1);
		}
	}

	Tokenizer::Tokenizer(const std::string& inp)
	{
		std::vector<Token> tokens;
		// Replace CR, FF and CR/LF pairs with LF
		// Replace U+0000 with U+FFFD
		bool is_lf = false;
		for(auto ch : utils::utf8_to_codepoint(inp)) {
			switch(ch) {
			case NULL_CP: 
				if(is_lf) {
					cp_string_.emplace_back(LF);
				}
				cp_string_.emplace_back(REPLACEMENT_CHAR); 
				break;
			case CR: // fallthrough
			case LF:
			case FF: is_lf = true; break;
			default: 
				if(is_lf) {
					cp_string_.emplace_back(LF);
				}
				cp_string_.emplace_back(ch);
				break;
			}
		}

		// tokenize string.
		it_ = 0;
		la0_ = cp_string_[0];
		while(it_ < cp_string_.size()) {
			if(la0_ == '/' && next() == '*') {
				consumeComments();
			}
			if(la0_ == LF || la0_ == TAB || la0_ == SPACE) {
				consumeWhitespace();
				tokens.emplace_back(TokenId::WHITESPACE);
			} else if(la0_ == '"') {
				tokens.emplace_back(consumeString(la0_));
			} else if(la0_ == '#') {
				if(namechar(next()) || is_valid_escape(next(1), next(2))) {
					tokens.emplace_back(TokenId::HASH, 
						woud_start_an_identifier(next(1), next(2), next(3)) ? TokenFlags::ID : TokenFlags::UNRESTRICTED, 
						consumeName());
				} else {
					tokens.emplace_back(TokenId::DELIM, utils::codepoint_to_utf8(la0_));
				}
			} else if(la0_ == '$') {
				if(next() == '=') {
					advance(2);
					tokens.emplace_back(TokenId::SUFFIX_MATCH);
				} else {
					tokens.emplace_back(TokenId::DELIM, utils::codepoint_to_utf8(la0_));
					advance();
				}
			} else if(la0_ == '\'') {
				tokens.emplace_back(consumeString(la0_));
			} else if(la0_ == '(') {
				advance();
				tokens.emplace_back(TokenId::LPAREN);
			} else if(la0_ == ')') {
				advance();
				tokens.emplace_back(TokenId::RPAREN);
			} else if(la0_ == '*') {
				if(next() == '=') {
					advance(2);
					tokens.emplace_back(TokenId::SUBSTRING_MATCH);
				} else {
					tokens.emplace_back(TokenId::DELIM, utils::codepoint_to_utf8(la0_));
					advance();
				}
			} else if(la0_ == '+') {
				if(would_start_a_number(la0_, next(1), next(2))) {
					tokens.emplace_back(consumeNumericToken());
				}
			} else if(la0_ == ',') {
				advance();
				tokens.emplace_back(TokenId::COMMA);
			} else if(la0_ == '-') {
				if(would_start_a_number(la0_, next(1), next(2))) {
					tokens.emplace_back(consumeNumericToken());
				} else if(next(1) == '-' && next(2) == '>') {
					tokens.emplace_back(TokenId::CDC);
					advance(3);
				} else if(woud_start_an_identifier(la0_, next(1), next(2))) {
					tokens.emplace_back(consumeIdentlikeToken());
				} else {
					tokens.emplace_back(TokenId::DELIM, utils::codepoint_to_utf8(la0_));
					advance();
				}
			} else if(la0_ == '.') {
				if(would_start_a_number(la0_, next(1), next(2))) {
					tokens.emplace_back(consumeNumericToken());
				} else {
					advance();
					tokens.emplace_back(TokenId::DELIM, ".");
				}
			} else if(la0_ == ':') {
				advance();
				tokens.emplace_back(TokenId::COLON);
			} else if(la0_ == ';') {
				advance();
				tokens.emplace_back(TokenId::SEMICOLON);
			} else if(la0_ == '<') {
				if(next(1) == '!' && next(2) == '-' && next(3) == '-') {
					tokens.emplace_back(TokenId::CDO);
				} else {
					advance();
					tokens.emplace_back(TokenId::DELIM, "<");
				}
			} else if(la0_ == '@') {
				if(woud_start_an_identifier(next(1), next(2), next(3))) {
					advance();
					tokens.emplace_back(TokenId::AT, consumeName());
				} else {
					advance();
					tokens.emplace_back(TokenId::DELIM, "@");
				}
			} else if(la0_ == '[') {
				advance();
				tokens.emplace_back(TokenId::LBRACKET);
			} else if(la0_ == '\\') {
				if(is_valid_escape(la0_, next(1))) {
					tokens.emplace_back(consumeIdentlikeToken());
				} else {
					LOG_ERROR("Parse error while processing codepoint: " << utils::codepoint_to_utf8(la0_));
					tokens.emplace_back(TokenId::DELIM, "\\");
				}
			} else if(la0_ == ']') {
				advance();
				tokens.emplace_back(TokenId::RBRACKET);
			} else if(la0_ == '^') {
				if(next() == '=') {
					tokens.emplace_back(TokenId::PREFIX_MATCH);
					advance(2);
				} else {
					tokens.emplace_back(TokenId::DELIM, "^");
					advance();
				}
			} else if(la0_ == '{') {
				advance();
				tokens.emplace_back(TokenId::LBRACE);
			} else if(la0_ == '}') {
				advance();
				tokens.emplace_back(TokenId::RBRACE);
			} else if(digit(la0_)) {
				tokens.emplace_back(consumeNumericToken());
			} else if(namestartchar(la0_)) {
				tokens.emplace_back(consumeIdentlikeToken());
			} else if(la0_ == '|') {
				if(next() == '=') {
					tokens.emplace_back(TokenId::DASH_MATCH);
					advance(2);
				} else if(next() == '|') {
					tokens.emplace_back(TokenId::COLUMN);
					advance(2);
				} else {
					tokens.emplace_back(TokenId::DELIM, "|");
					advance();
				}
			} else if(la0_ == '~') {
				if(next() == '=') {
					tokens.emplace_back(TokenId::INCLUDE_MATCH);
					advance(2);
				} else {
					tokens.emplace_back(TokenId::DELIM, "^");
					advance();
				}
			} else if(eof(la0_)) {
				tokens.emplace_back(TokenId::EOF_TOKEN);
			} else {
				tokens.emplace_back(TokenId::DELIM, utils::codepoint_to_utf8(la0_));
			}
		}
	}

	void Tokenizer::advance(int n)
	{
		it_ += n;
		if(it_ >= cp_string_.size()) {
			throw TokenizerError("EOF error");
		}
		la0_ = cp_string_[it_];
	}

	bool Tokenizer::eof(char32_t cp)
	{
		return cp == -1;
	}

	char32_t Tokenizer::next(int n)
	{
		if(n > 3) {
			throw TokenizerError("Out of spec error, no more than three codepoints of lookahead");
		}
		if(it_ + n >= cp_string_.size()) {
			return -1;
		}
		return cp_string_[it_ + n];
	}

	void Tokenizer::consumeWhitespace()
	{
		// consume LF/TAB/SPACE
		while(la0_ == LF || la0_ == TAB || la0_ == SPACE) {
			advance();
		}
	}

	void Tokenizer::consumeComments()
	{
		advance(2);
		while(it_ < cp_string_.size()-2) {
			if(la0_ == '*' && next() == '/') {
				return;
			}
			advance();
		}
		throw TokenizerError("EOF in comments");
	}

	Token Tokenizer::consumeString(char32_t end_codepoint)
	{
		std::string res;
		advance();
		while(la0_ != end_codepoint) {
			if(la0_ == LF) {
				return BadStringToken();
			} else if(eof(la0_)) {
				return StringToken(res);
			} else if(la0_ == '\\') {
				if(eof(next())) {
					// does nothing.
				} else if(next() == LF) {
					advance();
					continue;
				} else {
					res += consumeEscape();
				}
			}
			res += utils::codepoint_to_utf8(la0_);
			advance();
		}
		return StringToken(res);
	}

	std::string Tokenizer::consumeEscape()
	{
		std::string res;
		advance();
		if(hexdigit(la0_)) {
			std::string digits;
			digits.push_back(static_cast<char>(la0_));
			for(int n = 1; n < 6 && hexdigit(next()); ++n) {
				advance();
				digits.push_back(static_cast<char>(la0_));
			}
			if(whitespace(next())) {
				advance();
			}
			char32_t value;
			std::stringstream ss;
			ss << std::hex << digits;
			ss >> value;
			if(value >= MAX_CODEPOINT) {
				value = REPLACEMENT_CHAR;
			}
			res = utils::codepoint_to_utf8(value);
		} else if(eof(la0_)) {
			res = utils::codepoint_to_utf8(REPLACEMENT_CHAR);
		} else {
			res = utils::codepoint_to_utf8(la0_);
		}
		return res;
	}

	std::string Tokenizer::consumeName()
	{
		std::string res;
		while(true) {
			advance();
			if(namechar(la0_)) {
				res += utils::codepoint_to_utf8(la0_);
			} else if(is_valid_escape(la0_, next())) {
				res += consumeEscape();
			} else {
				advance(-1);
			}
		}
		return res;
	}

	Token Tokenizer::consumeNumericToken()
	{
		std::string res;
		auto num = consumeNumber();
		if(woud_start_an_identifier(la0_, next(1), next(2))) {
			return DimensionToken(num, consumeName());
		} else if(la0_ == '%') {
			return PercentToken(num);
		}
		return NumberToken(num);
	}

	std::string Tokenizer::consumeNumber()
	{
		std::string res;
		if(la0_ == '-' || la0_ == '+') {
			res += utils::codepoint_to_utf8(la0_);
			advance();
		}
		while(digit(la0_)) {
			res += utils::codepoint_to_utf8(la0_);
			advance();
		}
		if(la0_ == '.' && digit(next(1))) {
			res += utils::codepoint_to_utf8(la0_);
			advance();
			while(digit(la0_)) {
				res += utils::codepoint_to_utf8(la0_);
				advance();
			}
		}
		if((la0_ == 'e' || la0_ == 'E') && digit(next(1))) {
			res += utils::codepoint_to_utf8(la0_);
			advance();
			res += utils::codepoint_to_utf8(la0_);
			advance();
			while(digit(la0_)) {
				res += utils::codepoint_to_utf8(la0_);
				advance();
			}
		} else if((la0_ == 'e' || la0_ == 'E') && (next(1) == '-' || next(1) == '+') && digit(next(2))) {
			res += utils::codepoint_to_utf8(la0_);
			advance();
			res += utils::codepoint_to_utf8(la0_);
			advance();
			res += utils::codepoint_to_utf8(la0_);
			advance();
			while(digit(la0_)) {
				res += utils::codepoint_to_utf8(la0_);
				advance();
			}
		}
		return res;
	}

	Token Tokenizer::consumeIdentlikeToken()
	{
		std::string str = consumeName();
		if(str.size() >= 4 && (str[0] == 'u' || str[0] == 'U') 
			&& (str[1] == 'r' || str[1] == 'R')
			&& (str[2] == 'l' || str[2] == 'L') 
			&& str[3] == '(') {
			while(whitespace(la0_) && whitespace(next())) {
				advance();
			}
			if(la0_ == '\'' || la0_ == '"') {
				return FunctionToken(str);
			} else if(whitespace(la0_) && (next() == '\'' || next() == '"')) {
				return FunctionToken(str);
			} else {
				return consumeURLToken();
			}
		} else if(la0_ == '.') {
			advance();
			return FunctionToken(str);
		}
		return IdentToken(str);
	}

	Token Tokenizer::consumeURLToken()
	{
		std::string res;
		while(whitespace(la0_)) {
			advance();
		}
		if(eof(la0_)) {
			return URLToken();
		}
		while(true) {
			if(la0_ == ')' || eof(la0_)) {
				return URLToken(res);
			} else if(whitespace(la0_)) {
				while(whitespace(la0_)) {
					advance();
				}
				if(la0_ == ')' || eof(la0_)) {
					advance;
					return URLToken(res);
				} else {
					consumeBadURL();
					return BadURLToken();
				}
			} else if(la0_ == '"' || la0_ == '\'' || la0_ == '(' || nonprintable(la0_)) {
				LOG_ERROR("Parse error while processing codepoint: " << utils::codepoint_to_utf8(la0_));
				consumeBadURL();
				return BadURLToken();
			} else if(la0_ == '\\') {
				if(is_valid_escape(la0_, next(1))) {
					res += consumeEscape();
				} else {
					LOG_ERROR("Parse error while processing codepoint: " << utils::codepoint_to_utf8(la0_));
					consumeBadURL();
					return BadURLToken();
				}
			} else {
				res += utils::codepoint_to_utf8(la0_);
				advance();
			}
		}
	}

	void Tokenizer::consumeBadURL()
	{
		while(true) {
			if(la0_ == '-' || eof(la0_)) {
				return;
			} else if(is_valid_escape(la0_, next(1))) {
				consumeEscape();
			} else {
				advance();
			}
		}
	}
}
