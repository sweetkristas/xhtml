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
	enum class CssProperty {
		BACKGROUND_COLOR,
		BACKGROUND_ATTACHMENT,
		COLOR,
		BORDER_TOP_COLOR,
		BORDER_TOP_STYLE,
		BORDER_TOP_WIDTH,
		BORDER_BOTTOM_COLOR,
		BORDER_BOTTOM_STYLE,
		BORDER_BOTTOM_WIDTH,
		BORDER_LEFT_COLOR,
		BORDER_LEFT_STYLE,
		BORDER_LEFT_WIDTH,
		BORDER_RIGHT_COLOR,
		BORDER_RIGHT_STYLE,
		BORDER_RIGHT_WIDTH,

		MARGIN_TOP,
		MARGIN_BOTTOM,
		MARGIN_LEFT,
		MARGIN_RIGHT,

		PADDING_TOP,
		PADDING_BOTTOM,
		PADDING_LEFT,
		PADDING_RIGHT,

		DISPLAY,
	};
	typedef std::map<CssProperty, Object> PropertyList;

	typedef std::function<void(std::vector<TokenPtr>::const_iterator&, std::vector<TokenPtr>::const_iterator, PropertyList*)> property_parse_function;
	property_parse_function find_property_handler(const std::string& name);

	void apply_properties_to_css(CssStyles* attr, const PropertyList& plist);

	const std::string& get_property_name_from_id(CssProperty prop);
}
