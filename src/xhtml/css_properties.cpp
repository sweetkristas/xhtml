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
#include "formatter.hpp"
#include "css_parser.hpp"
#include "css_properties.hpp"

namespace css
{
	namespace 
	{
		const double border_width_thin = 2.0;
		const double border_width_medium = 4.0;
		const double border_width_thick = 10.0;

		typedef std::map<std::string, std::function<void(PropertyParser*, const std::string&)>> property_map;
		property_map& get_property_table()
		{
			static property_map res;
			return res;
		}

		typedef std::map<std::string, Object> default_value_map;
		default_value_map& get_default_value_table()
		{
			static default_value_map res;
			return res;
		}

		KRE::Color hsla_to_color(double h, double s, double l, double a)
		{
			const float hue_upper_limit = 360.0;
			double c = 0.0, m = 0.0, x = 0.0;
			c = (1.0 - std::abs(2 * l - 1.0)) * s;
			m = 1.0 * (l - 0.5 * c);
			x = c * (1.0 - std::abs(std::fmod(h / 60.0, 2) - 1.0));
			if (h >= 0.0 && h < (hue_upper_limit / 6.0)) {
				return KRE::Color(static_cast<float>(c+m), static_cast<float>(x+m), static_cast<float>(m), static_cast<float>(a));
			} else if (h >= (hue_upper_limit / 6.0) && h < (hue_upper_limit / 3.0)) {
				return KRE::Color(static_cast<float>(x+m), static_cast<float>(c+m), static_cast<float>(m), static_cast<float>(a));
			} else if (h < (hue_upper_limit / 3.0) && h < (hue_upper_limit / 2.0)) {
				return KRE::Color(static_cast<float>(m), static_cast<float>(c+m), static_cast<float>(x+m), static_cast<float>(a));
			} else if (h >= (hue_upper_limit / 2.0) && h < (2.0f * hue_upper_limit / 3.0)) {
				return KRE::Color(static_cast<float>(m), static_cast<float>(x+m), static_cast<float>(c+m), static_cast<float>(a));
			} else if (h >= (2.0 * hue_upper_limit / 3.0) && h < (5.0 * hue_upper_limit / 6.0)) {
				return KRE::Color(static_cast<float>(x+m), static_cast<float>(m), static_cast<float>(c+m), static_cast<float>(a));
			} else if (h >= (5.0 * hue_upper_limit / 6.0) && h < hue_upper_limit) {
				return KRE::Color(static_cast<float>(c+m), static_cast<float>(m), static_cast<float>(x+m), static_cast<float>(a));
			}
			return KRE::Color(static_cast<float>(m), static_cast<float>(m), static_cast<float>(m), static_cast<float>(a));
		}

		struct PropertyRegistrar
		{
			PropertyRegistrar(const std::string& name, std::function<void(PropertyParser*, const std::string&)> fn) {
				get_property_table()[name] = fn;
			}
			PropertyRegistrar(const std::string& name, Object def, std::function<void(PropertyParser*, const std::string&)> fn) {
				get_property_table()[name] = fn;
				get_default_value_table()[name] = def;
			}
		};

		using namespace std::placeholders;
		
		PropertyRegistrar property000("background-color", Object(CssColor(ColorParam::TRANSPARENT)), std::bind(&PropertyParser::parseColor, _1, _2));
		PropertyRegistrar property001("color", Object(CssColor(ColorParam::VALUE)), std::bind(&PropertyParser::parseColor, _1, _2));
		PropertyRegistrar property002("padding-left", Object(CssLength(0)), std::bind(&PropertyParser::parseWidth, _1, _2));
		PropertyRegistrar property003("padding-right", Object(CssLength(0)), std::bind(&PropertyParser::parseWidth, _1, _2));
		PropertyRegistrar property004("padding-top", Object(CssLength(0)), std::bind(&PropertyParser::parseWidth, _1, _2));
		PropertyRegistrar property005("padding-bottom", Object(CssLength(0)), std::bind(&PropertyParser::parseWidth, _1, _2));
		PropertyRegistrar property006("padding", std::bind(&PropertyParser::parseWidthList, _1, _2));
		PropertyRegistrar property007("margin-left", Object(CssLength(0)), std::bind(&PropertyParser::parseWidth, _1, _2));
		PropertyRegistrar property008("margin-right", Object(CssLength(0)), std::bind(&PropertyParser::parseWidth, _1, _2));
		PropertyRegistrar property009("margin-top", Object(CssLength(0)), std::bind(&PropertyParser::parseWidth, _1, _2));
		PropertyRegistrar property010("margin-bottom", Object(CssLength(0)), std::bind(&PropertyParser::parseWidth, _1, _2));
		PropertyRegistrar property011("margin", std::bind(&PropertyParser::parseWidthList, _1, _2));
		PropertyRegistrar property012("border-top-color", Object(CssColor(ColorParam::VALUE)), std::bind(&PropertyParser::parseColor, _1, _2));
		PropertyRegistrar property013("border-left-color", Object(CssColor(ColorParam::VALUE)), std::bind(&PropertyParser::parseColor, _1, _2));
		PropertyRegistrar property014("border-bottom-color", Object(CssColor(ColorParam::VALUE)), std::bind(&PropertyParser::parseColor, _1, _2));
		PropertyRegistrar property015("border-right-color", Object(CssColor(ColorParam::VALUE)), std::bind(&PropertyParser::parseColor, _1, _2));
		//PropertyRegistrar property016("border-color", std::bind(&PropertyParser::parseColorList, _1, _2));
		PropertyRegistrar property017("border-top-width", Object(CssLength(border_width_medium)), std::bind(&PropertyParser::parseBorderWidth, _1, _2));
		PropertyRegistrar property018("border-left-width", Object(CssLength(border_width_medium)), std::bind(&PropertyParser::parseBorderWidth, _1, _2));
		PropertyRegistrar property019("border-bottom-width", Object(CssLength(border_width_medium)), std::bind(&PropertyParser::parseBorderWidth, _1, _2));
		PropertyRegistrar property020("border-right-width", Object(CssLength(border_width_medium)), std::bind(&PropertyParser::parseBorderWidth, _1, _2));
		//PropertyRegistrar property021("border-width", std::bind(&PropertyParser::parseBorderWidthList, _1, _2));
		PropertyRegistrar property022("border-top-style", Object(BorderStyle::NONE), std::bind(&PropertyParser::parseBorderStyle, _1, _2));
		PropertyRegistrar property023("border-left-style", Object(BorderStyle::NONE), std::bind(&PropertyParser::parseBorderStyle, _1, _2));
		PropertyRegistrar property024("border-bottom-style", Object(BorderStyle::NONE), std::bind(&PropertyParser::parseBorderStyle, _1, _2));
		PropertyRegistrar property025("border-right-style", Object(BorderStyle::NONE), std::bind(&PropertyParser::parseBorderStyle, _1, _2));
		//PropertyRegistrar property026("border-style", std::bind(&PropertyParser::parseBorderStyleList, _1, _2));
		//PropertyRegistrar property027("border", std::bind(&PropertyParser::parseBorderList, _1, _2));
		PropertyRegistrar property026("display", Object(CssDisplay::INLINE), std::bind(&PropertyParser::parseDisplay, _1, _2));	
		PropertyRegistrar property027("width", Object(CssLength(CssLengthParam::AUTO)), std::bind(&PropertyParser::parseWidth, _1, _2));	
		PropertyRegistrar property028("height", Object(CssLength(CssLengthParam::AUTO)), std::bind(&PropertyParser::parseWidth, _1, _2));
		PropertyRegistrar property029("whitespace", Object(CssWhitespace::NORMAL), std::bind(&PropertyParser::parseWhitespace, _1, _2));	
		PropertyRegistrar property030("font-family", Object(std::vector<std::string>()), std::bind(&PropertyParser::parseFontFamily, _1, _2));	
	}

	PropertyList::PropertyList()
		: properties_()
	{
	}

	void PropertyList::addProperty(const std::string& name, const Object& o)
	{
		auto it = properties_.find(name);
		if(it == properties_.end()) {
			// unconditionally add new properties
			properties_[name] = o;
		} else {
			// check for important flag before merging.
			if((it->second.isImportant() && o.isImportant()) || !it->second.isImportant()) {
				it->second = o;
			}
		}
	}

	Object PropertyList::getProperty(const std::string& name) const
	{
		auto it = properties_.find(name);
		if(it == properties_.end()) {
			auto def = get_default_value_table().find(name);
			if(def == get_default_value_table().end()) {
				return Object();
			}
			return def->second;
		}
		return it->second;
	}

	void PropertyList::merge(const PropertyList& plist)
	{
		for(auto& p : plist.properties_) {
			addProperty(p.first, p.second);
		}
	}

	PropertyParser::PropertyParser()
		: it_(),
		  end_(),
		  plist_()
	{
	}

	PropertyParser::const_iterator PropertyParser::parse(const std::string& name, const const_iterator& begin, const const_iterator& end)
	{
		it_ = begin;
		end_ = end;

		auto it = get_property_table().find(name);
		if(it == get_property_table().end()) {
			throw ParserError(formatter() << "Unable to find a parse function for property '" << name << "'");
		}
		it->second(this, name);

		return it_;
	}

	void PropertyParser::advance()
	{
		if(it_ != end_) {
			++it_;
		}
	}

	void PropertyParser::skipWhitespace()
	{
		while(isToken(TokenId::WHITESPACE)) {
			advance();
		}
	}

	bool PropertyParser::isToken(TokenId tok) const
	{
		if(it_ == end_) {
			return tok == TokenId::EOF_TOKEN ? true : false;
		}
		return (*it_)->id() == tok;
	}

	bool PropertyParser::isTokenDelimiter(const std::string& delim)
	{
		return isToken(TokenId::DELIM) && (*it_)->getStringValue() == delim;
	}

	std::vector<TokenPtr> PropertyParser::parseCSVList(TokenId end_token)
	{
		std::vector<TokenPtr> res;
		while(!isToken(TokenId::EOF_TOKEN) && !isToken(end_token) && !isToken(TokenId::SEMICOLON)) {
			skipWhitespace();
			res.emplace_back(*it_);
			advance();
			skipWhitespace();
			if(isToken(TokenId::COMMA)) {
				advance();
			} else if(!isToken(end_token) && !isToken(TokenId::EOF_TOKEN) && !isToken(TokenId::SEMICOLON)) {
				throw ParserError("Expected ',' (COMMA) while parsing color value.");
			}
		}
		if(isToken(end_token)) {
			advance();
		}
		return res;
	}

	void PropertyParser::parseCSVNumberList(TokenId end_token, std::function<void(int,double,bool)> fn)
	{
		auto toks = parseCSVList(end_token);
		int n = 0;
		for(auto& t : toks) {
			if(t->id() == TokenId::PERCENT) {
				fn(n, t->getNumericValue(), true);
			} else if(t->id() == TokenId::NUMBER) {
				fn(n, t->getNumericValue(), false);
			} else {
				throw ParserError("Expected percent or numeric value while parsing numeric list.");
			}
			++n;
		}
	}

	void PropertyParser::parseCSVStringList(TokenId end_token, std::function<void(int, const std::string&)> fn)
	{
		auto toks = parseCSVList(end_token);
		int n = 0;
		for(auto& t : toks) {
			if(t->id() == TokenId::IDENT) {
				fn(n, t->getStringValue());
			} else if(t->id() == TokenId::STRING) {
				fn(n, t->getStringValue());
			} else {
				throw ParserError("Expected ident or string value while parsing string list.");
			}
			++n;
		}
	}

	Object PropertyParser::parseColorInternal()
	{
		CssColor color;
		if(isToken(TokenId::IDENT)) {
			const std::string& ref = (*it_)->getStringValue();
			advance();
			if(ref == "transparent") {
				color.setParam(ColorParam::TRANSPARENT);
			} else if(ref == "inherit") {
				return Object(true);
			} else {
				// color value is in ref.
				color.setColor(KRE::Color(ref));
			}
		} else if(isToken(TokenId::FUNCTION)) {
			const std::string& ref = (*it_)->getStringValue();
			advance();
			if(ref == "rgb") {
				int values[3] = { 255, 255, 255 };
				parseCSVNumberList(TokenId::RPAREN, [&values](int n, double value, bool is_percent) {
					if(n < 3) {
						if(is_percent) {
							value *= 255.0 / 100.0;
						}
						values[n] = std::min(255, std::max(0, static_cast<int>(value)));
					}
				});
				color.setColor(KRE::Color(values[0], values[1], values[2]));
			} else if(ref == "rgba") {
				int values[4] = { 255, 255, 255, 255 };
				parseCSVNumberList(TokenId::RPAREN, [&values](int n, double value, bool is_percent) {
					if(n < 4) {
						if(is_percent) {
							value *= 255.0 / 100.0;
						}
						values[n] = std::min(255, std::max(0, static_cast<int>(value)));
					}
				});
				color.setColor(KRE::Color(values[0], values[1], values[2], values[3]));
			} else if(ref == "hsl") {
				double values[3];
				const double multipliers[3] = { 360.0, 1.0, 1.0 };
				parseCSVNumberList(TokenId::RPAREN, [&values, &multipliers](int n, double value, bool is_percent) {
					if(n < 3) {
						if(is_percent) {
							value *= multipliers[n] / 100.0;
						}
						values[n] = value;
					}
				});					
				color.setColor(hsla_to_color(values[0], values[1], values[2], 1.0));
			} else if(ref == "hsla") {
				double values[4];
				const double multipliers[4] = { 360.0, 1.0, 1.0, 1.0 };
				parseCSVNumberList(TokenId::RPAREN, [&values, &multipliers](int n, double value, bool is_percent) {
					if(n < 4) {
						if(is_percent) {
							value *= multipliers[n] / 100.0;
						}
						values[n] = value;
					}
				});					
				color.setColor(hsla_to_color(values[0], values[1], values[2], values[3]));
			}
		} else if(isToken(TokenId::HASH)) {
			const std::string& ref = (*it_)->getStringValue();
			// is #fff or #ff00ff representation
			color.setColor(KRE::Color(ref));
			advance();
		}
		return Object(color);
	}

	Object PropertyParser::parseWidthInternal()
	{
		skipWhitespace();
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			if(ref == "inherit") {
				advance();
				return Object(true);
			} else if(ref == "auto") {
				advance();
				return Object(CssLength(CssLengthParam::AUTO));
			}
		} else if(isToken(TokenId::DIMENSION)) {
			const std::string units = (*it_)->getStringValue();
			double value = (*it_)->getNumericValue();
			advance();
			return Object(CssLength(value, units));
		} else if(isToken(TokenId::PERCENT)) {
			double d = (*it_)->getNumericValue();
			advance();
			return Object(CssLength(d, true));
		} else if(isToken(TokenId::NUMBER)) {
			double d = (*it_)->getNumericValue();
			advance();
			return Object(CssLength(d));
		}
		return Object();
	}

	Object PropertyParser::parseBorderWidthInternal()
	{
		skipWhitespace();
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			if(ref == "thin") {
				advance();
				return Object(CssLength(border_width_thin));
			} else if(ref == "medium") {
				advance();
				return Object(CssLength(border_width_medium));
			} else if(ref == "thick") {
				advance();
				return Object(CssLength(border_width_thick));
			}
		}	
		return parseWidthInternal();
	}

	void PropertyParser::parseColor(const std::string& name)
	{
		plist_.addProperty(name, parseColorInternal());
	}

	void PropertyParser::parseWidth(const std::string& name)
	{
		plist_.addProperty(name, parseWidthInternal());
	}

	void PropertyParser::parseWidthList(const std::string& name)
	{
		Object w1 = parseWidthInternal();
		skipWhitespace();
		if(isToken(TokenId::EOF_TOKEN) || isToken(TokenId::RBRACE) || isToken(TokenId::SEMICOLON) || isTokenDelimiter("!")) {
			plist_.addProperty(name + "-top", w1);
			plist_.addProperty(name + "-bottom", w1);
			plist_.addProperty(name + "-right", w1);
			plist_.addProperty(name + "-left", w1);
			return;
		}
		Object w2 = parseWidthInternal();
		skipWhitespace();
		if(isToken(TokenId::EOF_TOKEN) || isToken(TokenId::RBRACE) || isToken(TokenId::SEMICOLON) || isTokenDelimiter("!")) {
			plist_.addProperty(name + "-top", w1);
			plist_.addProperty(name + "-bottom", w1);
			plist_.addProperty(name + "-right", w2);
			plist_.addProperty(name + "-left", w2);
			return;
		}
		Object w3 = parseWidthInternal();
		skipWhitespace();
		if(isToken(TokenId::EOF_TOKEN) || isToken(TokenId::RBRACE) || isToken(TokenId::SEMICOLON) || isTokenDelimiter("!")) {
			plist_.addProperty(name + "-top", w1);
			plist_.addProperty(name + "-right", w2);
			plist_.addProperty(name + "-left", w2);
			plist_.addProperty(name + "-bottom", w3);
			return;
		}
		Object w4 = parseWidthInternal();
		skipWhitespace();

		// four values, apply to individual elements.
		plist_.addProperty(name + "-top", w1);
		plist_.addProperty(name + "-right", w2);
		plist_.addProperty(name + "-bottom", w3);
		plist_.addProperty(name + "-left", w4);
	}

	void PropertyParser::parseBorderWidth(const std::string& name)
	{
		plist_.addProperty(name, parseBorderWidthInternal());
	}

	Object PropertyParser::parseBorderStyleInternal()
	{
		BorderStyle bs = BorderStyle::NONE;
		skipWhitespace();
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			skipWhitespace();
			if(ref == "none") {
				bs = BorderStyle::NONE;
			} else if(ref == "inherit") {
				return Object(true);
			} else if(ref == "hidden") {
				bs = BorderStyle::HIDDEN;
			} else if(ref == "dotted") {
				bs = BorderStyle::DOTTED;
			} else if(ref == "dashed") {
				bs = BorderStyle::DASHED;
			} else if(ref == "solid") {
				bs = BorderStyle::SOLID;
			} else if(ref == "double") {
				bs = BorderStyle::DOUBLE;
			} else if(ref == "groove") {
				bs = BorderStyle::GROOVE;
			} else if(ref == "ridge") {
				bs = BorderStyle::RIDGE;
			} else if(ref == "inset") {
				bs = BorderStyle::INSET;
			} else if(ref == "outset") {
				bs = BorderStyle::OUTSET;
			} else {
				throw ParserError(formatter() << "Expected identifier '" << ref << "' while parsing border style");
			}
			return Object(bs);
		}
		throw ParserError(formatter() << "Expected IDENTIFIER, found: " << (*it_)->toString());
		return Object(bs);
	}

	void PropertyParser::parseBorderStyle(const std::string& name)
	{
		plist_.addProperty(name, parseBorderStyleInternal());
	}

	void PropertyParser::parseDisplay(const std::string& name)
	{
		CssDisplay display = CssDisplay::INLINE;
		skipWhitespace();
		if(isToken(TokenId::IDENT)) {
			std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inline") {
				display = CssDisplay::INLINE;
			} else if(ref == "none") {
				display = CssDisplay::NONE;
			} else if(ref == "block") {
				display = CssDisplay::BLOCK;
			} else if(ref == "list-item") {
				display = CssDisplay::LIST_ITEM;
			} else if(ref == "inline-block") {
				display = CssDisplay::INLINE_BLOCK;
			} else if(ref == "table") {
				display = CssDisplay::TABLE;
			} else if(ref == "inline-table") {
				display = CssDisplay::INLINE_TABLE;
			} else if(ref == "table-row-group") {
				display = CssDisplay::TABLE_ROW_GROUP;
			} else if(ref == "table-header-group") {
				display = CssDisplay::TABLE_HEADER_GROUP;
			} else if(ref == "table-footer-group") {
				display = CssDisplay::TABLE_FOOTER_GROUP;
			} else if(ref == "table-row") {
				display = CssDisplay::TABLE_ROW;
			} else if(ref == "table-column-group") {
				display = CssDisplay::TABLE_COLUMN_GROUP;
			} else if(ref == "table-column") {
				display = CssDisplay::TABLE_COLUMN;
			} else if(ref == "table-cell") {
				display = CssDisplay::TABLE_CELL;
			} else if(ref == "table-caption") {
				display = CssDisplay::TABLE_CAPTION;
			} else if(ref == "inherit") {
				plist_.addProperty(name, Object(true));
				return;
			} else {
				throw ParserError(formatter() << "Unrecognised token for display property: " << ref);
			}
		}
		plist_.addProperty(name, Object(display));
	}

	void PropertyParser::parseWhitespace(const std::string& name)
	{
		CssWhitespace ws = CssWhitespace::NORMAL;
		if(isToken(TokenId::IDENT)) {
			std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "normal") {
				ws = CssWhitespace::NORMAL;
			} else if(ref == "pre") {
				ws = CssWhitespace::PRE;
			} else if(ref == "nowrap") {
				ws = CssWhitespace::NOWRAP;
			} else if(ref == "pre-wrap") {
				ws = CssWhitespace::PRE_WRAP;
			} else if(ref == "pre-line") {
				ws = CssWhitespace::PRE_LINE;
			} else {
				throw ParserError(formatter() << "Unrecognised token for display property: " << ref);
			}			
		} else {
			throw ParserError(formatter() << "Expected identifier for property: " << name << " found " << Token::tokenIdToString((*it_)->id()));
		}
		plist_.addProperty(name, Object(ws));
	}

	void PropertyParser::parseFontFamily(const std::string& name)
	{
		std::vector<std::string> font_list;
		parseCSVStringList(TokenId::DELIM, [&font_list](int n, const std::string& str) {
			font_list.emplace_back(str);
		});
		plist_.addProperty(name, Object(font_list));
	}
}
