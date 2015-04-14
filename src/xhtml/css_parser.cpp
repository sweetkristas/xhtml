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

#include "asserts.hpp"
#include "css_parser.hpp"

namespace css
{
	namespace 
	{
		// rules
		class AtRule : public Rule
		{
		public:
			AtRule(const std::string& name) : Rule(RuleId::AT) {}
			std::string toString() const override {
				return "AT-RULE";
			}
		private:
		};

		class BlockRule : public Rule
		{
		public:
			BlockRule() : Rule(RuleId::BLOCK) {}
			std::string toString() const override {
				return "BLOCK-RULE";
			}
		private:
		};

		class FunctionRule : public Rule
		{
		public:
			FunctionRule() : Rule(RuleId::FUNCTION) {}
			std::string toString() const override {
				return "FUNCTION-RULE";
			}
		private:
		};
		
		class QualifiedRule : public Rule
		{
		public:
			QualifiedRule() : Rule(RuleId::QUALIFIED) {}
			std::string toString() const override {
				return "QUALIFIED-RULE";
			}
		private:
		};

		// Special case rule for preserving token values.
		class PreservedToken : public Rule
		{
		public:
			PreservedToken(TokenId token_id, const variant& value) : Rule(RuleId::TOKEN), token_id_(token_id), value_(value) {}
			std::string toString() const override {
				return "PRESERVED-TOKEN";
			}
		private:
			TokenId token_id_;
			variant value_;
		};
	}

	Parser::Parser(const std::vector<TokenPtr>& tokens)
		: style_sheet_(std::make_shared<StyleSheet>()),
		  token_(tokens.begin()),
		  end_(tokens.end())
	{
		style_sheet_->addRules(&pasrseRuleList(0));
	}

	TokenId Parser::currentTokenType()
	{
		if(token_ == end_) {
			return TokenId::EOF_TOKEN;
		}
		return (*token_)->id();
	}

	void Parser::advance(int n)
	{
		std::advance(token_, n);
	}

	std::vector<RulePtr> Parser::pasrseRuleList(int level)
	{
		std::vector<RulePtr> rules;
		while(true) {
			if(currentTokenType() == TokenId::WHITESPACE) {
				advance();
				continue;
			} else if(currentTokenType() == TokenId::EOF_TOKEN) {
				return rules;
			} else if(currentTokenType() == TokenId::CDO || currentTokenType() == TokenId::CDC) {
				if(level == 0) {
					advance();
					continue;
				}
				rules.emplace_back(parseQualifiedRule());
			} else if(currentTokenType() == TokenId::AT) {
				rules.emplace_back(parseAtRule());
			} else {
				rules.emplace_back(parseQualifiedRule());
			}
		}
		return rules;
	}

	RulePtr Parser::parseAtRule()
	{
		variant value = (*token_)->value();
		auto rule = std::make_shared<AtRule>(value.as_string());
		advance();
		while(true) {
			if(currentTokenType() == TokenId::SEMICOLON || currentTokenType() == TokenId::EOF_TOKEN) {
				return rule;
			} else if(currentTokenType() == TokenId::LBRACE) {
				advance();
				rule->setValue(parseBraceBlock());
			} else if(currentTokenType() == TokenId::LPAREN) {
				advance();
				rule->setValue(parseParenBlock());
			} else if(currentTokenType() == TokenId::LBRACKET) {
				advance();
				rule->setValue(parseBracketBlock());
			}
		}
		return nullptr;
	}

	RulePtr Parser::parseQualifiedRule()
	{
		auto rule = std::make_shared<QualifiedRule>();
		while(true) {
			if(currentTokenType() == TokenId::EOF_TOKEN) {
				LOG_ERROR("EOF token while parsing qualified rule prelude.");
				return nullptr;
			} else if(currentTokenType() == TokenId::LBRACE) {
				advance();
				rule->setValue(parseBraceBlock());
				return rule;
			} else {
				rule->addPrelude(parseComponentValue());
			}
		}
		return nullptr;
	}

	RulePtr Parser::parseDeclarationList()
	{
		// XXX
		return nullptr;
	}

	RulePtr Parser::parseDeclaration()
	{
		// XXX
		return nullptr;
	}

	RulePtr Parser::parseImportant()
	{
		// XXX
		return nullptr;
	}

	RulePtr Parser::parseComponentValue()
	{
		if(currentTokenType() == TokenId::LBRACE) {
			advance();
			return parseBraceBlock();
		} else if(currentTokenType() == TokenId::FUNCTION) {
			return parseFunction();
		}
		variant value = (*token_)->value();
		auto tok = std::make_shared<PreservedToken>(currentTokenType(), value);
		advance();
		return tok;
	}

	RulePtr Parser::parseBraceBlock()
	{
		auto block = std::make_shared<BlockRule>();
		block->addValue(std::make_shared<PreservedToken>(currentTokenType(), (*token_)->value()));
		while(true) {
			if(currentTokenType() == TokenId::EOF_TOKEN || currentTokenType() == TokenId::RBRACE) {
				advance();
				return block;
			} else {
				block->addValue(parseComponentValue());
			}
		}
		return nullptr;
	}

	RulePtr Parser::parseParenBlock()
	{
		auto block = std::make_shared<BlockRule>();
		block->addValue(std::make_shared<PreservedToken>(currentTokenType(), (*token_)->value()));
		while(true) {
			if(currentTokenType() == TokenId::EOF_TOKEN || currentTokenType() == TokenId::RPAREN) {
				advance();
				return block;
			} else {
				block->addValue(parseComponentValue());
			}
		}
		return nullptr;
	}

	RulePtr Parser::parseBracketBlock()
	{
		auto block = std::make_shared<BlockRule>();
		block->addValue(std::make_shared<PreservedToken>(currentTokenType(), (*token_)->value()));
		while(true) {
			if(currentTokenType() == TokenId::EOF_TOKEN || currentTokenType() == TokenId::RBRACKET) {
				advance();
				return block;
			} else {
				block->addValue(parseComponentValue());
			}
		}
		return nullptr;
	}

	RulePtr Parser::parseFunction()
	{
		auto func = std::make_shared<FunctionRule>();
		func->addValue(std::make_shared<PreservedToken>(currentTokenType(), (*token_)->value()));
		advance();
		while(true) {
			if(currentTokenType() == TokenId::EOF_TOKEN || currentTokenType() == TokenId::RPAREN) {
				return func;
			} else {
				func->addValue(parseComponentValue());
			}
		}
		return nullptr;
	}

	// StyleSheet functions
	StyleSheet::StyleSheet()
		: rules_()
	{
	}

	void StyleSheet::addRules(std::vector<RulePtr>* rules)
	{
		rules_.swap(*rules);
	}
}
