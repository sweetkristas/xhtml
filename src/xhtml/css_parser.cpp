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
		class AtRule : public Token
		{
		public:
			AtRule(const std::string& name) : Token(TokenId::AT_RULE_TOKEN), name_(name) {}
			std::string toString() const override {
				std::ostringstream ss;
				for(auto& p : getParameters()) {
					ss << " " << p->toString();
				}
				return formatter() << "@" << name_ << "(" << ss.str() << ")";
			}
		private:
			std::string name_;
		};

		class RuleToken : public Token
		{
		public:
			RuleToken() : Token(TokenId::RULE_TOKEN) {}
			std::string toString() const override {
				std::ostringstream ss;
				for(auto& p : getParameters()) {
					ss << " " << p->toString();
				}
				return formatter() << "QualifiedRule(" << ss.str() << ")";
			}
		private:
		};

		class BlockToken : public Token
		{
		public:
			BlockToken() : Token(TokenId::BLOCK_TOKEN) {}
			explicit BlockToken(const std::vector<TokenPtr>& params) 
				: Token(TokenId::BLOCK_TOKEN) 
			{
				addParameters(params);
			}
			std::string toString() const override {
				std::ostringstream ss;
				for(auto& p : getParameters()) {
					ss << " " << p->toString();
				}
				return formatter() << "BlockToken(" << ss.str() << ")";
			}
			variant value() override { return variant(); }
		private:
		};

		class SelectorToken : public Token
		{
		public:
			SelectorToken() : Token(TokenId::SELECTOR_TOKEN) {}
			std::string toString() const override {
				std::ostringstream ss;
				for(auto& p : getParameters()) {
					ss << " " << p->toString();
				}
				return formatter() << "Selector(" << ss.str() << ")";
			};
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

	std::vector<TokenPtr> Parser::pasrseRuleList(int level)
	{
		std::vector<TokenPtr> rules;
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

	TokenPtr Parser::parseAtRule()
	{
		variant value = (*token_)->value();
		auto rule = std::make_shared<AtRule>(value.as_string());
		advance();
		while(true) {
			if(currentTokenType() == TokenId::SEMICOLON || currentTokenType() == TokenId::EOF_TOKEN) {
				return rule;
			} else if(currentTokenType() == TokenId::LBRACE) {
				advance();
				rule->addParameters(parseBraceBlock());
			} else if(currentTokenType() == TokenId::LPAREN) {
				advance();
				rule->addParameters(parseParenBlock());
			} else if(currentTokenType() == TokenId::LBRACKET) {
				advance();
				rule->addParameters(parseBracketBlock());
			}
		}
		return nullptr;
	}

	TokenPtr Parser::parseQualifiedRule()
	{
		auto rule = std::make_shared<RuleToken>();
		while(true) {
			if(currentTokenType() == TokenId::EOF_TOKEN) {
				LOG_ERROR("EOF token while parsing qualified rule prelude.");
				return nullptr;
			} else if(currentTokenType() == TokenId::LBRACE) {
				advance();
				rule->setValue(std::make_shared<BlockToken>(parseBraceBlock()));
				return rule;
			} else {
				rule->addParameter(parseComponentValue());
			}
		}
		return nullptr;
	}

	TokenPtr Parser::parseDeclarationList()
	{
		// XXX
		return nullptr;
	}

	TokenPtr Parser::parseDeclaration()
	{
		// XXX
		return nullptr;
	}

	TokenPtr Parser::parseImportant()
	{
		// XXX
		return nullptr;
	}

	TokenPtr Parser::parseComponentValue()
	{
		if(currentTokenType() == TokenId::LBRACE) {
			advance();
			return std::make_shared<BlockToken>(parseBraceBlock());
		} else if(currentTokenType() == TokenId::FUNCTION) {
			return parseFunction();
		}
		
		auto tok = *token_;
		advance();
		return tok;
	}

	std::vector<TokenPtr> Parser::parseBraceBlock()
	{
		std::vector<TokenPtr> res;
		res.emplace_back(*token_);
		while(true) {
			if(currentTokenType() == TokenId::EOF_TOKEN || currentTokenType() == TokenId::RBRACE) {
				advance();
				return res;
			} else {
				res.emplace_back(parseComponentValue());
			}
		}
		return res;
	}

	std::vector<TokenPtr> Parser::parseParenBlock()
	{
		std::vector<TokenPtr> res;
		res.emplace_back(*token_);
		while(true) {
			if(currentTokenType() == TokenId::EOF_TOKEN || currentTokenType() == TokenId::RPAREN) {
				advance();
				return res;
			} else {
				res.emplace_back(parseComponentValue());
			}
		}
		return res;
	}

	std::vector<TokenPtr> Parser::parseBracketBlock()
	{
		std::vector<TokenPtr> res;
		res.emplace_back(*token_);
		while(true) {
			if(currentTokenType() == TokenId::EOF_TOKEN || currentTokenType() == TokenId::RBRACKET) {
				advance();
				return res;
			} else {
				res.emplace_back(parseComponentValue());
			}
		}
		return res;
	}

	TokenPtr Parser::parseFunction()
	{
		auto fn_token = *token_;
		advance();
		while(true) {
			if(currentTokenType() == TokenId::EOF_TOKEN || currentTokenType() == TokenId::RPAREN) {
				advance();
				return fn_token;
			} else {
				fn_token->addParameter(parseComponentValue());
			}
		}
		return fn_token;
	}

	void Parser::parseRule(TokenPtr rule)
	{
		std::ostringstream ss;
		ss << "RULE. prelude:";
		for(auto& r : rule->getParameters()) {
			ss << " " << r->toString();
		}
		ss << "; values: " << rule->getValue()->toString();
		LOG_DEBUG(ss.str());

		auto prelude = rule->getParameters().begin();
		while((*prelude)->id() == TokenId::WHITESPACE) {
			++prelude;
		}

		if((*prelude)->id() == TokenId::AT_RULE_TOKEN) {
			// parse at rule

			// XXX temporarily skip @ rules.
			//while(!(*prelude)->isToken(TokenId::SEMICOLON) && !(*prelude)->isToken(TokenId::RBRACE) && prelude != rule->getPrelude().end()) {
			//}
			ASSERT_LOG(false, "fix @ rules.");
		} else {
			SelectorParser selectors(prelude, rule->getParameters().end());
		}
	}

	// StyleSheet functions
	StyleSheet::StyleSheet()
		: rules_()
	{
	}

	void StyleSheet::addRules(std::vector<TokenPtr>* rules)
	{
		rules_.swap(*rules);
	}

	SelectorParser::SelectorParser(std::vector<TokenPtr>::const_iterator it, std::vector<TokenPtr>::const_iterator end)
		: it_(it),
		  end_(end)
	{
		parseSelectorGroup();
	}

	bool SelectorParser::isCurrentToken(TokenId value)
	{
		if(it_ == end_) {
			return false;
		}
		return (*it_)->id() == value;
	}

	void SelectorParser::advance(int n)
	{
		std::advance(it_, n);
	}

	std::vector<SelectorPtr> SelectorParser::parseSelectorGroup()
	{
		parseSelector();
		while(isCurrentToken(TokenId::COMMA)) {
			advance();
			whitespace();
			parseSelector();
		}
		std::vector<SelectorPtr> res;
		return res;
	}

	void SelectorParser::parseSelector()
	{
		parseSimpleSelectorSeq();
		while(isCombinator()) {
			parseCombinator();
			parseSimpleSelectorSeq();
		}
	}

	void SelectorParser::parseCombinator()
	{
		if(isCurrentTokenDelim("+") || isCurrentTokenDelim(">") || isCurrentTokenDelim("~")) {
			// XXX store combinator here.
			advance();
			whitespace();
		}
	}

	bool SelectorParser::isCurrentTokenDelim(const std::string& ch)
	{
		return isCurrentToken(TokenId::DELIM) && (*it_)->value().as_string() == ch;
	}

	bool SelectorParser::parseSeqModifiers()
	{
		bool res = true;
		// [ HASH | class | attrib | pseudo | negation ]
		if(isCurrentToken(TokenId::HASH)) {
			// XXX Store token attributes.
			advance();
		} else if(isCurrentTokenDelim(".")) {
			advance();
			if(!isCurrentToken(TokenId::IDENT)) {
				LOG_ERROR("did not find identifier after matching class selector");
			}
			// XXX Store class identifier.
			advance();
		} else if(isCurrentTokenDelim("[")) {
			advance();
			parseAttribute();
		} else if(isCurrentTokenDelim(":")) {
			advance();
			if(isCurrentTokenDelim(":")) {
				// XXX is pseudo-element
				advance();
			} else {
				// XXX is pseudo-class
			}
			if(isCurrentToken(TokenId::IDENT)) {
				// XXX store identifier
				advance();
			} else if(isCurrentToken(TokenId::FUNCTION)) {
				// XXX store function name and parameters.
				// may need to parse parameters with
				//  [ [ PLUS | '-' | DIMENSION | NUMBER | STRING | IDENT ] S* ]+
				advance();
			}
		//} else if(isCurrentToken(TokenId::NOT) {
		} else {
			res = false;
		}
		return res;
	}

	void SelectorParser::parseSimpleSelectorSeq()
	{
		// [ type_selector | universal ]
		if((isCurrentToken(TokenId::IDENT) || isCurrentTokenDelim("*")) && isNextTokenDelim("|")) {
			// XXX parse namespace selector
			advance(2);
		}

		if(isCurrentToken(TokenId::IDENT)) {
			// XXX element name;
			advance();
		} else if(isCurrentTokenDelim("*")) {
			// XXX universal element
			advance();
		} else {
			parseSeqModifiers();
		}

		while(parseSeqModifiers()) {
		}
	}

	bool SelectorParser::isCombinator()
	{
		bool res = false;
		while(isCurrentToken(TokenId::WHITESPACE)) {
			advance();
			res = true;
		}
		if(isCurrentTokenDelim("+") || isCurrentTokenDelim(">") || isCurrentTokenDelim("~")) {
			res = true;
		}
		return res;
	}

	void SelectorParser::parseAttribute()
	{
		// assume first '[' already removed.
		// remove whitespace.
		whitespace();
		// [ namespace_prefix ]?
		if((isCurrentToken(TokenId::IDENT) || isCurrentTokenDelim("*")) || isNextTokenDelim("|")) {
			advance(2);
			// XXX Set namespace prefix.
		}
		if(!isCurrentToken(TokenId::IDENT)) {
			LOG_ERROR("error no identifier found in selector production.");
			// XXX recovery			
		}
		// XXX store selector identifier
		if(isCurrentToken(TokenId::PREFIX_MATCH) 
			|| isCurrentToken(TokenId::SUFFIX_MATCH) 
			|| isCurrentToken(TokenId::SUBSTRING_MATCH) 
			|| isCurrentTokenDelim("=") 
			|| isCurrentToken(TokenId::INCLUDE_MATCH)
			|| isCurrentToken(TokenId::DASH_MATCH)) {
			// XXX store match type
			advance();
			whitespace();
			if(isCurrentToken(TokenId::STRING)) {
				// XXX add string to attribute
			} else if (!isCurrentToken(TokenId::IDENT)) {
				// XXX add ident to attribute
			} else {
				LOG_ERROR("Didn't match right bracket in production.");
				// XXX throw error and re-sync to next rule
			}
			whitespace();
		}

		if(!isCurrentTokenDelim("]")) {
			advance();
			LOG_ERROR("Didn't match right bracket in production.");
			// XXX throw error and re-sync to next rule
		}
		advance();
	}

	bool SelectorParser::isNextTokenDelim(const std::string& ch)
	{
		auto next = it_ + 1;
		if(next == end_) {
			return false;
		}
		return (*next)->id() == TokenId::DELIM && (*next)->value().as_string() == ch;
	}

	void SelectorParser::whitespace()
	{
		// S*
		while(isCurrentToken(TokenId::WHITESPACE)) {
			advance();
		}
	}
}
