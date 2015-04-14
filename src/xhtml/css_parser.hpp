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

namespace css
{
	enum class RuleId {
		AT,
		QUALIFIED,
		DECLARATION,
		BLOCK,
		FUNCTION,
		TOKEN,
	};

	class Rule;
	typedef std::shared_ptr<Rule> RulePtr;

	class Rule
	{
	public:
		explicit Rule(RuleId id) : id_(id) {}
		virtual ~Rule() {}
		virtual std::string toString() const = 0;
		void setValue(RulePtr value) { values_.clear(); values_.emplace_back(value); }
		void addValue(RulePtr value) { values_.emplace_back(value); }
		void addPrelude(RulePtr value) { prelude_.emplace_back(value); }
	private:
		RuleId id_;
		std::vector<RulePtr> values_;
		std::vector<RulePtr> prelude_;
	};

	class StyleSheet
	{
	public:
		StyleSheet();
		void addRules(std::vector<RulePtr>* rule);
	private:
		std::vector<RulePtr> rules_;
	};
	typedef std::shared_ptr<StyleSheet> StyleSheetPtr;

	class Parser
	{
	public:
		Parser(const std::vector<TokenPtr>& tokens);
	private:
		std::vector<RulePtr> pasrseRuleList(int level);
		RulePtr parseAtRule();
		RulePtr parseQualifiedRule();
		RulePtr parseDeclarationList();
		RulePtr parseDeclaration();
		RulePtr parseImportant();
		RulePtr parseComponentValue();
		RulePtr parseBraceBlock();
		RulePtr parseParenBlock();
		RulePtr parseBracketBlock();
		RulePtr parseFunction();

		TokenId currentTokenType();
		void advance(int n=1);

		StyleSheetPtr style_sheet_;
		std::vector<TokenPtr>::const_iterator token_;
		std::vector<TokenPtr>::const_iterator end_;
	};
}