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

#include "xhtml_block_box.hpp"
#include "xhtml_layout_engine.hpp"

namespace xhtml
{
	using namespace css;

	BlockBox::BlockBox(BoxPtr parent, NodePtr node, NodePtr child)
		: Box(BoxId::BLOCK, parent, node),
		  child_height_(0),
		  child_(child)
	{
	}

	std::string BlockBox::toString() const
	{
		std::ostringstream ss;
		ss << "BlockBox: " << getDimensions().content_ << (isFloat() ? " floating" : "");
		return ss.str();
	}

	void BlockBox::handleLayout(LayoutEngine& eng, const Dimensions& containing, const FloatList& floats)
	{
		NodePtr node = getNode();
		bool is_replaced = false;
		if(node != nullptr && node->id() == NodeId::ELEMENT) {
			is_replaced = node->isReplaced();
		}

		if(is_replaced) {
			layoutChildren(eng);
		} else {
			layoutChildren(eng);
			layoutHeight(containing);
		}
	}

	std::vector<NodePtr> BlockBox::getChildNodes() const
	{
		NodePtr child = child_.lock();
		if(child != nullptr) {
			return std::vector<NodePtr>(1, child);
		}
		return Box::getChildNodes();
	}

	void BlockBox::handlePreChildLayout(LayoutEngine& eng, const Dimensions& containing, const FloatList& floats)
	{
		NodePtr node = getNode();
		if(node != nullptr && node->id() == NodeId::ELEMENT && node->isReplaced()) {
			calculateHorzMPB(containing.content_.width);
			setContentRect(Rect(0, 0, node->getDimensions().w() * LayoutEngine::getFixedPointScale(), node->getDimensions().h() * LayoutEngine::getFixedPointScale()));
			auto css_width = getCssWidth();
			auto css_height = getCssHeight();
			if(!css_width.isAuto()) {
				setContentWidth(css_width.getLength().compute(containing.content_.width));
			}
			if(!css_height.isAuto()) {
				setContentHeight(css_height.getLength().compute(containing.content_.height));
			}
			if(!css_width.isAuto() || !css_height.isAuto()) {
				node->setDimensions(rect(0, 0, getDimensions().content_.width/LayoutEngine::getFixedPointScale(), getDimensions().content_.height/LayoutEngine::getFixedPointScale()));
			}
		} else {
			layoutWidth(containing);
		}

		calculateVertMPB(containing.content_.height);

		FixedPoint left = 0;
		FixedPoint top = 0;
		if(getPosition() == CssPosition::FIXED) {
			const FixedPoint containing_width = containing.content_.width;
			const FixedPoint containing_height = containing.content_.height;
			left = containing.content_.x;
			if(!getCssLeft().isAuto()) {
				left = getCssLeft().getLength().compute(containing_width);
			}
			top = containing.content_.y;
			if(!getCssTop().isAuto()) {
				top = getCssTop().getLength().compute(containing_height);
			}
		} else if(isFloat()) {
			const FixedPoint lh = getLineHeight();
			const FixedPoint box_w = getDimensions().content_.width;

			FixedPoint y = 0;
			FixedPoint x = 0;

			FixedPoint y1 = y + eng.getOffset().y;
			left = getFloatValue() == CssFloat::LEFT ? eng.getXAtPosition(y1, y1 + lh, floats) + x : eng.getX2AtPosition(y1, y1 + lh, floats);
			FixedPoint w = eng.getWidthAtPosition(y1, y1 + lh, containing.content_.width, floats);
			bool placed = false;
			while(!placed) {
				if(w >= box_w) {
					left = left - (getFloatValue() == CssFloat::LEFT ? x : box_w);
					top = y;
					placed = true;
				} else {
					y += lh;
					y1 = y + eng.getOffset().y;
					left = getFloatValue() == CssFloat::LEFT ? eng.getXAtPosition(y1, y1 + lh, floats) + x : eng.getX2AtPosition(y1, y1 + lh, floats);
					w = eng.getWidthAtPosition(y1, y1 + lh, containing.content_.width, floats);
				}
			}
		}
	
		setContentX(left + getMBPLeft());
		setContentY(top + getMBPTop() + containing.content_.height);
	}

	void BlockBox::handlePostChildLayout(LayoutEngine& eng, BoxPtr child)
	{
		// Called after every child is laid out.
		setContentHeight(child->getTop() + child->getHeight() + child->getMBPBottom());
	}

	void BlockBox::layoutWidth(const Dimensions& containing)
	{
		RenderContext& ctx = RenderContext::get();
		const FixedPoint containing_width = containing.content_.width;

		const auto& css_width = getCssWidth();
		FixedPoint width = 0;
		if(!css_width.isAuto()) {
			width = css_width.getLength().compute(containing_width);
			setContentWidth(width);
		}

		calculateHorzMPB(containing_width);
		const auto& css_margin_left = getCssMargin(Side::LEFT);
		const auto& css_margin_right = getCssMargin(Side::RIGHT);

		FixedPoint total = getMBPWidth() + width;
			
		if(!css_width.isAuto() && total > containing.content_.width) {
			if(css_margin_left.isAuto()) {
				setMarginLeft(0);
			}
			if(css_margin_right.isAuto()) {
				setMarginRight(0);
			}
		}

		// If negative is overflow.
		FixedPoint underflow = containing.content_.width - total;

		if(css_width.isAuto()) {
			if(css_margin_left.isAuto()) {
				setMarginLeft(0);
			}
			if(css_margin_right.isAuto()) {
				setMarginRight(0);
			}
			if(underflow >= 0) {
				width = underflow;
			} else {
				width = 0;
				const auto rmargin = css_margin_right.getLength().compute(containing_width);
				setMarginRight(rmargin + underflow);
			}
			setContentWidth(width);
		} else if(!css_margin_left.isAuto() && !css_margin_right.isAuto()) {
			setMarginRight(getDimensions().margin_.right + underflow);
		} else if(!css_margin_left.isAuto() && css_margin_right.isAuto()) {
			setMarginRight(underflow);
		} else if(css_margin_left.isAuto() && !css_margin_right.isAuto()) {
			setMarginLeft(underflow);
		} else if(css_margin_left.isAuto() && css_margin_right.isAuto()) {
			setMarginLeft(underflow / 2);
			setMarginRight(underflow / 2);
		} 

		if(isFloat()) {
			setMarginLeft(0);
			setMarginRight(0);
		}
	}

	void BlockBox::layoutChildren(LayoutEngine& eng)
	{
		// XXX we should add collapsible margins to children here.
		child_height_ = 0;
		for(auto& child : getChildren()) {
			if(!child->isFloat()) {
				child_height_ += child->getHeight() + child->getMBPHeight();
			}
		}
		auto css_height = getCssHeight();
		if(css_height.isAuto() && !(getNode() != nullptr && getNode()->id() == NodeId::ELEMENT && getNode()->isReplaced())) {
			setContentHeight(child_height_);
		}
	}
	
	void BlockBox::layoutHeight(const Dimensions& containing)
	{
		// a set height value overrides the calculated value.
		if(!getCssHeight().isAuto()) {
			FixedPoint h = getCssHeight().getLength().compute(containing.content_.height);
			if(h > child_height_) {
				/* apply overflow properties */
			}
			setContentHeight(h);
		}
		// XXX deal with min-height and max-height
		//auto& min_h = getCssMinHeight().compute(containing.content_.height);
		//if(getHeight() < min_h) {
		//	setContentHeight(min_h);
		//}
		//auto& css_max_h = getCssMaxHeight();
		//if(!css_max_h.isNone()) {
		//	FixedPoint max_h = css_max_h.getValue<Length>().compute(containing.content_.height);
		//	if(getHeight() > max_h) {
		//		setContentHeight(max_h);
		//	}
		//}
	}

	void BlockBox::handleRender(DisplayListPtr display_list, const point& offset) const
	{
		NodePtr node = getNode();
		if(node != nullptr && node->isReplaced()) {
			auto r = node->getRenderable();
			r->setPosition(glm::vec3(static_cast<float>(offset.x)/65536.0f,
				static_cast<float>(offset.y)/65536.0f,
				0.0f));
			display_list->addRenderable(r);
		}
	}

}
