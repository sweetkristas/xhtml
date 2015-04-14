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

#include <string>
#include <vector>

namespace css
{
	struct TokenizerError : public std::runtime_error 
	{
		TokenizerError(const char* str) : std::runtime_error(str) {}
	};
	
	enum class TokenId {
		IDENT,
		FUNCTION,
		AT,
		HASH,
		STRING,
		BAD_STRING,
		URL,
		BAD_URL,
		DELIM,
		NUMBER,
		PERCENT,
		DIMENSION,
		INCLUDE_MATCH,
		DASH_MATCH,
		PREFIX_MATCH,
		SUFFIX_MATCH,
		SUBSTRING_MATCH,
		COLUMN,
		WHITESPACE,
		CDO,
		CDC,
		COLON,
		SEMICOLON,
		COMMA,
		LBRACKET,
		RBRACKET,
		LPAREN,
		RPAREN,
		LBRACE,
		RBRACE,
		EOF_TOKEN,
	};

	enum class TokenFlags {
		UNRESTRICTED = 1,
		ID = 2,
	};

	struct Token {
		explicit Token(TokenId ident) : id(ident), tok(), flags(TokenFlags::UNRESTRICTED) {}
		explicit Token(TokenId ident, const std::string& s) : id(ident), tok(s), flags(TokenFlags::UNRESTRICTED) {}
		explicit Token(TokenId ident, const std::string& s, TokenFlags f) : id(ident), tok(s), flags(f) {}
		explicit Token(TokenId ident, TokenFlags f) : id(ident), tok(), flags(f) {}
		TokenId id;
		std::string tok;
		TokenFlags flags;
	};

	// The tokenizer class expects an input of unicode codepoints.
	class Tokenizer
	{
	public:
		explicit Tokenizer(const std::string& inp);
	private:
		void advance(int n=1);
		char32_t next(int n=1);
		bool eof(char32_t cp);
		std::string consumeEscape();
		void consumeWhitespace();
		void consumeComments();
		Token consumeString(char32_t end_codepoint);
		Token consumeNumericToken();
		Token consumeIdentlikeToken();
		std::string consumeNumber();
		std::string consumeName();
		Token consumeURLToken();
		void consumeBadURL();

		std::vector<char32_t> cp_string_;
		std::vector<char32_t>::size_type it_;

		// Look ahead+0
		char32_t la0_;
	};
}
