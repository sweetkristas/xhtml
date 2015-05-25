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
		const int fixed_point_scale = 65536;

		const xhtml::FixedPoint border_width_thin = 2 * fixed_point_scale;
		const xhtml::FixedPoint border_width_medium = 4 * fixed_point_scale;
		const xhtml::FixedPoint border_width_thick = 10 * fixed_point_scale;

		const xhtml::FixedPoint line_height_scale = (115 * fixed_point_scale) / 100;

		const xhtml::FixedPoint default_font_size = 12 * fixed_point_scale;

		typedef std::function<void(PropertyParser*, const std::string&, const std::string&)> ParserFunction;
		struct PropertyNameInfo
		{
			PropertyNameInfo() : value(Property::MAX_PROPERTIES), fn() {}
			PropertyNameInfo(Property p, std::function<void(PropertyParser*)> f) : value(p), fn(f) {}
			Property value;
			std::function<void(PropertyParser*)> fn;
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

		KRE::Color hsla_to_color(float h, float s, float l, float a)
		{
			const float hue_upper_limit = 360.0f;
			float c = 0.0f, m = 0.0f, x = 0.0f;
			c = (1.0f - std::abs(2.f * l - 1.0f)) * s;
			m = 1.0f * (l - 0.5f * c);
			x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.f) - 1.0f));
			if (h >= 0.0f && h < (hue_upper_limit / 6.0f)) {
				return KRE::Color(c+m, x+m, m, a);
			} else if (h >= (hue_upper_limit / 6.0f) && h < (hue_upper_limit / 3.0f)) {
				return KRE::Color(x+m, c+m, m, a);
			} else if (h < (hue_upper_limit / 3.0f) && h < (hue_upper_limit / 2.0f)) {
				return KRE::Color(m, c+m, x+m, a);
			} else if (h >= (hue_upper_limit / 2.0f) && h < (2.0f * hue_upper_limit / 3.0f)) {
				return KRE::Color(m, x+m, c+m, a);
			} else if (h >= (2.0 * hue_upper_limit / 3.0f) && h < (5.0f * hue_upper_limit / 6.0f)) {
				return KRE::Color(x+m, m, c+m, a);
			} else if (h >= (5.0 * hue_upper_limit / 6.0f) && h < hue_upper_limit) {
				return KRE::Color(c+m, m, x+m, a);
			}
			return KRE::Color(m, m, m, a);
		}

		struct PropertyRegistrar
		{
			PropertyRegistrar(const std::string& name, std::function<void(PropertyParser*)> fn) {
				get_property_table()[name] = PropertyNameInfo(Property::MAX_PROPERTIES, fn);
			}
			PropertyRegistrar(const std::string& name, Property p, bool inherited, Object def, std::function<void(PropertyParser*)> fn) {
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
		
		PropertyRegistrar property000("background-color", Property::BACKGROUND_COLOR, false, Object(CssColor(CssColorParam::TRANSPARENT)), std::bind(&PropertyParser::parseColor, _1, "background-color", ""));
		PropertyRegistrar property001("color", Property::COLOR, true, Object(CssColor(CssColorParam::VALUE)), std::bind(&PropertyParser::parseColor, _1, "color", ""));
		PropertyRegistrar property002("padding-left", Property::PADDING_LEFT, false, Object(Length(0)), std::bind(&PropertyParser::parseLength, _1, "padding-left", ""));
		PropertyRegistrar property003("padding-right", Property::PADDING_RIGHT, false, Object(Length(0)), std::bind(&PropertyParser::parseLength, _1, "padding-right", ""));
		PropertyRegistrar property004("padding-top", Property::PADDING_TOP, false, Object(Length(0)), std::bind(&PropertyParser::parseLength, _1, "padding-top", ""));
		PropertyRegistrar property005("padding-bottom", Property::PADDING_BOTTOM, false, Object(Length(0)), std::bind(&PropertyParser::parseLength, _1, "padding-bottom", ""));
		PropertyRegistrar property006("padding", std::bind(&PropertyParser::parseLengthList, _1, "padding", ""));
		PropertyRegistrar property007("margin-left", Property::MARGIN_LEFT, false, Object(Width(0)), std::bind(&PropertyParser::parseWidth, _1, "margin-left", ""));
		PropertyRegistrar property008("margin-right", Property::MARGIN_RIGHT, false, Object(Width(0)), std::bind(&PropertyParser::parseWidth, _1, "margin-right", ""));
		PropertyRegistrar property009("margin-top", Property::MARGIN_TOP, false, Object(Width(0)), std::bind(&PropertyParser::parseWidth, _1, "margin-top", ""));
		PropertyRegistrar property010("margin-bottom", Property::MARGIN_BOTTOM, false, Object(Width(0)), std::bind(&PropertyParser::parseWidth, _1, "margin-bottom", ""));
		PropertyRegistrar property011("margin", std::bind(&PropertyParser::parseWidthList, _1, "margin", ""));
		PropertyRegistrar property012("border-top-color", Property::BORDER_TOP_COLOR, false, Object(CssColor(CssColorParam::CURRENT)), std::bind(&PropertyParser::parseColor, _1, "border-top-color", ""));
		PropertyRegistrar property013("border-left-color", Property::BORDER_LEFT_COLOR, false, Object(CssColor(CssColorParam::CURRENT)), std::bind(&PropertyParser::parseColor, _1, "border-left-color", ""));
		PropertyRegistrar property014("border-bottom-color", Property::BORDER_BOTTOM_COLOR, false, Object(CssColor(CssColorParam::CURRENT)), std::bind(&PropertyParser::parseColor, _1, "border-bottom-color", ""));
		PropertyRegistrar property015("border-right-color", Property::BORDER_RIGHT_COLOR, false, Object(CssColor(CssColorParam::CURRENT)), std::bind(&PropertyParser::parseColor, _1, "border-right-color", ""));
		PropertyRegistrar property016("border-color", std::bind(&PropertyParser::parseColorList, _1, "border", "color"));
		PropertyRegistrar property017("border-top-width", Property::BORDER_TOP_WIDTH, false, Object(Length(border_width_medium)), std::bind(&PropertyParser::parseBorderWidth, _1, "border-top-width", ""));
		PropertyRegistrar property018("border-left-width", Property::BORDER_LEFT_WIDTH, false, Object(Length(border_width_medium)), std::bind(&PropertyParser::parseBorderWidth, _1, "border-left-width", ""));
		PropertyRegistrar property019("border-bottom-width", Property::BORDER_BOTTOM_WIDTH, false, Object(Length(border_width_medium)), std::bind(&PropertyParser::parseBorderWidth, _1, "border-bottom-width", ""));
		PropertyRegistrar property020("border-right-width", Property::BORDER_RIGHT_WIDTH, false, Object(Length(border_width_medium)), std::bind(&PropertyParser::parseBorderWidth, _1, "border-right-width", ""));
		PropertyRegistrar property021("border-width", std::bind(&PropertyParser::parseBorderWidthList, _1, "border", "width"));
		PropertyRegistrar property022("border-top-style", Property::BORDER_TOP_STYLE, false, Object(CssBorderStyle::NONE), std::bind(&PropertyParser::parseBorderStyle, _1, "border-top-style", ""));
		PropertyRegistrar property023("border-left-style", Property::BORDER_LEFT_STYLE, false, Object(CssBorderStyle::NONE), std::bind(&PropertyParser::parseBorderStyle, _1, "border-left-style", ""));
		PropertyRegistrar property024("border-bottom-style", Property::BORDER_BOTTOM_STYLE, false, Object(CssBorderStyle::NONE), std::bind(&PropertyParser::parseBorderStyle, _1, "border-bottom-style", ""));
		PropertyRegistrar property025("border-right-style", Property::BORDER_RIGHT_STYLE, false, Object(CssBorderStyle::NONE), std::bind(&PropertyParser::parseBorderStyle, _1, "border-right-style", ""));
		//PropertyRegistrar property027("border", std::bind(&PropertyParser::parseBorderList, _1, _2));
		PropertyRegistrar property026("display", Property::DISPLAY, false, Object(CssDisplay::INLINE), std::bind(&PropertyParser::parseDisplay, _1, "display", ""));	
		PropertyRegistrar property027("width", Property::WIDTH, false, Object(Width(true)), std::bind(&PropertyParser::parseWidth, _1, "width", ""));	
		PropertyRegistrar property028("height", Property::HEIGHT, false, Object(Width(true)), std::bind(&PropertyParser::parseWidth, _1, "height", ""));
		PropertyRegistrar property029("white-space", Property::WHITE_SPACE, true, Object(CssWhitespace::NORMAL), std::bind(&PropertyParser::parseWhitespace, _1, "white-space", ""));	
		PropertyRegistrar property030("font-family", Property::FONT_FAMILY, true, Object(get_default_fonts()), std::bind(&PropertyParser::parseFontFamily, _1, "font-family", ""));	
		PropertyRegistrar property031("font-size", Property::FONT_SIZE, true, Object(default_font_size), std::bind(&PropertyParser::parseFontSize, _1, "font-size", ""));
		PropertyRegistrar property032("font-style", Property::FONT_STYLE, true, Object(CssFontStyle::NORMAL), std::bind(&PropertyParser::parseFontStyle, _1, "font-style", ""));
		PropertyRegistrar property033("font-variant", Property::FONT_VARIANT, true, Object(CssFontVariant::NORMAL), std::bind(&PropertyParser::parseFontVariant, _1, "font-variant", ""));
		PropertyRegistrar property034("font-weight", Property::FONT_WEIGHT, true, Object(400), std::bind(&PropertyParser::parseFontWeight, _1, "font-weight", ""));
		//PropertyRegistrar property035("font", Object(Font), std::bind(&PropertyParser::parseFont, _1, _2));
		PropertyRegistrar property036("letter-spacing", Property::LETTER_SPACING, true, Object(Length(0)), std::bind(&PropertyParser::parseSpacing, _1, "letter-spacing", ""));
		PropertyRegistrar property037("word-spacing", Property::WORD_SPACING, true, Object(Length(0)), std::bind(&PropertyParser::parseSpacing, _1, "word-spacing", ""));
		PropertyRegistrar property038("text-align", Property::TEXT_ALIGN, true, Object(CssTextAlign::NORMAL), std::bind(&PropertyParser::parseTextAlign, _1, "text-align", ""));
		PropertyRegistrar property039("direction", Property::DIRECTION, true, Object(CssDirection::LTR), std::bind(&PropertyParser::parseDirection, _1, "direction", ""));
		PropertyRegistrar property040("text-transform", Property::TEXT_TRANSFORM, true, Object(CssTextTransform::NONE), std::bind(&PropertyParser::parseTextTransform, _1, "text-transform", ""));
		PropertyRegistrar property041("line-height", Property::LINE_HEIGHT, true, Object(Length(line_height_scale)), std::bind(&PropertyParser::parseLineHeight, _1, "line-height", ""));
		PropertyRegistrar property042("overflow", Property::CSS_OVERFLOW, false, Object(CssOverflow::VISIBLE), std::bind(&PropertyParser::parseOverflow, _1, "overflow", ""));
		PropertyRegistrar property043("position", Property::POSITION, false, Object(CssPosition::STATIC), std::bind(&PropertyParser::parsePosition, _1, "position", ""));
		PropertyRegistrar property044("float", Property::FLOAT, false, Object(CssFloat::NONE), std::bind(&PropertyParser::parseFloat, _1, "float", ""));
		PropertyRegistrar property045("left", Property::LEFT, false, Object(Width(true)), std::bind(&PropertyParser::parseWidth, _1, "left", ""));
		PropertyRegistrar property046("top", Property::TOP, false, Object(Width(true)), std::bind(&PropertyParser::parseWidth, _1, "top", ""));
		PropertyRegistrar property047("right", Property::RIGHT, false, Object(Width(true)), std::bind(&PropertyParser::parseWidth, _1, "right", ""));
		PropertyRegistrar property048("bottom", Property::BOTTOM, false, Object(Width(true)), std::bind(&PropertyParser::parseWidth, _1, "bottom", ""));
		PropertyRegistrar property049("background-image", Property::BACKGROUND_IMAGE, false, Object(UriStyle(true)), std::bind(&PropertyParser::parseUri, _1, "background-image", ""));
		PropertyRegistrar property050("background-repeat", Property::BACKGROUND_REPEAT, false, Object(CssBackgroundRepeat::REPEAT), std::bind(&PropertyParser::parseBackgroundRepeat, _1, "background-repeat", ""));
		PropertyRegistrar property051("background-position", Property::BACKGROUND_POSITION, false, Object(BackgroundPosition()), std::bind(&PropertyParser::parseBackgroundPosition, _1, "background-position", ""));
		PropertyRegistrar property052("list-style-type", Property::LIST_STYLE_TYPE, true, Object(ListStyleType()), std::bind(&PropertyParser::parseListStyleType, _1, "list-style-type", ""));
		PropertyRegistrar property053("border-style", std::bind(&PropertyParser::parseBorderStyleList, _1, "border", "style"));
		// background-attachment
		// clear
		// clip
		// content
		// counter-increment
		// counter-reset
		// cursor
		// list-style-image
		// list-style-position
		// max-height
		// max-width
		// min-height
		// min-width
		// outline-color
		// outline-style
		// outline-width
		// quotes
		// text-decoration
		// text-indent
		// unicode-bidi
		// vertical-align
		// visibility
		// z-index

		// Compount properties.
		// background
		// border
		// font
		// list-style
		// outline

		// Table related properties
		// border-collapse
		// border-spacing
		// caption-side
		// empty-cells
		// table-layout
		
		// Paged
		// orphans
		// widows

		//transition -- transition-property, transition-duration, transition-timing-function, transition-delay
		//text-shadow
	}

	PropertyList::PropertyList()
		: properties_()
	{
	}

	void PropertyList::addProperty(Property p, StylePtr o, const Specificity& specificity)
	{
		PropertyInfo defaults = get_property_info_table()[static_cast<int>(p)];

		auto it = properties_.find(p);
		if(it == properties_.end()) {
			// unconditionally add new properties
			properties_[p] = PropertyStyle(o, specificity);
		} else {
			// check for important flag before merging.
			/*LOG_DEBUG("property: " << get_property_info_table()[static_cast<int>(p)].name << ", current spec: "
				<< it->second.specificity[0] << "," << it->second.specificity[1] << "," << it->second.specificity[2]
				<< ", new spec: " << specificity[0] << "," << specificity[1] << "," << specificity[2] 
				<< ", test: " << (it->second.specificity <= specificity ? "true" : "false"));*/
			if(((it->second.style->isImportant() && o->isImportant()) || !it->second.style->isImportant()) && it->second.specificity <= specificity) {
				it->second = PropertyStyle(o, specificity);
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
		return it->second.style;
	}

	void PropertyList::merge(const Specificity& specificity, const PropertyList& plist)
	{
		for(auto& p : plist.properties_) {
			addProperty(p.first, p.second.style, specificity);
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
		it->second.fn(this);

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

	void PropertyParser::parseCSVNumberList(TokenId end_token, std::function<void(int, float, bool)> fn)
	{
		auto toks = parseCSVList(end_token);
		int n = 0;
		for(auto& t : toks) {
			if(t->id() == TokenId::PERCENT) {
				fn(n, static_cast<float>(t->getNumericValue()), true);
			} else if(t->id() == TokenId::NUMBER) {
				fn(n, static_cast<float>(t->getNumericValue()), false);
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

	void PropertyParser::parseCSVNumberListFromIt(std::vector<TokenPtr>::const_iterator start, 
		std::vector<TokenPtr>::const_iterator end, 
		std::function<void(int, float, bool)> fn)
	{
		int n = 0;
		for(auto it = start; it != end; ++it) {
			auto& t = *it;
			if(t->id() == TokenId::NUMBER) {
				fn(n, static_cast<float>(t->getNumericValue()), false);
			} else if(t->id() == TokenId::PERCENT) {
				fn(n, static_cast<float>(t->getNumericValue()), true);
			} else if(t->id() == TokenId::COMMA) {
				++n;
			}
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
			// XXX  we need to parse (*it_)->getParameters() here for the number list!			
			if(ref == "rgb") {
				int values[3] = { 255, 255, 255 };
				parseCSVNumberListFromIt((*it_)->getParameters().cbegin(), (*it_)->getParameters().end(), 
					[&values](int n, float value, bool is_percent) {
					if(n < 3) {
						if(is_percent) {
							value *= 255.0f / 100.0f;
						}
						values[n] = std::min(255, std::max(0, static_cast<int>(value)));
					}
				});
				advance();
				color->setColor(KRE::Color(values[0], values[1], values[2]));
			} else if(ref == "rgba") {
				int values[4] = { 255, 255, 255, 255 };
				parseCSVNumberListFromIt((*it_)->getParameters().cbegin(), (*it_)->getParameters().end(), 
					[&values](int n, float value, bool is_percent) {
					if(n < 4) {
						if(is_percent) {
							value *= 255.0f / 100.0f;
						}
						values[n] = std::min(255, std::max(0, static_cast<int>(value)));
					}
				});
				advance();
				color->setColor(KRE::Color(values[0], values[1], values[2], values[3]));
			} else if(ref == "hsl") {
				float values[3];
				const float multipliers[3] = { 360.0f, 1.0f, 1.0f };
				parseCSVNumberListFromIt((*it_)->getParameters().cbegin(), (*it_)->getParameters().end(), 
					[&values, &multipliers](int n, float value, bool is_percent) {
					if(n < 3) {
						if(is_percent) {
							value *= multipliers[n] / 100.0f;
						}
						values[n] = value;
					}
				});					
				advance();
				color->setColor(hsla_to_color(values[0], values[1], values[2], 1.0));
			} else if(ref == "hsla") {
				float values[4];
				const float multipliers[4] = { 360.0f, 1.0f, 1.0f, 1.0f };
				parseCSVNumberListFromIt((*it_)->getParameters().cbegin(), (*it_)->getParameters().end(), 
					[&values, &multipliers](int n, float value, bool is_percent) {
					if(n < 4) {
						if(is_percent) {
							value *= multipliers[n] / 100.0f;
						}
						values[n] = value;
					}
				});					
				advance();
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

	void PropertyParser::parseColorList(const std::string& prefix, const std::string& suffix)
	{
		StylePtr w1 = parseColorInternal();
		skipWhitespace();
		if(isToken(TokenId::EOF_TOKEN) || isToken(TokenId::RBRACE) || isToken(TokenId::SEMICOLON) || isTokenDelimiter("!")) {
			plist_.addProperty(prefix + "-top" + (!suffix.empty() ? "-" + suffix : ""), w1);
			plist_.addProperty(prefix + "-bottom" + (!suffix.empty() ? "-" + suffix : ""), w1);
			plist_.addProperty(prefix + "-right" + (!suffix.empty() ? "-" + suffix : ""), w1);
			plist_.addProperty(prefix + "-left" + (!suffix.empty() ? "-" + suffix : ""), w1);
			return;
		}
		StylePtr w2 = parseColorInternal();
		skipWhitespace();
		if(isToken(TokenId::EOF_TOKEN) || isToken(TokenId::RBRACE) || isToken(TokenId::SEMICOLON) || isTokenDelimiter("!")) {
			plist_.addProperty(prefix + "-top" + (!suffix.empty() ? "-" + suffix : ""), w1);
			plist_.addProperty(prefix + "-bottom" + (!suffix.empty() ? "-" + suffix : ""), w1);
			plist_.addProperty(prefix + "-right" + (!suffix.empty() ? "-" + suffix : ""), w2);
			plist_.addProperty(prefix + "-left" + (!suffix.empty() ? "-" + suffix : ""), w2);
			return;
		}
		StylePtr w3 = parseColorInternal();
		skipWhitespace();
		if(isToken(TokenId::EOF_TOKEN) || isToken(TokenId::RBRACE) || isToken(TokenId::SEMICOLON) || isTokenDelimiter("!")) {
			plist_.addProperty(prefix + "-top" + (!suffix.empty() ? "-" + suffix : ""), w1);
			plist_.addProperty(prefix + "-right" + (!suffix.empty() ? "-" + suffix : ""), w2);
			plist_.addProperty(prefix + "-left" + (!suffix.empty() ? "-" + suffix : ""), w2);
			plist_.addProperty(prefix + "-bottom" + (!suffix.empty() ? "-" + suffix : ""), w3);
			return;
		}
		StylePtr w4 = parseColorInternal();
		skipWhitespace();

		// four values, apply to individual elements.
		plist_.addProperty(prefix + "-top" + (!suffix.empty() ? "-" + suffix : ""), w1);
		plist_.addProperty(prefix + "-right" + (!suffix.empty() ? "-" + suffix : ""), w2);
		plist_.addProperty(prefix + "-bottom" + (!suffix.empty() ? "-" + suffix : ""), w3);
		plist_.addProperty(prefix + "-left" + (!suffix.empty() ? "-" + suffix : ""), w4);
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
		return std::make_shared<Width>(parseLengthInternal());
	}

	Length PropertyParser::parseLengthInternal(NumericParseOptions opts)
	{
		skipWhitespace();
		if(isToken(TokenId::DIMENSION) && (opts & LENGTH)) {
			const std::string units = (*it_)->getStringValue();
			xhtml::FixedPoint value = static_cast<xhtml::FixedPoint>((*it_)->getNumericValue() * fixed_point_scale);
			advance();
			return Length(value, units);
		} else if(isToken(TokenId::PERCENT) && (opts & PERCENTAGE)) {
			xhtml::FixedPoint d = static_cast<xhtml::FixedPoint>((*it_)->getNumericValue() * fixed_point_scale);
			advance();
			return Length(d, true);
		} else if(isToken(TokenId::NUMBER) && (opts & NUMBER)) {
			xhtml::FixedPoint d = static_cast<xhtml::FixedPoint>((*it_)->getNumericValue() * fixed_point_scale);
			advance();
			skipWhitespace();
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
				return std::make_shared<Length>(border_width_thin);
			} else if(ref == "medium") {
				advance();
				return std::make_shared<Length>(border_width_medium);
			} else if(ref == "thick") {
				advance();
				return std::make_shared<Length>(border_width_thick);
			}
		}	
		return std::make_shared<Length>(parseLengthInternal());
	}

	void PropertyParser::parseColor(const std::string& prefix, const std::string& suffix)
	{
		plist_.addProperty(prefix, parseColorInternal());
	}

	void PropertyParser::parseWidth(const std::string& prefix, const std::string& suffix)
	{
		plist_.addProperty(prefix, parseWidthInternal());
	}

	void PropertyParser::parseLength(const std::string& prefix, const std::string& suffix)
	{
		plist_.addProperty(prefix, Length::create(parseLengthInternal()));
	}

	void PropertyParser::parseWidthList(const std::string& prefix, const std::string& suffix)
	{
		StylePtr w1 = parseWidthInternal();
		skipWhitespace();
		if(isToken(TokenId::EOF_TOKEN) || isToken(TokenId::RBRACE) || isToken(TokenId::SEMICOLON) || isTokenDelimiter("!")) {
			plist_.addProperty(prefix + "-top", w1);
			plist_.addProperty(prefix + "-bottom", w1);
			plist_.addProperty(prefix + "-right", w1);
			plist_.addProperty(prefix + "-left", w1);
			return;
		}
		StylePtr w2 = parseWidthInternal();
		skipWhitespace();
		if(isToken(TokenId::EOF_TOKEN) || isToken(TokenId::RBRACE) || isToken(TokenId::SEMICOLON) || isTokenDelimiter("!")) {
			plist_.addProperty(prefix + "-top", w1);
			plist_.addProperty(prefix + "-bottom", w1);
			plist_.addProperty(prefix + "-right", w2);
			plist_.addProperty(prefix + "-left", w2);
			return;
		}
		StylePtr w3 = parseWidthInternal();
		skipWhitespace();
		if(isToken(TokenId::EOF_TOKEN) || isToken(TokenId::RBRACE) || isToken(TokenId::SEMICOLON) || isTokenDelimiter("!")) {
			plist_.addProperty(prefix + "-top", w1);
			plist_.addProperty(prefix + "-right", w2);
			plist_.addProperty(prefix + "-left", w2);
			plist_.addProperty(prefix + "-bottom", w3);
			return;
		}
		StylePtr w4 = parseWidthInternal();
		skipWhitespace();

		// four values, apply to individual elements.
		plist_.addProperty(prefix + "-top", w1);
		plist_.addProperty(prefix + "-right", w2);
		plist_.addProperty(prefix + "-bottom", w3);
		plist_.addProperty(prefix + "-left", w4);
	}

	void PropertyParser::parseLengthList(const std::string& prefix, const std::string& suffix)
	{
		StylePtr w1 = Length::create(parseLengthInternal());
		skipWhitespace();
		if(isToken(TokenId::EOF_TOKEN) || isToken(TokenId::RBRACE) || isToken(TokenId::SEMICOLON) || isTokenDelimiter("!")) {
			plist_.addProperty(prefix + "-top", w1);
			plist_.addProperty(prefix + "-bottom", w1);
			plist_.addProperty(prefix + "-right", w1);
			plist_.addProperty(prefix + "-left", w1);
			return;
		}
		StylePtr w2 = Length::create(parseLengthInternal());
		skipWhitespace();
		if(isToken(TokenId::EOF_TOKEN) || isToken(TokenId::RBRACE) || isToken(TokenId::SEMICOLON) || isTokenDelimiter("!")) {
			plist_.addProperty(prefix + "-top", w1);
			plist_.addProperty(prefix + "-bottom", w1);
			plist_.addProperty(prefix + "-right", w2);
			plist_.addProperty(prefix + "-left", w2);
			return;
		}
		StylePtr w3 = Length::create(parseLengthInternal());
		skipWhitespace();
		if(isToken(TokenId::EOF_TOKEN) || isToken(TokenId::RBRACE) || isToken(TokenId::SEMICOLON) || isTokenDelimiter("!")) {
			plist_.addProperty(prefix + "-top", w1);
			plist_.addProperty(prefix + "-right", w2);
			plist_.addProperty(prefix + "-left", w2);
			plist_.addProperty(prefix + "-bottom", w3);
			return;
		}
		StylePtr w4 = Length::create(parseLengthInternal());
		skipWhitespace();

		// four values, apply to individual elements.
		plist_.addProperty(prefix + "-top", w1);
		plist_.addProperty(prefix + "-right", w2);
		plist_.addProperty(prefix + "-bottom", w3);
		plist_.addProperty(prefix + "-left", w4);
	}

	void PropertyParser::parseBorderWidth(const std::string& prefix, const std::string& suffix)
	{
		plist_.addProperty(prefix, parseBorderWidthInternal());
	}

	void PropertyParser::parseBorderWidthList(const std::string& prefix, const std::string& suffix)
	{
		StylePtr w1 = parseBorderWidthInternal();
		skipWhitespace();
		if(isToken(TokenId::EOF_TOKEN) || isToken(TokenId::RBRACE) || isToken(TokenId::SEMICOLON) || isTokenDelimiter("!")) {
			plist_.addProperty(prefix + "-top-" + suffix, w1);
			plist_.addProperty(prefix + "-bottom-" + suffix, w1);
			plist_.addProperty(prefix + "-right-" + suffix, w1);
			plist_.addProperty(prefix + "-left-" + suffix, w1);
			return;
		}
		StylePtr w2 = parseBorderWidthInternal();
		skipWhitespace();
		if(isToken(TokenId::EOF_TOKEN) || isToken(TokenId::RBRACE) || isToken(TokenId::SEMICOLON) || isTokenDelimiter("!")) {
			plist_.addProperty(prefix + "-top-" + suffix, w1);
			plist_.addProperty(prefix + "-bottom-" + suffix, w1);
			plist_.addProperty(prefix + "-right-" + suffix, w2);
			plist_.addProperty(prefix + "-left-" + suffix, w2);
			return;
		}
		StylePtr w3 = parseBorderWidthInternal();
		skipWhitespace();
		if(isToken(TokenId::EOF_TOKEN) || isToken(TokenId::RBRACE) || isToken(TokenId::SEMICOLON) || isTokenDelimiter("!")) {
			plist_.addProperty(prefix + "-top-" + suffix, w1);
			plist_.addProperty(prefix + "-right-" + suffix, w2);
			plist_.addProperty(prefix + "-left-" + suffix, w2);
			plist_.addProperty(prefix + "-bottom-" + suffix, w3);
			return;
		}
		StylePtr w4 = parseBorderWidthInternal();
		skipWhitespace();

		// four values, apply to individual elements.
		plist_.addProperty(prefix + "-top-" + suffix, w1);
		plist_.addProperty(prefix + "-right-" + suffix, w2);
		plist_.addProperty(prefix + "-bottom-" + suffix, w3);
		plist_.addProperty(prefix + "-left-" + suffix, w4);	
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

	void PropertyParser::parseBorderStyle(const std::string& prefix, const std::string& suffix)
	{
		plist_.addProperty(prefix, parseBorderStyleInternal());
	}

	void PropertyParser::parseBorderStyleList(const std::string& prefix, const std::string& suffix)
	{
		StylePtr w1 = parseBorderStyleInternal();
		skipWhitespace();
		if(isToken(TokenId::EOF_TOKEN) || isToken(TokenId::RBRACE) || isToken(TokenId::SEMICOLON) || isTokenDelimiter("!")) {
			plist_.addProperty(prefix + "-top-" + suffix, w1);
			plist_.addProperty(prefix + "-bottom-" + suffix, w1);
			plist_.addProperty(prefix + "-right-" + suffix, w1);
			plist_.addProperty(prefix + "-left-" + suffix, w1);
			return;
		}
		StylePtr w2 = parseBorderStyleInternal();
		skipWhitespace();
		if(isToken(TokenId::EOF_TOKEN) || isToken(TokenId::RBRACE) || isToken(TokenId::SEMICOLON) || isTokenDelimiter("!")) {
			plist_.addProperty(prefix + "-top-" + suffix, w1);
			plist_.addProperty(prefix + "-bottom-" + suffix, w1);
			plist_.addProperty(prefix + "-right-" + suffix, w2);
			plist_.addProperty(prefix + "-left-" + suffix, w2);
			return;
		}
		StylePtr w3 = parseBorderStyleInternal();
		skipWhitespace();
		if(isToken(TokenId::EOF_TOKEN) || isToken(TokenId::RBRACE) || isToken(TokenId::SEMICOLON) || isTokenDelimiter("!")) {
			plist_.addProperty(prefix + "-top-" + suffix, w1);
			plist_.addProperty(prefix + "-right-" + suffix, w2);
			plist_.addProperty(prefix + "-left-" + suffix, w2);
			plist_.addProperty(prefix + "-bottom-" + suffix, w3);
			return;
		}
		StylePtr w4 = parseBorderStyleInternal();
		skipWhitespace();

		// four values, apply to individual elements.
		plist_.addProperty(prefix + "-top-" + suffix, w1);
		plist_.addProperty(prefix + "-right-" + suffix, w2);
		plist_.addProperty(prefix + "-bottom-" + suffix, w3);
		plist_.addProperty(prefix + "-left-" + suffix, w4);	
	}

	void PropertyParser::parseDisplay(const std::string& prefix, const std::string& suffix)
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
				plist_.addProperty(prefix, std::make_shared<Style>(true));
				return;
			} else {
				throw ParserError(formatter() << "Unrecognised token for display property: " << ref);
			}
		}
		plist_.addProperty(prefix, Display::create(display));
	}

	void PropertyParser::parseWhitespace(const std::string& prefix, const std::string& suffix)
	{
		CssWhitespace ws = CssWhitespace::NORMAL;
		if(isToken(TokenId::IDENT)) {
			std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "normal") {
				ws = CssWhitespace::NORMAL;
			} else if(ref == "inherit") {
				plist_.addProperty(prefix, std::make_shared<Style>(true));
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
			throw ParserError(formatter() << "Expected identifier for property: " << prefix << " found " << Token::tokenIdToString((*it_)->id()));
		}
		plist_.addProperty(prefix, Whitespace::create(ws));
	}

	void PropertyParser::parseFontFamily(const std::string& prefix, const std::string& suffix)
	{
		std::vector<std::string> font_list;
		parseCSVStringList(TokenId::DELIM, [&font_list](int n, const std::string& str) {
			font_list.emplace_back(str);
		});
		plist_.addProperty(prefix, FontFamily::create(font_list));
	}
	
	void PropertyParser::parseFontSize(const std::string& prefix, const std::string& suffix)
	{
		FontSize fs;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(prefix, std::make_shared<Style>(true));
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
				throw ParserError(formatter() << "Unrecognised identifier for '" << prefix << "' property: " << ref);
			}
		} else if(isToken(TokenId::DIMENSION)) {
			const std::string units = (*it_)->getStringValue();
			xhtml::FixedPoint value = static_cast<xhtml::FixedPoint>((*it_)->getNumericValue() * fixed_point_scale);
			advance();
			fs.setFontSize(Length(value, units));
		} else if(isToken(TokenId::PERCENT)) {
			xhtml::FixedPoint d = static_cast<xhtml::FixedPoint>((*it_)->getNumericValue() * fixed_point_scale);
			advance();
			fs.setFontSize(Length(d, true));
		} else if(isToken(TokenId::NUMBER)) {
			xhtml::FixedPoint d = static_cast<xhtml::FixedPoint>((*it_)->getNumericValue() * fixed_point_scale);
			advance();
			fs.setFontSize(Length(d));
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << prefix << "': "  << (*it_)->toString());
		}		
		plist_.addProperty(prefix, FontSize::create(fs));
	}

	void PropertyParser::parseFontWeight(const std::string& prefix, const std::string& suffix)
	{
		FontWeight fw;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(prefix, std::make_shared<Style>(true));
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
				throw ParserError(formatter() << "Unrecognised identifier for '" << prefix << "' property: " << ref);
			}
		} else if(isToken(TokenId::NUMBER)) {
			fw.setWeight(static_cast<int>((*it_)->getNumericValue()));
			advance();
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << prefix << "': "  << (*it_)->toString());
		}
		plist_.addProperty(prefix, FontWeight::create(fw));
	}

	void PropertyParser::parseSpacing(const std::string& prefix, const std::string& suffix)
	{
		Length spacing;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(prefix, std::make_shared<Style>(true));
				return;
			} else if(ref == "normal") {
				// spacing defaults to 0
			} else {
				throw ParserError(formatter() << "Unrecognised identifier for '" << prefix << "' property: " << ref);
			}
		} else if(isToken(TokenId::DIMENSION)) {
			const std::string units = (*it_)->getStringValue();
			xhtml::FixedPoint value = static_cast<xhtml::FixedPoint>((*it_)->getNumericValue() * fixed_point_scale);
			advance();
			spacing = Length(value, units);
		} else if(isToken(TokenId::NUMBER)) {
			xhtml::FixedPoint d = static_cast<xhtml::FixedPoint>((*it_)->getNumericValue() * fixed_point_scale);
			advance();
			spacing = Length(d);
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << prefix << "': "  << (*it_)->toString());
		}
		plist_.addProperty(prefix, Length::create(spacing));
	}

	void PropertyParser::parseTextAlign(const std::string& prefix, const std::string& suffix)
	{
		CssTextAlign ta = CssTextAlign::NORMAL;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(prefix, std::make_shared<Style>(true));
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
				throw ParserError(formatter() << "Unrecognised identifier for '" << prefix << "' property: " << ref);
			}
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << prefix << "': "  << (*it_)->toString());
		}
		plist_.addProperty(prefix, TextAlign::create(ta));
	}

	void PropertyParser::parseDirection(const std::string& prefix, const std::string& suffix)
	{
		CssDirection dir = CssDirection::LTR;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(prefix, std::make_shared<Style>(true));
				return;
			} else if(ref == "ltr") {
				dir = CssDirection::LTR;
			} else if(ref == "rtl") {
				dir = CssDirection::RTL;
			} else {
				throw ParserError(formatter() << "Unrecognised identifier for '" << prefix << "' property: " << ref);
			}
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << prefix << "': "  << (*it_)->toString());
		}
		plist_.addProperty(prefix, Direction::create(dir));
	}

	void PropertyParser::parseTextTransform(const std::string& prefix, const std::string& suffix)
	{
		CssTextTransform tt = CssTextTransform::NONE;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(prefix, std::make_shared<Style>(true));
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
				throw ParserError(formatter() << "Unrecognised identifier for '" << prefix << "' property: " << ref);
			}
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << prefix << "': "  << (*it_)->toString());
		}
		plist_.addProperty(prefix, TextTransform::create(tt));
	}

	void PropertyParser::parseLineHeight(const std::string& prefix, const std::string& suffix)
	{
		Length lh(static_cast<xhtml::FixedPoint>(1.1f * fixed_point_scale));
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(prefix, std::make_shared<Style>(true));
				return;
			} else if(ref == "normal") {
				// do nothing as already set in default
			} else {
				throw ParserError(formatter() << "Unrecognised identifier for '" << prefix << "' property: " << ref);
			}
		} else {
			lh = parseLengthInternal(NUMERIC);
		}
		plist_.addProperty(prefix, Length::create(lh));
	}

	void PropertyParser::parseFontStyle(const std::string& prefix, const std::string& suffix)
	{
		CssFontStyle fs = CssFontStyle::NORMAL;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(prefix, std::make_shared<Style>(true));
				return;
			} else if(ref == "italic") {
				fs = CssFontStyle::ITALIC;
			} else if(ref == "normal") {
				fs = CssFontStyle::NORMAL;
			} else if(ref == "oblique") {
				fs = CssFontStyle::OBLIQUE;
			} else {
				throw ParserError(formatter() << "Unrecognised identifier for '" << prefix << "' property: " << ref);
			}
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << prefix << "': "  << (*it_)->toString());
		}
		plist_.addProperty(prefix, FontStyle::create(fs));
	}

	void PropertyParser::parseFontVariant(const std::string& prefix, const std::string& suffix)
	{
		CssFontVariant fv = CssFontVariant::NORMAL;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(prefix, std::make_shared<Style>(true));
				return;
			} else if(ref == "normal") {
				fv = CssFontVariant::NORMAL;
			} else if(ref == "small-caps") {
				fv = CssFontVariant::SMALL_CAPS;
			} else {
				throw ParserError(formatter() << "Unrecognised identifier for '" << prefix << "' property: " << ref);
			}
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << prefix << "': "  << (*it_)->toString());
		}
		plist_.addProperty(prefix, FontVariant::create(fv));
	}

	void PropertyParser::parseOverflow(const std::string& prefix, const std::string& suffix)
	{
		CssOverflow of = CssOverflow::VISIBLE;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(prefix, std::make_shared<Style>(true));
				return;
			} else if(ref == "visible") {
				of = CssOverflow::VISIBLE;
			} else if(ref == "hidden") {
				of = CssOverflow::HIDDEN;
			} else if(ref == "scroll") {
				of = CssOverflow::SCROLL;
			} else if(ref == "clip") {
				of = CssOverflow::CLIP;
			} else if(ref == "auto") {
				of = CssOverflow::AUTO;
			} else {
				throw ParserError(formatter() << "Unrecognised identifier for '" << prefix << "' property: " << ref);
			}
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << prefix << "': "  << (*it_)->toString());
		}
		plist_.addProperty(prefix, Overflow::create(of));
	}

	void PropertyParser::parsePosition(const std::string& prefix, const std::string& suffix)
	{
		CssPosition p = CssPosition::STATIC;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(prefix, std::make_shared<Style>(true));
				return;
			} else if(ref == "satic") {
				p = CssPosition::STATIC;
			} else if(ref == "absolute") {
				p = CssPosition::ABSOLUTE;
			} else if(ref == "relative") {
				p = CssPosition::RELATIVE;
			} else if(ref == "fixed") {
				p = CssPosition::FIXED;
			} else {
				throw ParserError(formatter() << "Unrecognised identifier for '" << prefix << "' property: " << ref);
			}
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << prefix << "': "  << (*it_)->toString());
		}
		plist_.addProperty(prefix, Position::create(p));
	}

	void PropertyParser::parseFloat(const std::string& prefix, const std::string& suffix)
	{
		CssFloat p = CssFloat::NONE;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(prefix, std::make_shared<Style>(true));
				return;
			} else if(ref == "none") {
				p = CssFloat::NONE;
			} else if(ref == "left") {
				p = CssFloat::LEFT;
			} else if(ref == "right") {
				p = CssFloat::RIGHT;
			} else {
				throw ParserError(formatter() << "Unrecognised identifier for '" << prefix << "' property: " << ref);
			}
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << prefix << "': "  << (*it_)->toString());
		}
		plist_.addProperty(prefix, Float::create(p));
	}

	void PropertyParser::parseUri(const std::string& prefix, const std::string& suffix)
	{
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(prefix, std::make_shared<Style>(true));
				return;
			} else if(ref == "none") {
				plist_.addProperty(prefix, std::make_shared<UriStyle>(true));
			} else {
				throw ParserError(formatter() << "Unrecognised identifier for '" << prefix << "' property: " << ref);
			}
		} else if(isToken(TokenId::URL)) {
			const std::string uri = (*it_)->getStringValue();
			advance();
			plist_.addProperty(prefix, UriStyle::create(uri));
			// XXX here would be a good place to place to have a background thread working
			// to look up the uri and download it. or look-up in cache, etc.
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << prefix << "': "  << (*it_)->toString());
		}
	}

	void PropertyParser::parseBackgroundRepeat(const std::string& prefix, const std::string& suffix)
	{
		CssBackgroundRepeat repeat = CssBackgroundRepeat::REPEAT;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(prefix, std::make_shared<Style>(true));
				return;
			} else if(ref == "repeat") {
				repeat = CssBackgroundRepeat::REPEAT;
			} else if(ref == "repeat-x") {
				repeat = CssBackgroundRepeat::REPEAT_X;
			} else if(ref == "repeat-y") {
				repeat = CssBackgroundRepeat::REPEAT_Y;
			} else if(ref == "no-repeat") {
				repeat = CssBackgroundRepeat::NO_REPEAT;
			} else {
				throw ParserError(formatter() << "Unrecognised identifier for '" << prefix << "' property: " << ref);
			}
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << prefix << "': "  << (*it_)->toString());
		}
		plist_.addProperty(prefix, BackgroundRepeat::create(repeat));
	}

	void PropertyParser::parseBackgroundPosition(const std::string& prefix, const std::string& suffix)
	{
		/// need to check this works.
		// this is slightly complicated by the fact that "top center" and "center top" are valid.
		// as is "0% top" and "top 0%" whereas "50% 25%" is fixed for left then top.
		auto pos = BackgroundPosition::create();
		bool was_horiz_set = false;
		bool was_vert_set = false;
		int cnt = 2;
		std::vector<Length> holder;
		while(cnt-- > 0) {
			if(isToken(TokenId::IDENT)) {
				const std::string ref = (*it_)->getStringValue();
				advance();
				if(ref == "inherit") {
					plist_.addProperty(prefix, std::make_shared<Style>(true));
					return;
				} else if(ref == "left") {
					pos->setLeft(Length(0, true));
					was_horiz_set = true; 
				} else if(ref == "top") {
					pos->setTop(Length(0, true));
					was_vert_set = true;
				} else if(ref == "right") {
					pos->setLeft(Length(100 * fixed_point_scale, true));
					was_horiz_set = true; 
				} else if(ref == "bottom") {
					pos->setTop(Length(100 * fixed_point_scale, true));
					was_vert_set = true;
				} else if(ref == "center") {
					holder.emplace_back(50 * fixed_point_scale, true);
				} else {
					throw ParserError(formatter() << "Unrecognised identifier for '" << prefix << "' property: " << ref);
				}
			} else if(isToken(TokenId::DIMENSION)) {
				const std::string units = (*it_)->getStringValue();
				xhtml::FixedPoint value = static_cast<xhtml::FixedPoint>((*it_)->getNumericValue() * fixed_point_scale);
				advance();
				holder.emplace_back(value, units);
			} else if(isToken(TokenId::PERCENT)) {
				xhtml::FixedPoint d = static_cast<xhtml::FixedPoint>((*it_)->getNumericValue() * fixed_point_scale);
				advance();
				holder.emplace_back(d, true);
			} else if(cnt > 0) {
				throw ParserError(formatter() << "Unrecognised value for property '" << prefix << "': "  << (*it_)->toString());
			}
			
			skipWhitespace();
		}
		if(was_horiz_set && !was_vert_set) {
			if(holder.size() > 0) {
				// we set something so apply it.
				pos->setTop(holder.front());
			} else {
				// apply a default, center
				pos->setTop(Length(50, true));
			}
		}
		if(was_vert_set && !was_horiz_set) {
			if(holder.size() > 0) {
				// we set something so apply it.
				pos->setLeft(holder.front());
			} else {
				// apply a default, center
				pos->setLeft(Length(50, true));
			}
		}
		if(!was_horiz_set && !was_vert_set) {
			// assume left top ordering.
			if(holder.size() > 1) {
				pos->setLeft(holder[0]);
				pos->setTop(holder[1]);
			} else if(holder.size() > 0) {
				pos->setLeft(holder.front());
				pos->setTop(holder.front());
			} else {
				pos->setLeft(Length(0, true));
				pos->setTop(Length(0, true));
			}
		}
		plist_.addProperty(prefix, pos);
	}

	void PropertyParser::parseListStyleType(const std::string& prefix, const std::string& suffix)
	{
		CssListStyleType lst = CssListStyleType::DISC;
		if(isToken(TokenId::IDENT)) {
			const std::string ref = (*it_)->getStringValue();
			advance();
			if(ref == "inherit") {
				plist_.addProperty(prefix, std::make_shared<Style>(true));
				return;
			} else if(ref == "none") {
				lst = CssListStyleType::NONE;
			} else if(ref == "disc") {
				lst = CssListStyleType::DISC;
			} else if(ref == "circle") {
				lst = CssListStyleType::CIRCLE;
			} else if(ref == "square") {
				lst = CssListStyleType::SQUARE;
			} else if(ref == "decimal") {
				lst = CssListStyleType::DECIMAL;
			} else if(ref == "decimal-leading-zero") {
				lst = CssListStyleType::DECIMAL_LEADING_ZERO;
			} else if(ref == "lower-roman") {
				lst = CssListStyleType::LOWER_ROMAN;
			} else if(ref == "upper-roman") {
				lst = CssListStyleType::UPPER_ROMAN;
			} else if(ref == "lower-greek") {
				lst = CssListStyleType::LOWER_GREEK;
			} else if(ref == "lower-latin") {
				lst = CssListStyleType::LOWER_LATIN;
			} else if(ref == "upper-latin") {
				lst = CssListStyleType::UPPER_LATIN;
			} else if(ref == "armenian") {
				lst = CssListStyleType::ARMENIAN;
			} else if(ref == "georgian") {
				lst = CssListStyleType::GEORGIAN;
			} else if(ref == "lower-alpha") {
				lst = CssListStyleType::LOWER_ALPHA;
			} else if(ref == "upper-alpha") {
				lst = CssListStyleType::UPPER_ALPHA;
			} else {
				throw ParserError(formatter() << "Unrecognised identifier for '" << prefix << "' property: " << ref);
			}
		} else {
			throw ParserError(formatter() << "Unrecognised value for property '" << prefix << "': "  << (*it_)->toString());
		}
		plist_.addProperty(prefix, ListStyleType::create(lst));
	}
}
