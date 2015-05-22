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

#pragma once

#include <functional>

#include "css_styles.hpp"
#include "css_lexer.hpp"
#include "variant_object.hpp"

namespace css
{
	struct PropertyInfo
	{
		PropertyInfo() : name(), inherited(false), obj(), is_default(false) {}
		PropertyInfo(const std::string& n, bool inh, Object def) : name(n), inherited(inh), obj(def), is_default(false) {}
		std::string name;
		bool inherited;
		Object obj;
		bool is_default;
	};

	class PropertyList
	{
	public:
		struct PropertyStyle {
			PropertyStyle() : style(nullptr), specificity() {}
			explicit PropertyStyle(StylePtr s, const Specificity& sp) : style(s), specificity(sp) {}
			StylePtr style;
			Specificity specificity;
		};

		typedef std::map<Property, PropertyStyle>::iterator iterator;
		typedef std::map<Property, PropertyStyle>::const_iterator const_iterator;
		PropertyList();
		void addProperty(Property p, StylePtr o, const Specificity& specificity=Specificity());
		void addProperty(const std::string& name, StylePtr o);
		StylePtr getProperty(Property p) const;
		bool hasProperty(Property p) const { return properties_.find(p) != properties_.end(); }
		void merge(const Specificity& specificity, const PropertyList& plist);
		void clear() { properties_.clear(); }
		iterator begin() { return properties_.begin(); }
		iterator end() { return properties_.end(); }
		const_iterator begin() const { return properties_.cbegin(); }
		const_iterator end() const { return properties_.cend(); }
	private:
		std::map<Property, PropertyStyle> properties_;
	};

	class PropertyParser
	{
	public:
		typedef std::vector<TokenPtr>::const_iterator const_iterator;
		PropertyParser();
		const_iterator parse(const std::string& name, const const_iterator& begin, const const_iterator& end);
		const PropertyList& getPropertyList() const { return plist_; }
		PropertyList& getPropertyList() { return plist_; }
		typedef std::vector<TokenPtr>::const_iterator const_iterator;
		void parseColor(const std::string& name);
		void parseWidth(const std::string& name);
		void parseLength(const std::string& name);
		void parseWidthList(const std::string& name);
		void parseLengthList(const std::string& name);
		void parseBorderWidth(const std::string& name);
		void parseBorderStyle(const std::string& name);
		void parseDisplay(const std::string& name);
		void parseWhitespace(const std::string& name);
		void parseFontFamily(const std::string& name);
		void parseFontSize(const std::string& name);
		void parseFontWeight(const std::string& name);
		void parseSpacing(const std::string& name);
		void parseTextAlign(const std::string& name);
		void parseDirection(const std::string& name);
		void parseTextTransform(const std::string& name);
		void parseLineHeight(const std::string& name);
		void parseFontStyle(const std::string& name);
		void parseFontVariant(const std::string& name);
		void parseOverflow(const std::string& name);
		void parsePosition(const std::string& name);
		void parseFloat(const std::string& name);
		void parseUri(const std::string& name);
		void parseBackgroundRepeat(const std::string& name);
		void parseBackgroundPosition(const std::string& name);
	private:
		enum NumericParseOptions {
			NUMBER = 1,
			PERCENTAGE = 2,
			LENGTH = 4,
			AUTO = 8,
			NUMERIC = NUMBER | PERCENTAGE | LENGTH,
			ALL = NUMBER | PERCENTAGE | LENGTH | AUTO,
		};
		void advance();
		void skipWhitespace();
		bool isToken(TokenId tok) const;
		bool isTokenDelimiter(const std::string& delim);
		std::vector<TokenPtr> PropertyParser::parseCSVList(TokenId end_token);
		void parseCSVNumberListFromIt(std::vector<TokenPtr>::const_iterator start, std::vector<TokenPtr>::const_iterator end, std::function<void(int, float, bool)> fn);
		void parseCSVNumberList(TokenId end_token, std::function<void(int, float, bool)> fn);
		void parseCSVStringList(TokenId end_token, std::function<void(int, const std::string&)> fn);
		StylePtr parseColorInternal();
		Length parseLengthInternal(NumericParseOptions opts=ALL);
		StylePtr parseWidthInternal();
		StylePtr parseBorderWidthInternal();
		StylePtr parseBorderStyleInternal();

		const_iterator it_;
		const_iterator end_;
		PropertyList plist_;
	};

	const std::string& get_property_name(Property p);
	const PropertyInfo& get_default_property_info(Property p);
}
