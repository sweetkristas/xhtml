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

#include "css_lexer.hpp"
#include "css_selector.hpp"

namespace css
{
	struct ParserError : public std::runtime_error
	{
		ParserError(const char* msg) : std::runtime_error(msg) {}
		ParserError(const std::string& str) : str_(str), std::runtime_error(str_) {}
		std::string str_;
	};

	class StyleSheet
	{
	public:
		StyleSheet();
		void addRules(std::vector<TokenPtr>* rule);

		const std::vector<TokenPtr>& getRules() const { return rules_; }
	private:
		std::vector<TokenPtr> rules_;
	};
	typedef std::shared_ptr<StyleSheet> StyleSheetPtr;

	class Parser
	{
	public:
		Parser(const std::vector<TokenPtr>& tokens);
		const StyleSheetPtr& getStyleSheet() const { return style_sheet_; }
		static void parseRule(TokenPtr);
	private:
		std::vector<TokenPtr> pasrseRuleList(int level);
		TokenPtr parseAtRule();
		TokenPtr parseQualifiedRule();
		TokenPtr parseDeclarationList();
		TokenPtr parseDeclaration();
		TokenPtr parseImportant();
		TokenPtr parseComponentValue();
		std::vector<TokenPtr> parseBraceBlock();
		std::vector<TokenPtr> parseParenBlock();
		std::vector<TokenPtr> parseBracketBlock();
		TokenPtr parseFunction();

		TokenId currentTokenType();
		void advance(int n=1);

		StyleSheetPtr style_sheet_;
		std::vector<TokenPtr>::const_iterator token_;
		std::vector<TokenPtr>::const_iterator end_;
	};
}