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
#include "css_selector.hpp"
#include "css_lexer.hpp"
#include "unit_test.hpp"
#include "xhtml_doc.hpp"
#include "xhtml_element.hpp"
#include "xhtml_parser.hpp"

namespace css
{
	namespace 
	{
		class PseudoClassSelector : public FilterSelector
		{
		public:
			PseudoClassSelector(const std::string& name, const std::string& param) 
				: FilterSelector(FilterId::PSEUDO),
				  name_(name),
				  has_param_(!param.empty()),
				  param_(xhtml::ElementId::ANY)
			{
				if(!param.empty()) {
					param_ = xhtml::string_to_element_id(param);
				}
			}
			bool match(xhtml::ElementPtr element) const override
			{
				// XXX
				return false;
			}
			std::string toString() const override 
			{
				std::ostringstream ss;
				ss << ":" << name_;
				if(has_param_) {
					ss << "(" << xhtml::element_id_to_string(param_) << ")";
				}
				return ss.str();
			}
			const std::string& getName() const { return name_; }
			bool hasParameter() const { return has_param_; }
			xhtml::ElementId getParameter() const { return param_; }
			std::array<int,3> calculateSpecificity() override {
				std::array<int,3> specificity;
				for(int n = 0; n != 3; ++n) {
					specificity[n] = 0;
				}
				if(has_param_) {
					specificity[2] = 1;
				}
				if(name_ != "not") {
					specificity[1] = 1;
				}
				return specificity;
			}
		private:
			std::string name_;
			bool has_param_;
			xhtml::ElementId param_;
		};

		class ClassSelector : public FilterSelector
		{
		public:
			ClassSelector(const std::string& class_name) : FilterSelector(FilterId::CLASS), class_name_(class_name) {}
			bool match(xhtml::ElementPtr element) const override
			{
				// XXX
				return false;
			}
			std::string toString() const override 
			{
				return "." + class_name_;
			}
			std::array<int,3> calculateSpecificity() override {
				std::array<int,3> specificity;
				for(int n = 0; n != 3; ++n) {
					specificity[n] = 0;
				}
				specificity[1] = 1;
				return specificity;
			}
		private:
			std::string class_name_;
		};

		class IdSelector : public FilterSelector
		{
		public:
			IdSelector(const std::string& id) : FilterSelector(FilterId::ID), id_(id) {}
			bool match(xhtml::ElementPtr element) const override
			{
				// XXX
				return false;
			}
			std::string toString() const override 
			{
				return "#" + id_;
			}
			std::array<int,3> calculateSpecificity() override {
				std::array<int,3> specificity;
				for(int n = 0; n != 3; ++n) {
					specificity[n] = 0;
				}
				specificity[0] = 1;
				return specificity;
			}
		private:
			std::string id_;
		};

		enum class AttributeMatching {
			NONE,			// E[foo]			- an E element with "foo" attribute 
			PREFIX,			// E[foo^="bar"]	- an E element whose "foo" attribute value starts with "bar".
			SUFFIX,			// E[foo$="bar"]	- an E element whose "foo" attribute value ends with "bar".
			SUBSTRING,		// E[foo*="bar"]	- an E element whose "foo" attribute value contains with "bar".
			EXACT,			// E[foo="bar"]		- an E element whose "foo" attribute value exactly matches "bar".
			INCLUDE,		// E[foo~="bar"]	- an E element whose "foo" attribute value is a list of whitespace-separated values, one of which is exactly equal to "bar" 
			DASH,			// E[foo|="bar"]	- an E element whose "foo" attribute has a hyphen-separated list of values beginning (from the left) with "bar"
		};

		class AttributeSelector : public FilterSelector
		{
		public:
			AttributeSelector(const std::string& attr, AttributeMatching matching, const std::string& value)
				: FilterSelector(FilterId::ATTRIBUTE),
				  attr_(attr),
				  matching_(matching),
				  value_(value)
			{
			}
			bool match(xhtml::ElementPtr element) const override
			{
				// XXX
				return false;
			}
			std::string toString() const override 
			{
				std::ostringstream ss;
				ss << "[" << attr_;
				switch(matching_) {
					case AttributeMatching::NONE:		break;
					case AttributeMatching::PREFIX:		ss << "^="; break;
					case AttributeMatching::SUFFIX:		ss << "$="; break;
					case AttributeMatching::SUBSTRING:	ss << "*="; break;
					case AttributeMatching::EXACT:		ss << "="; break;
					case AttributeMatching::INCLUDE:	ss << "~="; break;
					case AttributeMatching::DASH:		ss << "|="; break;
					default: break;
				}
				ss << value_ << "]";
				return ss.str();
			}
			std::array<int,3> calculateSpecificity() override {
				std::array<int,3> specificity;
				for(int n = 0; n != 3; ++n) {
					specificity[n] = 0;
				}
				specificity[1] = 1;
				return specificity;
			}
		private:
			std::string attr_;
			AttributeMatching matching_;
			std::string value_;
		};

		class SelectorParser 
		{
		public:
			explicit SelectorParser(std::vector<TokenPtr>::const_iterator begin, std::vector<TokenPtr>::const_iterator end)
				: it_(begin),
				  end_(end)
			{
				selector_.emplace_back(std::make_shared<Selector>());
				parseSelector();
				while(true) {
					while(isToken(TokenId::WHITESPACE)) {
						advance();
					}
					if(isTokenDelimiter(",") || isToken(TokenId::COMMA)) {
						advance();
						while(isToken(TokenId::WHITESPACE)) {
							advance();
						}
						selector_.emplace_back(std::make_shared<Selector>());
						parseSelector();
					} else {
						return;
					}
				}
			}
			const std::vector<SelectorPtr>& getSelectors() { return selector_; }
		private:
			std::vector<SelectorPtr> selector_;
			std::vector<TokenPtr>::const_iterator it_;
			std::vector<TokenPtr>::const_iterator end_;

			void advance(int n = 1) {
				if(it_ == end_) {
					return;
				}
				it_ += n;
			}

			bool isToken(TokenId value) {
				if(it_ == end_) {
					return false;
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

			bool isTokenDelimiter(const std::string& ch) {
				return isToken(TokenId::DELIM) && (*it_)->getStringValue() == ch;
			}

			SimpleSelectorPtr parseSelector() {
				// simple_selector [ combinator selector | S+ [ combinator? selector ]? ]?
				
				auto simple_selector = parseSimpleSelector();
				selector_.back()->addSimpleSelector(simple_selector);
				while(true) {
					bool was_ws = false;
					while(isToken(TokenId::WHITESPACE)) {
						advance();
						was_ws = true;
					}
					if(isTokenDelimiter("+")) {
						advance();
						while(isToken(TokenId::WHITESPACE)) {
							advance();
						}
						auto next_selector = parseSelector();
						if(next_selector != nullptr) {
							simple_selector->setCombinator(Combinator::SIBLING);
							simple_selector = next_selector;
						}
					} else if(isTokenDelimiter(">")) {
						advance();
						while(isToken(TokenId::WHITESPACE)) {
							advance();
						}
						auto next_selector = parseSelector();
						if(next_selector != nullptr) {
							simple_selector->setCombinator(Combinator::CHILD);
							simple_selector = next_selector;
						}
					} else if(was_ws) {
						auto next_selector = parseSelector();
						if(next_selector != nullptr) {
							simple_selector->setCombinator(Combinator::DESCENDENT);
							simple_selector = next_selector;
						}
					} else {
						return simple_selector;
					}
				}
			}

			SimpleSelectorPtr parseSimpleSelector() {
				// element_name [ HASH | class | attrib | pseudo ]*
				// | [ HASH | class | attrib | pseudo ]+

				auto simple_selector = std::make_shared<SimpleSelector>();

				if(isToken(TokenId::IDENT)) {
					// element name
					simple_selector->setElementId(xhtml::string_to_element_id((*it_)->getStringValue()));
					advance();
				}

				while(true) {
					if(isToken(TokenId::HASH)) {
						simple_selector->addFilter(std::make_shared<IdSelector>((*it_)->getStringValue()));
						advance();
					} else if(isTokenDelimiter("#") && isNextToken(TokenId::IDENT)) {
						// HASH
						advance();
						simple_selector->addFilter(std::make_shared<IdSelector>((*it_)->getStringValue()));
						advance();
					} else if(isTokenDelimiter(".") && isNextToken(TokenId::IDENT)) {
						// class
						advance();
						simple_selector->addFilter(std::make_shared<ClassSelector>((*it_)->getStringValue()));
						advance();
					} else if(isTokenDelimiter("[") || isToken(TokenId::LBRACKET)) {
						//attrib
						// '[' S* IDENT S* [ [ '=' | INCLUDES | DASHMATCH ] S* [ IDENT | STRING ] S* ]? ']'
						advance();
						while(isToken(TokenId::WHITESPACE)) {
							advance();
						}
						if(!isToken(TokenId::IDENT)) {
							throw SelectorParseError("IDENT not matched in attribute token");
						}
						std::string attr = (*it_)->getStringValue();
						advance();

						while(isToken(TokenId::WHITESPACE)) {
							advance();
						}

						AttributeMatching match = AttributeMatching::NONE;
						std::string value;
						if(isToken(TokenId::INCLUDE_MATCH)) {
							match = AttributeMatching::INCLUDE;
						} else if(isTokenDelimiter("=")) {
							match = AttributeMatching::EXACT;
						} else if(isToken(TokenId::SUBSTRING_MATCH)) {
							match = AttributeMatching::SUBSTRING;
						} else if(isToken(TokenId::PREFIX_MATCH)) {
							match = AttributeMatching::PREFIX;
						} else if(isToken(TokenId::SUFFIX_MATCH)) {
							match = AttributeMatching::SUFFIX;
						} else if(isToken(TokenId::DASH_MATCH)) {
							match = AttributeMatching::DASH;
						}
						if(match != AttributeMatching::NONE) {
							advance();
							while(isToken(TokenId::WHITESPACE)) {
								advance();
							}
							if(!isToken(TokenId::IDENT) && !isToken(TokenId::STRING)) {
								throw SelectorParseError("IDENT not matched in attribute token");
							}
							value = (*it_)->getStringValue();
							advance();
						}
						while(isToken(TokenId::WHITESPACE)) {
							advance();
						}
						if(!isTokenDelimiter("]") && !isToken(TokenId::RBRACKET)) {
							throw SelectorParseError("] not matched in attribute token");
						}
						advance();
						simple_selector->addFilter(std::make_shared<AttributeSelector>(attr, match, value));
					} else if(isTokenDelimiter(":") || isToken(TokenId::COLON)) {
						// pseudo
						// ':' [ IDENT | FUNCTION S* [IDENT S*]? ')' ] -- CSS2 (CSS3 is more complex with an+b stuff)
						advance();
						std::string name;
						std::string param;
						if(isToken(TokenId::IDENT)) {
							name = (*it_)->getStringValue();
							advance();
						} else if(isToken(TokenId::FUNCTION)) {
							name = (*it_)->getStringValue();
							advance();
							while(isToken(TokenId::WHITESPACE)) {
								advance();
							}
							if(isToken(TokenId::IDENT)) {
								param = (*it_)->getStringValue();
								advance();
								while(isToken(TokenId::WHITESPACE)) {
									advance();
								}
							}
							if(!isToken(TokenId::RPAREN)) {
								throw SelectorParseError(") not matched in pseudo class");
							}
							advance();
						} else {
							throw SelectorParseError("Expected IDENT or FUNCTION while parsing pseudo-class");
						}
						simple_selector->addFilter(std::make_shared<PseudoClassSelector>(name, param));
					} else {
						return simple_selector;
					}					 
				}
			}
		};
	}

	Selector::Selector()
		: selector_chain_()
	{
		specificity_[0] = specificity_[1] = specificity_[2] = 0;
	}

	std::vector<SelectorPtr> Selector::factory(const std::vector<TokenPtr>& tokens)
	{
		SelectorParser parser(tokens.begin(), tokens.end());

		for(auto& selector : parser.getSelectors()) {
			selector->calculateSpecificity();
		}

		return parser.getSelectors();
	}

	bool Selector::match(xhtml::ElementPtr element) const
	{
		for(auto& simple : selector_chain_) {
			if(simple->match(element)) {
			}
		}
		return false;
	}
	
	void Selector::calculateSpecificity()
	{		
		for(auto& s : selector_chain_) {
			auto specs = s->getSpecificity();

			for(int n = 0; n != 3; ++n) {
				specificity_[n] += specs[n];
			}
		}
	}

	std::string Selector::toString() const
	{
		std::ostringstream ss;
		for(auto& selector : selector_chain_) {
			ss << selector->toString();
		}
		ss << " specificity(" << specificity_[0] << "," << specificity_[1] << "," << specificity_[2] << ")";
		return ss.str();
	}

	SimpleSelector::SimpleSelector()
		: element_(xhtml::ElementId::ANY),
		  filters_(),
		  combinator_(Combinator::NONE)
	{
		for(int n = 0; n != 3; ++n) {
			specificity_[n] = 0;
		}
	}

	void SimpleSelector::addFilter(FilterSelectorPtr f) 
	{
		auto s = f->calculateSpecificity();
		for(int n = 0; n != 3; ++n) {
			specificity_[n] += s[n];
		}
		filters_.emplace_back(f); 
	}

	std::string SimpleSelector::toString() const
	{
		std::ostringstream ss;
		ss << xhtml::element_id_to_string(element_);
		for(auto& f : filters_) {
			ss << f->toString();
		}
		switch(combinator_) {
			case Combinator::CHILD:			ss << " > "; break;
			case Combinator::DESCENDENT:	ss << " ";	 break;
			case Combinator::SIBLING:		ss << " + "; break;
			case Combinator::NONE: // fallthrough
			default: break;
		}
		return ss.str();
	}

	bool SimpleSelector::match(xhtml::ElementPtr element) const
	{
		return false;
	}

	void SimpleSelector::setElementId(xhtml::ElementId id) 
	{ 
		element_ = id; 
		specificity_[2] = 1;
	}

	FilterSelector::FilterSelector(FilterId id)
		: id_(id)
	{
	}
}

bool check_selector(const std::string& selector, const std::string& string_to_match)
{
	css::Tokenizer tokens(selector);
	auto selectors = css::Selector::factory(tokens.getTokens());
	for(auto& s : selectors) {
		LOG_DEBUG(s->toString());
	}

	xhtml::DocPtr doc = xhtml::Parser::parseFromString(string_to_match);
	bool failed_match = false;
	for(auto& s : selectors) {
		doc->getRootElement()->preOrderTraverse([s](xhtml::ElementPtr e){ 
			if(s->match(e)) {
				LOG_DEBUG("Matched element was: " << e->getName());
			}
		});
	}
	// XXX actual tests should go here.
	//xhtml::Element::factory(xhtml::parse(string_to_match))
	return true;
}

UNIT_TEST(css_selectors)
{
	CHECK_EQ(check_selector("*", "<em><p>Some text</p></em>"), true);
	CHECK_EQ(check_selector("em", "<em><p>Some text</p></em>"), true);
	CHECK_EQ(check_selector("p#xx123", "<em><p id=\"xx123\">Some text</p></em>"), true);
	CHECK_EQ(check_selector("em > p", "<em><p>Some text</p></em>"), true);
	CHECK_EQ(check_selector("em + p", "<body><em><p>Some text</p></em><p>A new paragraph.</p></body>"), true);
	CHECK_EQ(check_selector("em p", "<em><b><p>Some text</p></b></em>"), true);
	CHECK_EQ(check_selector("em[foo]", "<em foo=\"xxx\"><p>Some text</p></em>"), true);
	CHECK_EQ(check_selector("em[foo=\"xxx\"]", "<em foo=\"xxx\"><p>Some text</p></em>"), true);
	CHECK_EQ(check_selector("em[foo^=x]", "<em foo=\"xxx\"><p>Some text</p></em>"), true);
	CHECK_EQ(check_selector("em[foo^=x][bar=x]", "<em foo=\"xxx\" bar=\"x\"><p>Some text</p></em>"), true);
	CHECK_EQ(check_selector("h1, h2, h3", "<h1>Now is the time.</h1>"), true);
	CHECK_EQ(check_selector("#s12:not(FOO)", "<html><head></head><body><h1 id=\"s12\">Now is the time.</h1><FOO id=\"s12\"></FOO></body></html>"), true);

	// XXX actual tests should go here.
	//xhtml::Element::factory(xhtml::parse("<em><p>Some text</p></em>"))
}
