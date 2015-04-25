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
#include "xhtml_text_node.hpp"
#include "xhtml_render_ctx.hpp"

namespace xhtml
{
	namespace
	{
		struct TextImpl : public Text
		{
			TextImpl(const std::string& txt, WeakDocumentPtr owner) : Text(txt, owner) {}
		};
	}

	Text::Text(const std::string& txt, WeakDocumentPtr owner)
		: Node(NodeId::TEXT, owner),
		  text_(txt)
	{
	}

	TextPtr Text::create(const std::string& txt, WeakDocumentPtr owner)
	{
		return std::make_shared<TextImpl>(txt, owner);
	}

	std::string Text::toString() const 
	{
		std::ostringstream ss;
		ss << "Text('" << text_ << "' " << nodeToString() << ")";
		return ss.str();
	}

	const std::string& Text::generateLine(int maximum_line_width, int offset, int* line_width, int* line_offset)
	{
		auto parent = getParent();
		ASSERT_LOG(parent != nullptr, "No parent for this text node data '" << text_ << "' can't generate line");
		auto ff = parent->getStyle("font-family").getValue<std::vector<std::string>>();
	}

}
