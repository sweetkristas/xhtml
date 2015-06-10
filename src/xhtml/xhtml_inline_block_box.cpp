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

#include "xhtml_inline_block_box.hpp"
#include "xhtml_layout_engine.hpp"

namespace xhtml
{
	using namespace css;

	InlineBlockBox::InlineBlockBox(BoxPtr parent, NodePtr node)
		: Box(BoxId::INLINE_BLOCK, parent, node),
		  is_replacable_(false)
	{
	}

	std::string InlineBlockBox::toString() const
	{
		std::ostringstream ss;
		ss << "InlineBlockBox: " << getDimensions().content_;
		return ss.str();
	}

	void InlineBlockBox::handleLayout(LayoutEngine& eng, const Dimensions& containing)
	{
		std::unique_ptr<RenderContext::Manager> ctx_manager;
		NodePtr node = getNode();
		bool is_replaced = false;
		if(node != nullptr && node->id() == NodeId::ELEMENT) {
			ctx_manager.reset(new RenderContext::Manager(node->getProperties()));
			is_replacable_ = node->isReplaced();
		}

		if(is_replacable_) {
			calculateHorzMPB(containing.content_.width);
			setContentWidth(node->getDimensions().w() * LayoutEngine::getFixedPointScale());
			setContentHeight(node->getDimensions().h() * LayoutEngine::getFixedPointScale());
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
			// add this to set a default width to the containing box if it is zero. we should re-evaluate the actual width
			// based on the children size.
			if(getDimensions().content_.width == 0) {
				setContentWidth(containing.content_.width);
			}
			layoutPosition(eng, containing);
			layoutChildren(eng);
		} else {
			layoutWidth(containing);
			layoutChildren(eng);
			layoutPosition(eng, containing);
			layoutHeight(containing);
		}
	}

	void InlineBlockBox::layoutWidth(const Dimensions& containing)
	{
		RenderContext& ctx = RenderContext::get();
		const FixedPoint containing_width = containing.content_.width;

		auto css_width = getCssWidth();
		FixedPoint width = 0;
		if(!css_width.isAuto()) {
			width = css_width.getLength().compute(containing_width);
			setContentWidth(width);
		}

		calculateHorzMPB(containing_width);
		auto css_margin_left = getCssMargin(Side::LEFT);
		auto css_margin_right = getCssMargin(Side::RIGHT);

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
	}

	void InlineBlockBox::layoutPosition(LayoutEngine& eng, const Dimensions& containing)
	{
		const FixedPoint containing_height = containing.content_.height;

		calculateVertMPB(containing_height);

		//eng.moveCursorToFitWidth(getDimensions().content_.width + getMBPWidth(), containing.content_.width);
		/*if(getDimensions().content_.width >= containing.content_.width) {
			setContentX(0);
			setContentY(eng.getLineHeight());
		} else {
			//setContentX(eng.getCursor().x);
			setContentY(0);
			setContentX(0);
		}*/
		// 0 aligns the top of the box with the baseline,
		// setting to negative height aligns the bottom of the box with the baseline.
		//eng.incrCursor(getDimensions().content_.width + getMBPWidth());
	}

	void InlineBlockBox::layoutChildren(LayoutEngine& eng)
	{
	}

	void InlineBlockBox::layoutHeight(const Dimensions& containing)
	{
		RenderContext& ctx = RenderContext::get();
		// a set height value overrides the calculated value.
		if(!getCssHeight().isAuto()) {
			setContentHeight(getCssHeight().getLength().compute(containing.content_.height));
		}
		// XXX deal with min-height and max-height
		//auto min_h = ctx.getComputedValue(Property::MIN_HEIGHT).getValue<Length>().compute(containing.content_.height);
		//if(getDimensions().content_.height() < min_h) {
		//	setContentHeight(min_h);
		//}
		//auto css_max_h = ctx.getComputedValue(Property::MAX_HEIGHT).getValue<Height>();
		//if(!css_max_h.isNone()) {
		//	FixedPoint max_h = css_max_h.getValue<Length>().compute(containing.content_.height);
		//	if(getDimensions().content_.height() > max_h) {
		//		setContentHeight(max_h);
		//	}
		//}
	}

	void InlineBlockBox::handleRender(DisplayListPtr display_list, const point& offset) const
	{
		NodePtr node = getNode();
		if(node != nullptr && node->isReplaced()) {
			auto r = node->getRenderable();
			if(r == nullptr) {
				LOG_ERROR("No renderable returned for repalced element: " << node->toString());
			} else {
				r->setPosition(glm::vec3(static_cast<float>(offset.x)/LayoutEngine::getFixedPointScaleFloat(),
					static_cast<float>(offset.y)/LayoutEngine::getFixedPointScaleFloat(),
					0.0f));
				display_list->addRenderable(r);
			}
		}
	}
}
