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

#include "solid_renderable.hpp"
#include "xhtml_line_box.hpp"
#include "xhtml_layout_engine.hpp"
#include "xhtml_text_box.hpp"

namespace xhtml
{
	LineBox::LineBox(BoxPtr parent, const point& cursor)
		: Box(BoxId::LINE, parent, nullptr),
		  starting_x_(0),
		  cursor_(cursor)
	{
	}

	std::string LineBox::toString() const
	{
		std::ostringstream ss;
		ss << "LineBox: " << getDimensions().content_;
		return ss.str();
	}

	void LineBox::reflowChildren(LayoutEngine& eng, const Dimensions& containing)
	{
		FixedPoint lh = eng.getLineHeight();
		FixedPoint width = eng.getWidthAtPosition(cursor_.y + eng.getOffset().y, containing.content_.width) - cursor_.x;

		auto children = getChildren();
		clearChildren();
		for(auto& child : children) {
			if(child->id() == BoxId::TEXT) {
				auto txt = std::dynamic_pointer_cast<TextBox>(child);
				ASSERT_LOG(txt != nullptr, "Something went wrong child box with id TEXT couldn't be cast to Text box");
				TextPtr tnode = txt->getText();
				auto it = tnode->begin();

				while(it != tnode->end()) {
					it = txt->reflow(eng, cursor_, it);

					LinePtr line = txt->getLine();
					if(line != nullptr && !line->line.empty()) {
						txt->setContentX(cursor_.x);
						txt->setContentY(cursor_.y);

						FixedPoint x_inc = txt->getWidth() + txt->getMBPWidth();
						cursor_.x += x_inc;
						width -= x_inc;
						addChild(txt);
						txt.reset();
					}
			
					if((line != nullptr && line->is_end_line) || width < 0) {
						cursor_.y += lh;
						cursor_.x = 0;
						width = eng.getWidthAtPosition(cursor_.y + eng.getOffset().y, containing.content_.width);
					}

					if(it != tnode->end()) {
						txt = std::make_shared<TextBox>(shared_from_this(), tnode);
					}
				}
			} else {
				// XXX fixme
				addChild(child);
			}
		}
	}

	void LineBox::handlePreChildLayout(LayoutEngine& eng, const Dimensions& containing)
	{
	}

	void LineBox::handleLayout(LayoutEngine& eng, const Dimensions& containing)
	{
		reflowChildren(eng, containing);

		// Our children should already be set at this point.
		// we want to compute our own width/height based on our children and set the 
		// children's x/y
		FixedPoint height = 0;
		FixedPoint width = 0;

		// compute our width/height
		for(auto& child : getChildren()) {
			//child->setContentX(width + starting_x_);

			height = std::max(height, child->getHeight() + child->getMBPHeight());
			width += child->getWidth() + getMBPWidth();
		}
		
		setContentWidth(width);
		setContentHeight(height);

		// computer&set children X/Y offsets
		for(auto& child : getChildren()) {
			FixedPoint child_y = 0;
			// XXX we should implement this fully.
			switch(child->getVerticalAlign()) {
				case css::CssVerticalAlign::BASELINE:
					// Align the baseline of the box with the baseline of the parent box. 
					// If the box does not have a baseline, align the bottom margin edge 
					// with the parent's baseline.
					child_y = child->getBaselineOffset();
					break;
				case css::CssVerticalAlign::MIDDLE:
					// Align the vertical midpoint of the box with the baseline of the 
					// parent box plus half the x-height of the parent.
					child_y = height / 2;
					break;
				case css::CssVerticalAlign::BOTTOM:
					// Align the bottom of the aligned subtree with the bottom of the line box.
					child_y = child->getBottomOffset();
					break;
				case css::CssVerticalAlign::SUB:
					// Lower the baseline of the box to the proper position for subscripts of the 
					// parent's box. (This value has no effect on the font size of the element's text.)
				case css::CssVerticalAlign::SUPER:
					// Raise the baseline of the box to the proper position for superscripts of the 
					// parent's box. (This value has no effect on the font size of the element's text.)
				case css::CssVerticalAlign::TOP:
					// Align the top of the aligned subtree with the top of the line box.
				case css::CssVerticalAlign::TEXT_TOP:
					// Align the top of the box with the top of the parent's content area
				case css::CssVerticalAlign::TEXT_BOTTOM:
					// Align the bottom of the box with the bottom of the parent's content area
				case css::CssVerticalAlign::LENGTH:
				default:  break;
			}
			//child->setContentY(child_y);
		}
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
