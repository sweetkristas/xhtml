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
#include "css_properties.hpp"
#include "unit_test.hpp"

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

		class DeclarationParser
		{
		public:
			DeclarationParser(std::vector<TokenPtr>::const_iterator begin, std::vector<TokenPtr>::const_iterator end) 
				: it_(begin),
				  end_(end),
				  pp_()
			{
				while(isToken(TokenId::WHITESPACE)) {
					advance();
				}
				if(isToken(TokenId::IDENT)) {
					parseDeclarationList(&pp_);
				} else if(isToken(TokenId::BLOCK_TOKEN)) {
					auto old_it = it_;
					auto old_end = end_;
					it_ = (*old_it)->getParameters().begin();
					end_ = (*old_it)->getParameters().end();
					parseDeclarationList(&pp_);
					it_ = old_it;
					end_ = old_end;
					advance();
				} else if(isToken(TokenId::LBRACE)) {
					advance();
					parseDeclarationList(&pp_);
				} else if(isToken(TokenId::EOF_TOKEN)) {
					throw ParserError("expected block declaration");
				}

			}
			static PropertyList parseTokens(const std::vector<TokenPtr>& tokens) {
				DeclarationParser p(tokens.begin(), tokens.end());
				return p.getProperties();
			}
			PropertyList getProperties() { return pp_.getPropertyList(); }
		private:
			PropertyParser pp_;
			void advance(int n = 1) {
				if(it_ == end_) {
					return;
				}
				it_ += n;
			}

			bool isToken(TokenId value) {
				if(it_ == end_ ) {
					return value == TokenId::EOF_TOKEN ? true : false;
				}
				return (*it_)->id() == value;
			}
			
			bool isNextToken(TokenId value) {
				auto next = it_+1;
				if(next == end_) {
					return false;
				}
				return (*next)->id() == value;
			}

			void parseDeclarationList(PropertyParser* pp) {
				while(true) {
					while(isToken(TokenId::WHITESPACE)) {
						advance();
					}
					if(isToken(TokenId::RBRACE)) {
						advance();
						return;
					}
					if(isToken(TokenId::EOF_TOKEN) || it_ == end_) {
						return;
					}
					try {
						parseDeclaration(pp);
					} catch (ParserError& e) {
						LOG_ERROR("Dropping declaration: " << e.what());
						while(!isToken(TokenId::SEMICOLON) && !isToken(TokenId::RBRACE) && !isToken(TokenId::EOF_TOKEN)) {
							advance();
						}
					}
					while(isToken(TokenId::WHITESPACE)) {
						advance();
					}
					if(isToken(TokenId::SEMICOLON)) {
						advance();
					} else if(!isToken(TokenId::RBRACE) && !isToken(TokenId::EOF_TOKEN)) {
						throw ParserError("Expected semicolon.");
					}
				}
			}

			void parseDeclaration(PropertyParser* pp) {
				// assume first token is ident
				std::string property = (*it_)->getStringValue();
				advance();
				while(isToken(TokenId::WHITESPACE)) {
					advance();
				}
				if(!isToken(TokenId::COLON)) {
					throw ParserError("Expected ':' in declaration");
				}
				advance();
			
				while(isToken(TokenId::WHITESPACE)) {
					advance();
				}
				
				it_ = pp->parse(property, it_, end_);
				while(isToken(TokenId::WHITESPACE)) {
					advance();
				}
				if(isTokenDelimiter("!")) {
					advance();
					while(isToken(TokenId::WHITESPACE)) {
						advance();
					}
					if(isToken(TokenId::IDENT)) {
						const std::string ref = (*it_)->getStringValue();
						advance();
						if(ref == "important") {
							// add important tag to the rule in plist.
							for(auto& pl : pp->getPropertyList()) {
								pl.second.style->setImportant(true);
							}
						}
					}
				}
			}

			bool isTokenDelimiter(const std::string& ch) {
				return isToken(TokenId::DELIM) && (*it_)->getStringValue() == ch;
			}
			std::vector<TokenPtr>::const_iterator it_;
			std::vector<TokenPtr>::const_iterator end_;
		};

	}

	Parser::Parser(StyleSheetPtr ss, const std::vector<TokenPtr>& tokens)
		: style_sheet_(ss),
		  token_(tokens.begin()),
		  end_(tokens.end())
	{
		for(auto& token : pasrseRuleList(0)) {
			try {
				parseRule(token);
			} catch(ParserError& e) {
				LOG_DEBUG("Dropping rule: " << e.what() << " " << (token != nullptr ? token->toString() : ""));
			}
		}
	}

	void Parser::parse(StyleSheetPtr ss, const std::string& str)
	{
		css::Tokenizer tokens(str);
		Parser p(ss, tokens.getTokens());
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

	PropertyList Parser::parseDeclarationList(const std::string& str)
	{
		css::Tokenizer tokens(str);
		return DeclarationParser::parseTokens(tokens.getTokens());		
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
		if(rule == nullptr) {
			throw ParserError("Trying to parse empty rule.");
		}
		/*std::ostringstream ss;
		ss << "RULE. prelude:";
		for(auto& r : rule->getParameters()) {
			ss << " " << r->toString();
		}
		ss << "; values: " << rule->getValue()->toString();
		LOG_DEBUG(ss.str());*/

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
			CssRulePtr css_rule = std::make_shared<CssRule>();
			css_rule->selectors = Selector::parseTokens(rule->getParameters());
			css_rule->declaractions = DeclarationParser::parseTokens(rule->getValue()->getParameters());
			style_sheet_->addRule(css_rule);
		}
	}
}

UNIT_TEST(css_declarations)
{
	//css::Tokenizer tokens("color: rgb(100%,0,0);");
	//css::Tokenizer tokens("{ color: #ff0 !important; font-family: 'Arial'; color: hsl(360,0,0) }");
	//for(auto& tok : tokens.getTokens()) {
	//	LOG_DEBUG("  TOKEN: " << tok->toString());
	//}
	//css::DeclarationParser::parseTokens(tokens.getTokens());

	//css::Tokenizer tokens("margin-left: 1.15em;");
	//for(auto& tok : tokens.getTokens()) {
	//	LOG_DEBUG("  TOKEN: " << tok->toString());
	//}
	//css::DeclarationParser::parseTokens(tokens.getTokens());
}
