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
	class PropertyList
	{
	public:
		typedef std::map<std::string, Object>::iterator iterator;
		typedef std::map<std::string, Object>::const_iterator const_iterator;
		PropertyList();
		void addProperty(const std::string& name, const Object& o);
		Object getProperty(const std::string& name) const;
		void merge(const PropertyList& plist);
		iterator begin() { return properties_.begin(); }
		iterator end() { return properties_.end(); }
		const_iterator begin() const { return properties_.cbegin(); }
		const_iterator end() const { return properties_.cend(); }
	private:
		std::map<std::string, Object> properties_;
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
		void parseWidthList(const std::string& name);
		void parseBorderWidth(const std::string& name);
		void parseBorderStyle(const std::string& name);
		void parseDisplay(const std::string& name);
		void parseWhitespace(const std::string& name);
		void parseFontFamily(const std::string& name);
	private:
		void advance();
		void skipWhitespace();
		bool isToken(TokenId tok) const;
		bool isTokenDelimiter(const std::string& delim);
		std::vector<TokenPtr> PropertyParser::parseCSVList(TokenId end_token);
		void parseCSVNumberList(TokenId end_token, std::function<void(int,double,bool)> fn);
		void parseCSVStringList(TokenId end_token, std::function<void(int, const std::string&)> fn);
		Object parseColorInternal();
		Object parseWidthInternal();
		Object parseBorderWidthInternal();
		Object parseBorderStyleInternal();

		const_iterator it_;
		const_iterator end_;
		PropertyList plist_;
	};
}
