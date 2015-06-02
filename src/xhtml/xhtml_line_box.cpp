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

#include "xhtml_line_box.hpp"
#include "xhtml_layout_engine.hpp"
#include "solid_renderable.hpp"

namespace xhtml
{
	LineBox::LineBox(BoxPtr parent, NodePtr node)
		: Box(BoxId::LINE, parent, node)
	{
	}

	std::string LineBox::toString() const
	{
		std::ostringstream ss;
		ss << "LineBox: " << getDimensions().content_;
		return ss.str();
	}

	void LineBox::handleLayout(LayoutEngine& eng, const Dimensions& containing)
	{
		FixedPoint max_height = 0;
		FixedPoint width = !getChildren().empty() 
			? getChildren().back()->getDimensions().content_.width +  getChildren().back()->getDimensions().content_.x + getChildren().back()->getMBPRight()
			: containing.content_.width;
		for(auto& child : getChildren()) {
			max_height = std::max(max_height, child->getDimensions().content_.height);			
		}
		setContentHeight(max_height);
		setContentWidth(width);
	}

	void LineBox::handleRender(DisplayListPtr display_list, const point& offset) const
	{
		// do nothing
	}

	void LineBox::handleRenderBorder(DisplayListPtr display_list, const point& offset) const
	{
		// add a debug background, around content.
		/*rect r(getDimensions().content_.x + offset.x, 
			getDimensions().content_.y + offset.y, 
			getDimensions().content_.width, 
			getDimensions().content_.height);
		auto sr = std::make_shared<SolidRenderable>(r, KRE::Color::colorSlateblue());
		display_list->addRenderable(sr);*/
	}
}
