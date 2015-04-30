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

		typedef std::function<void(PropertyParser*, const std::string&)> ParserFunction;
		struct PropertyNameInfo
		{
			PropertyNameInfo() : value(Property::MAX_PROPERTIES), fn() {}
			PropertyNameInfo(Property p, ParserFunction f) : value(p), fn(f) {}
			Property value;
			ParserFunction fn;
		};

		typedef std::map<std::string, PropertyNameInfo> property_map;
		property_map& get_property_table()
		{
			static property_map res;
			return res;
		}

		typedef std::vector<PropertyInfo> property_info_list;
		property_info_list& get_property_info_table()
		{
			static property_info_list res;
			if(res.empty()) {
				res.resize(static_cast<int>(Property::MAX_PROPERTIES));
			}
			return res;
		}

		std::vector<std::string>& get_default_fonts()
		{
			static std::vector<std::string> res;
			if(res.empty()) {
				res.emplace_back("arial.ttf");
				res.emplace_back("FreeSerif.ttf");
			}
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
				get_property_table()[name] = PropertyNameInfo(Property::MAX_PROPERTIES, fn);
			}
			PropertyRegistrar(const std::string& name, Property p, bool inherited, Object def, std::function<void(PropertyParser*, const std::string&)> fn) {
				get_property_table()[name] = PropertyNameInfo(p, fn);
				ASSERT_LOG(static_cast<int>(p) < static_cast<int>(get_property_info_table().size()),
					"Something went wrong. Tried to add a property outside of the maximum range of our property list table.");
				get_property_info_table()[static_cast<int>(p)].name = name;
				get_property_info_table()[static_cast<int>(p)].inherited = inherited;
				get_property_info_table()[static_cast<int>(p)].obj = def;
				get_property_info_table()[static_cast<int>(p)].is_default = true;
			}
		};

		using namespace std::placeholders;
		
		PropertyRegistrar property000("background-color", Property::BACKGROUND_COLOR, false, Object(KRE::Color(0,0,0,0)), std::bind(&PropertyParser::parseColor, _1, _2));
		PropertyRegistrar property001("color", Property::COLOR, true, Object(KRE::Color::colorWhite()), std::bind(&PropertyParser::parseColor, _1, _2));
		PropertyRegistrar property002("padding-left", Property::PADDING_LEFT, false, Object(Length(0)), std::bind(&PropertyParser::parseWidth, _1, _2));
		PropertyRegistrar property003("padding-right", Property::PADDING_RIGHT, false, Object(Length(0)), std::bind(&PropertyParser::parseWidth, _1, _2));
		PropertyRegistrar property004("padding-top", Property::PADDING_TOP, false, Object(Length(0)), std::bind(&PropertyParser::parseWidth, _1, _2));
		PropertyRegistrar property005("padding-bottom", Property::PADDING_BOTTOM, false, Object(Length(0)), std::bind(&PropertyParser::parseWidth, _1, _2));
		PropertyRegistrar property006("padding", std::bind(&PropertyParser::parseWidthList, _1, _2));
		PropertyRegistrar property007("margin-left", Property::MARGIN_LEFT, false, Object(Width(0)), std::bind(&PropertyParser::parseWidth, _1, _2));
		PropertyRegistrar property008("margin-right", Property::MARGIN_RIGHT, false, Object(Width(0)), std::bind(&PropertyParser::parseWidth, _1, _2));
		PropertyRegistrar property009("margin-top", Property::MARGIN_TOP, false, Object(Width(0)), std::bind(&PropertyParser::parseWidth, _1, _2));
		PropertyRegistrar property010("margin-bottom", Property::MARGIN_BOTTOM, false, Object(Width(0)), std::bind(&PropertyParser::parseWidth, _1, _2));
		PropertyRegistrar property011("margin", std::bind(&PropertyParser::parseWidthList, _1, _2));
		PropertyRegistrar property012("border-top-color", Property::BORDER_TOP_COLOR, false, Object(KRE::Color::colorWhite()), std::bind(&PropertyParser::parseColor, _1, _2));
		PropertyRegistrar property013("border-left-color", Property::BORDER_LEFT_COLOR, false, Object(KRE::Color::colorWhite()), std::bind(&PropertyParser::parseColor, _1, _2));
		PropertyRegistrar property014("border-bottom-color", Property::BORDER_BOTTOM_COLOR, false, Object(KRE::Color::colorWhite()), std::bind(&PropertyParser::parseColor, _1, _2));
		PropertyRegistrar property015("border-right-color", Property::BORDER_RIGHT_COLOR, false, Object(KRE::Color::colorWhite()), std::bind(&PropertyParser::parseColor, _1, _2));
		//PropertyRegistrar property016("border-color", std::bind(&PropertyParser::parseColorList, _1, _2));
		PropertyRegistrar property017("border-top-width", Property::BORDER_TOP_WIDTH, false, Object(Length(border_width_medium)), std::bind(&PropertyParser::parseBorderWidth, _1, _2));
		PropertyRegistrar property018("border-left-width", Property::BORDER_LEFT_WIDTH, false, Object(Length(border_width_medium)), std::bind(&PropertyParser::parseBorderWidth, _1, _2));
		PropertyRegistrar property019("border-bottom-width", Property::BORDER_BOTTOM_WIDTH, false, Object(Length(border_width_medium)), std::bind(&PropertyParser::parseBorderWidth, _1, _2));
		PropertyRegistrar property020("border-right-width", Property::BORDER_RIGHT_WIDTH, false, Object(Length(border_width_medium)), std::bind(&PropertyParser::parseBorderWidth, _1, _2));
		//PropertyRegistrar property021("border-width", std::bind(&PropertyParser::parseBorderWidthList, _1, _2));
		PropertyRegistrar property022("border-top-style", Property::BORDER_TOP_STYLE, false, Object(CssBorderStyle::NONE), std::bind(&PropertyParser::parseBorderStyle, _1, _2));
		PropertyRegistrar property023("border-left-style", Property::BORDER_LEFT_STYLE, false, Object(CssBorderStyle::NONE), std::bind(&PropertyParser::parseBorderStyle, _1, _2));
		PropertyRegistrar property024("border-bottom-style", Property::BORDER_BOTTOM_STYLE, false, Object(CssBorderStyle::NONE), std::bind(&PropertyParser::parseBorderStyle, _1, _2));
		PropertyRegistrar property025("border-right-style", Property::BORDER_RIGHT_STYLE, false, Object(CssBorderStyle::NONE), std::bind(&PropertyParser::parseBorderStyle, _1, _2));
		//PropertyRegistrar property026("border-style", std::bind(&PropertyParser::parseBorderStyleList, _1, _2));
		//PropertyRegistrar property027("border", std::bind(&PropertyParser::parseBorderList, _1, _2));
		PropertyRegistrar property026("display", Property::DISPLAY, false, Object(CssDisplay::INLINE), std::bind(&PropertyParser::parseDisplay, _1, _2));	
		PropertyRegistrar property027("width", Property::WIDTH, false, Object(Width(true)), std::bind(&PropertyParser::parseWidth, _1, _2));	
		PropertyRegistrar property028("height", Property::HEIGHT, false, Object(Width(true)), std::bind(&PropertyParser::parseWidth, _1, _2));
		PropertyRegistrar property029("white-space", Property::WHITE_SPACE, true, Object(CssWhitespace::NORMAL), std::bind(&PropertyParser::parseWhitespace, _1, _2));	
		PropertyRegistrar property030("font-family", Property::FONT_FAMILY, true, Object(get_default_fonts()), std::bind(&PropertyParser::parseFontFamily, _1, _2));	
		PropertyRegistrar property031("font-size", Property::FONT_SIZE, true, Object(12.0), std::bind(&PropertyParser::parseFontSize, _1, _2));
		PropertyRegistrar property032("font-style", Property::FONT_STYLE, true, Object(CssFontStyle::NORMAL), std::bind(&PropertyParser::parseFontStyle, _1, _2));
		PropertyRegistrar property033("font-variant", Property::FONT_VARIANT, true, Object(CssFontVariant::NORMAL), std::bind(&PropertyParser::parseFontVariant, _1, _2));
		PropertyRegistrar property034("font-weight", Property::FONT_WEIGHT, true, Object(400.0), std::bind(&PropertyParser::parseFontWeight, _1, _2));
		//PropertyRegistrar property035("font", Object(Font), std::bind(&PropertyParser::parseFont, _1, _2));
		PropertyRegistrar property036("letter-spacing", Property::LETTER_SPACING, true, Object(Length(0)), std::bind(&PropertyParser::parseSpacing, _1, _2));
		PropertyRegistrar property037("word-spacing", Property::WORD_SPACING, true, Object(Length(0)), std::bind(&PropertyParser::parseSpacing, _1, _2));
		PropertyRegistrar property038("text-align", Property::TEXT_ALIGN, true, Object(CssTextAlign::NORMAL), std::bind(&PropertyParser::parseTextAlign, _1, _2));
		PropertyRegistrar property039("direction", Property::DIRECTION, true, Object(CssDirection::LTR), std::bind(&PropertyParser::parseDirection, _1, _2));
		PropertyRegistrar property040("text-transform", Property::TEXT_TRANSFORM, true, Object(CssTextTransform::NONE), std::bind(&PropertyParser::parseTextTransform, _1, _2));
		PropertyRegistrar property041("line-height", Property::LINE_HEIGHT, true, Object(Length(1.15)), std::bind(&PropertyParser::parseLineHeight, _1, _2));
		PropertyRegistrar property042("overflow", Property::CSS_OVERFLOW, false, Object(CssOverflow::VISIBLE), std::bind(&PropertyParser::parseOverflow, _1, _2));
	}

	PropertyList::PropertyList()
		: properties_()
	{
	}

	void PropertyList::addProperty(Property p, StylePtr o)
	{
		PropertyInfo defaults = get_property_info_table()[static_cast<int>(p)];

		auto it = properties_.find(p);
		if(it == properties_.end()) {
			// unconditionally add new properties
			properties_[p] = o;
		} else {
			// check for important flag before merging.
			if((it->second->isImportant() && o->isImportant()) || !it->second->isImportant()) {
				it->second = o;
			}
		}
	}

	void PropertyList::addProperty(const std::string& name, StylePtr o)
	{
		ASSERT_LOG(o != nullptr, "Adding invalid property is nullptr.");
		auto prop_it = get_property_table().find(name);
		if(prop_it == get_property_table().end()) {
			LOG_ERROR("Not adding property '" << name << "' since we have no mapping for it.");
			return;
		}
		addProperty(prop_it->second.value, o);
	}

	StylePtr PropertyList::getProperty(Property value) const
	{
		auto it = properties_.find(value);
		if(it == properties_.end()) {
			return nullptr;
		}
		return it->second;
	}

	void PropertyList::merge(const PropertyList& plist)
	{
		for(auto& p : plist.properties_) {
			addProperty(p.first, p.second);
		}
	}

	const std::string& get_property_name(Property p)
	{
		const int ndx = static_cast<int>(p);
		ASSERT_LOG(ndx < static_cast<int>(get_property_info_table().size()), 
			"Requested name of property, index not in table: " << ndx);
		return get_property_info_table()[ndx].name;
	}

	const PropertyInfo& get_default_property_info(Property p)
	{
		const int ndx = static_cast<int>(p);
		ASSERT_LOG(ndx < static_cast<int>(get_property_info_table().size()), 
			"Requested property info, index not in table: " << ndx);
		return get_property_info_table()[ndx];
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
		it->second.fn(this, name);

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

	StylePtr PropertyParser::parseColorInternal()
	{
		auto color = CssColor::create();
		if(isToken(TokenId::IDENT)) {
			const std::string& ref = (*it_)->getStringValue();
			advance();
			if(ref == "transparent") {
				color->setParam(CssColorParam::TRANSPARENT);
			} else if(ref == "inherit") {
				return std::make_shared<Style>(true);
			} else {
				// color value is in ref.
				color->setColor(KRE::Color(ref));
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
				color->setColor(KRE::Color(values[0], values[1], values[2]));
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
				color->setColor(KRE::Color(values[0], values[1], values[2], values[3]));
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
				color->setColor(hsla_to_color(values[0], values[1], values[2], 1.0));
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
				color->setColor(hsla_to_color(values[0], values[1], values[2], values[3]));
			}
		} else if(isToken(TokenId::HASH)) {
			const std::string& ref = (*it_)->getStringValue();
			// is #fff or #ff00ff representation
			color->setColor(KRE::Color(ref));
			advance();
		}
		return color;
	}

	StylePtr PropertyParser::parseWidthInternal()
	{
		skipWhitespace();
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			if(ref == "inherit") {
				advance();
				return std::make_shared<Style>(true);
			} else if(ref == "auto") {
				advance();
				return std::make_shared<Width>(true);
			}
		}
		return std::make_shared<Length>(parseLengthInternal());
	}

	Length PropertyParser::parseLengthInternal(NumericParseOptions opts)
	{
		skipWhitespace();
		if(isToken(TokenId::DIMENSION) && (opts & LENGTH)) {
			const std::string units = (*it_)->getStringValue();
			double value = (*it_)->getNumericValue();
			advance();
			return Length(value, units);
		} else if(isToken(TokenId::PERCENT) && (opts & PERCENTAGE)) {
			double d = (*it_)->getNumericValue();
			advance();
			return Length(d, true);
		} else if(isToken(TokenId::NUMBER) && (opts & NUMBER)) {
			double d = (*it_)->getNumericValue();
			advance();
			return Length(d);
		}
		throw ParserError(formatter() << "Unrecognised value for property: "  << (*it_)->toString());
	}

	StylePtr PropertyParser::parseBorderWidthInternal()
	{
		skipWhitespace();
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			if(ref == "inherit") {
				advance();
			} else if(ref == "thin") {
				advance();
				return std::make_shared<Width>(Length(border_width_thin));
			} else if(ref == "medium") {
				advance();
				return std::make_shared<Width>(Length(border_width_medium));
			} else if(ref == "thick") {
				advance();
				return std::make_shared<Width>(Length(border_width_thick));
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
		StylePtr w1 = parseWidthInternal();
		skipWhitespace();
		if(isToken(TokenId::EOF_TOKEN) || isToken(TokenId::RBRACE) || isToken(TokenId::SEMICOLON) || isTokenDelimiter("!")) {
			plist_.addProperty(name + "-top", w1);
			plist_.addProperty(name + "-bottom", w1);
			plist_.addProperty(name + "-right", w1);
			plist_.addProperty(name + "-left", w1);
			return;
		}
		StylePtr w2 = parseWidthInternal();
		skipWhitespace();
		if(isToken(TokenId::EOF_TOKEN) || isToken(TokenId::RBRACE) || isToken(TokenId::SEMICOLON) || isTokenDelimiter("!")) {
			plist_.addProperty(name + "-top", w1);
			plist_.addProperty(name + "-bottom", w1);
			plist_.addProperty(name + "-right", w2);
			plist_.addProperty(name + "-left", w2);
			return;
		}
		StylePtr w3 = parseWidthInternal();
		skipWhitespace();
		if(isToken(TokenId::EOF_TOKEN) || isToken(TokenId::RBRACE) || isToken(TokenId::SEMICOLON) || isTokenDelimiter("!")) {
			plist_.addProperty(name + "-top", w1);
			plist_.addProperty(name + "-right", w2);
			plist_.addProperty(name + "-left", w2);
			plist_.addProperty(name + "-bottom", w3);
			return;
		}
		StylePtr w4 = parseWidthInternal();
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

	StylePtr PropertyParser::parseBorderStyleInternal()
	{
		CssBorderStyle bs = CssBorderStyle::NONE;
		skipWhitespace();
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			skipWhitespace();
			if(ref == "none") {
				bs = CssBorderStyle::NONE;
			} else if(ref == "inherit") {
				return std::make_shared<Style>(true);
			} else if(ref == "hidden") {
				bs = CssBorderStyle::HIDDEN;
			} else if(ref == "dotted") {
				bs = CssBorderStyle::DOTTED;
			} else if(ref == "dashed") {
				bs = CssBorderStyle::DASHED;
			} else if(ref == "solid") {
				bs = CssBorderStyle::SOLID;
			} else if(ref == "double") {
				bs = CssBorderStyle::DOUBLE;
			} else if(ref == "groove") {
				bs = CssBorderStyle::GROOVE;
			} else if(ref == "ridge") {
				bs = CssBorderStyle::RIDGE;
			} else if(ref == "inset") {
				bs = CssBorderStyle::INSET;
			} else if(ref == "outset") {
				bs = CssBorderStyle::OUTSET;
			} else {
				throw ParserError(formatter() << "Unwxpected identifier '" << ref << "' while parsing border style");
			}
			return std::make_shared<BorderStyle>(bs);
		}
		throw ParserError(formatter() << "Unexpected IDENTIFIER, found: " << (*it_)->toString());
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
				plist_.addProperty(name, std::make_shared<Style>(true));
				return;
			} else {
				throw ParserError(formatter() << "Unrecognised token for display property: " << ref);
			}
		}
		plist_.addProperty(name, Display::create(display));
	}

	void PropertyParser::parseWhitespace(const std::string& name)
	{
		CssWhitespace ws = CssWhitespace::NORMAL;
		if(isToken(TokenId::IDENT)) {
			std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "normal") {
				ws = CssWhitespace::NORMAL;
			} else if(ref == "inherit") {
				plist_.addProperty(name, std::make_shared<Style>(true));
				return;
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
		plist_.addProperty(name, Whitespace::create(ws));
	}

	void PropertyParser::parseFontFamily(const std::string& name)
	{
		std::vector<std::string> font_list;
		parseCSVStringList(TokenId::DELIM, [&font_list](int n, const std::string& str) {
			font_list.emplace_back(str);
		});
		plist_.addProperty(name, FontFamily::create(font_list));
	}
	
	void PropertyParser::parseFontSize(const std::string& name)
	{
		FontSize fs;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(name, std::make_shared<Style>(true));
				return;
			} else if(ref == "xx-small") {
				fs.setFontSize(FontSizeAbsolute::XX_SMALL);
			} else if(ref == "x-small") {
				fs.setFontSize(FontSizeAbsolute::X_SMALL);
			} else if(ref == "small") {
				fs.setFontSize(FontSizeAbsolute::SMALL);
			} else if(ref == "medium") {
				fs.setFontSize(FontSizeAbsolute::MEDIUM);
			} else if(ref == "large") {
				fs.setFontSize(FontSizeAbsolute::LARGE);
			} else if(ref == "x-large") {
				fs.setFontSize(FontSizeAbsolute::X_LARGE);
			} else if(ref == "xx-large") {
				fs.setFontSize(FontSizeAbsolute::XX_LARGE);
			} else if(ref == "larger") {
				fs.setFontSize(FontSizeRelative::LARGER);
			} else if(ref == "smaller") {
				fs.setFontSize(FontSizeRelative::SMALLER);
			} else {
				throw ParserError(formatter() << "Unrecognised identifier for '" << name << "' property: " << ref);
			}
		} else if(isToken(TokenId::DIMENSION)) {
			const std::string units = (*it_)->getStringValue();
			double value = (*it_)->getNumericValue();
			advance();
			fs.setFontSize(Length(value, units));
		} else if(isToken(TokenId::PERCENT)) {
			double d = (*it_)->getNumericValue();
			advance();
			fs.setFontSize(Length(d, true));
		} else if(isToken(TokenId::NUMBER)) {
			double d = (*it_)->getNumericValue();
			advance();
			fs.setFontSize(Length(d));
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << name << "': "  << (*it_)->toString());
		}		
		plist_.addProperty(name, FontSize::create(fs));
	}

	void PropertyParser::parseFontWeight(const std::string& name)
	{
		FontWeight fw;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(name, std::make_shared<Style>(true));
				return;
			} else if(ref == "lighter") {
				fw.setRelative(FontWeightRelative::LIGHTER);
			} else if(ref == "bolder") {
				fw.setRelative(FontWeightRelative::BOLDER);
			} else if(ref == "normal") {
				fw.setWeight(400);
			} else if(ref == "bold") {
				fw.setWeight(700);
			} else {
				throw ParserError(formatter() << "Unrecognised identifier for '" << name << "' property: " << ref);
			}
		} else if(isToken(TokenId::NUMBER)) {
			fw.setWeight((*it_)->getNumericValue());
			advance();
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << name << "': "  << (*it_)->toString());
		}
		plist_.addProperty(name, FontWeight::create(fw));
	}

	void PropertyParser::parseSpacing(const std::string& name)
	{
		Length spacing;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(name, std::make_shared<Style>(true));
				return;
			} else if(ref == "normal") {
				// spacing defaults to 0
			} else {
				throw ParserError(formatter() << "Unrecognised identifier for '" << name << "' property: " << ref);
			}
		} else if(isToken(TokenId::DIMENSION)) {
			const std::string units = (*it_)->getStringValue();
			double value = (*it_)->getNumericValue();
			advance();
			spacing = Length(value, units);
		} else if(isToken(TokenId::NUMBER)) {
			double d = (*it_)->getNumericValue();
			advance();
			spacing = Length(d);
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << name << "': "  << (*it_)->toString());
		}
		plist_.addProperty(name, Length::create(spacing));
	}

	void PropertyParser::parseTextAlign(const std::string& name)
	{
		CssTextAlign ta = CssTextAlign::NORMAL;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(name, std::make_shared<Style>(true));
				return;
			} else if(ref == "left") {
				ta = CssTextAlign::LEFT;
			} else if(ref == "right") {
				ta = CssTextAlign::RIGHT;
			} else if(ref == "center" || ref == "centre") {
				ta = CssTextAlign::CENTER;
			} else if(ref == "justify") {
				ta = CssTextAlign::JUSTIFY;
			} else {
				throw ParserError(formatter() << "Unrecognised identifier for '" << name << "' property: " << ref);
			}
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << name << "': "  << (*it_)->toString());
		}
		plist_.addProperty(name, TextAlign::create(ta));
	}

	void PropertyParser::parseDirection(const std::string& name)
	{
		CssDirection dir = CssDirection::LTR;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(name, std::make_shared<Style>(true));
				return;
			} else if(ref == "ltr") {
				dir = CssDirection::LTR;
			} else if(ref == "rtl") {
				dir = CssDirection::RTL;
			} else {
				throw ParserError(formatter() << "Unrecognised identifier for '" << name << "' property: " << ref);
			}
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << name << "': "  << (*it_)->toString());
		}
		plist_.addProperty(name, Direction::create(dir));
	}

	void PropertyParser::parseTextTransform(const std::string& name)
	{
		CssTextTransform tt = CssTextTransform::NONE;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(name, std::make_shared<Style>(true));
				return;
			} else if(ref == "capitalize") {
				tt = CssTextTransform::CAPITALIZE;
			} else if(ref == "uppercase") {
				tt = CssTextTransform::UPPERCASE;
			} else if(ref == "lowercase") {
				tt = CssTextTransform::LOWERCASE;
			} else if(ref == "none") {
				tt = CssTextTransform::NONE;
			} else {
				throw ParserError(formatter() << "Unrecognised identifier for '" << name << "' property: " << ref);
			}
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << name << "': "  << (*it_)->toString());
		}
		plist_.addProperty(name, TextTransform::create(tt));
	}

	void PropertyParser::parseLineHeight(const std::string& name)
	{
		Length lh(1.1);
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(name, std::make_shared<Style>(true));
				return;
			} else if(ref == "normal") {
				// do nothing as already set in default
			} else {
				throw ParserError(formatter() << "Unrecognised identifier for '" << name << "' property: " << ref);
			}
		} else {
			lh = parseLengthInternal(NUMERIC);
		}
		plist_.addProperty(name, Length::create(lh));
	}

	void PropertyParser::parseFontStyle(const std::string& name)
	{
		CssFontStyle fs = CssFontStyle::NORMAL;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(name, std::make_shared<Style>(true));
				return;
			} else if(ref == "italic") {
				fs = CssFontStyle::ITALIC;
			} else if(ref == "normal") {
				fs = CssFontStyle::NORMAL;
			} else if(ref == "oblique") {
				fs = CssFontStyle::OBLIQUE;
			} else {
				throw ParserError(formatter() << "Unrecognised identifier for '" << name << "' property: " << ref);
			}
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << name << "': "  << (*it_)->toString());
		}
		plist_.addProperty(name, FontStyle::create(fs));
	}

	void PropertyParser::parseFontVariant(const std::string& name)
	{
		CssFontVariant fv = CssFontVariant::NORMAL;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(name, std::make_shared<Style>(true));
				return;
			} else if(ref == "normal") {
				fv = CssFontVariant::NORMAL;
			} else if(ref == "small-caps") {
				fv = CssFontVariant::SMALL_CAPS;
			} else {
				throw ParserError(formatter() << "Unrecognised identifier for '" << name << "' property: " << ref);
			}
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << name << "': "  << (*it_)->toString());
		}
		plist_.addProperty(name, FontVariant::create(fv));
	}

	void PropertyParser::parseOverflow(const std::string& name)
	{
		CssOverflow of = CssOverflow::VISIBLE;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(name, std::make_shared<Style>(true));
				return;
			} else if(ref == "visible") {
				of = CssOverflow::VISIBLE;
			} else if(ref == "hidden") {
				of = CssOverflow::HIDDEN;
			} else if(ref == "scroll") {
				of = CssOverflow::SCROLL;
			} else if(ref == "auto") {
				of = CssOverflow::AUTO;
			} else {
				throw ParserError(formatter() << "Unrecognised identifier for '" << name << "' property: " << ref);
			}
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << name << "': "  << (*it_)->toString());
		}
		plist_.addProperty(name, Overflow::create(of));
	}
}
