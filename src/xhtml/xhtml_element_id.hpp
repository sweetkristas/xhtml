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

#include <string>

namespace xhtml
{
	enum class ElementId {
		// special element id to match anything "*"
		ANY,
		// special element for text
		XMLTEXT,
		// Normal tokens
		HTML,
		HEAD,
		BODY,
		SCRIPT,
		P,
		ABBR,
		BR,
		EM,
		IMG,
		OBJECT,
		STYLE,
		TITLE,
		LINK,
		META,
		BASE,
		FORM,
		SELECT,
		OPTGROUP,
		OPTION,
		INPUT,
		TEXTAREA,
		BUTTON,
		LABEL,
		FIELDSET,
		LEGEND,
		UL,
		OL,
		DL,
		DIR,
		MENU,
		LI,
		DIV,
		H1,
		H2,
		H3,
		H4,
		H5,
		H6,
		H7,
		H8,
		H9,
		H10,
		H11,
		H12,
		H13,
		H14,
		H15,
		H16,
		Q,
		PRE,
		BLOCKQUOTE,
		HR,
		MOD,
		A,
		PARAM,
		APPLET,
		MAP,
		AREA,
		TABLE,
		CAPTION,
		COL,
		COLGROUP,
		THEAD,
		TFOOT,
		TBODY,
		TR,
		TD,
		FRAMESET,
		FRAME,
		IFRAME,
	};

	ElementId string_to_element_id(const std::string& e);
	const std::string& element_id_to_string(ElementId id);
}
