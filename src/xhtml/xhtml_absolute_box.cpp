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

#include "xhtml_absolute_box.hpp"
#include "xhtml_layout_engine.hpp"

namespace xhtml
{
	using namespace css;

	AbsoluteBox::AbsoluteBox(BoxPtr parent, NodePtr node)
		: Box(BoxId::ABSOLUTE, parent, node)
	{
		RenderContext& ctx = RenderContext::get();
		
		css_rect_[0] = ctx.getComputedValue(Property::LEFT).getValue<Width>();
		css_rect_[1] = ctx.getComputedValue(Property::TOP).getValue<Width>();
		css_rect_[2] = ctx.getComputedValue(Property::RIGHT).getValue<Width>();
		css_rect_[3] = ctx.getComputedValue(Property::BOTTOM).getValue<Width>();
	}

	std::string AbsoluteBox::toString() const
	{
		std::ostringstream ss;
		ss << "AbsoluteBox: " << getDimensions().content_;
		return ss.str();
	}

	void AbsoluteBox::handleLayout(LayoutEngine& eng, const Dimensions& containing)
	{
		Rect container = containing.content_;

		// Find the first ancestor with non-static position
		auto parent = getParent();
		if(parent != nullptr && !parent->ancestralTraverse([&container](const ConstBoxPtr& box) {
			if(box->getPosition() != CssPosition::STATIC) {
				container = box->getDimensions().content_;
				return false;
			}
			return true;
		})) {
			// couldn't find anything use the layout engine dimensions
			container = eng.getDimensions().content_;
		}
		// we expect top/left and either bottom/right or width/height
		// if the appropriate thing isn't set then we use the parent container dimensions.
		RenderContext& ctx = RenderContext::get();
		const FixedPoint containing_width = container.width;
		const FixedPoint containing_height = container.height;
		
		FixedPoint left = container.x;
		if(!css_rect_[0].isAuto()) {
			left = css_rect_[0].getLength().compute(containing_width);
		}
		FixedPoint top = container.y;
		if(!css_rect_[1].isAuto()) {
			top = css_rect_[1].getLength().compute(containing_height);
		}
		FixedPoint width = container.width;
		if(!css_rect_[2].isAuto()) {
			width = css_rect_[2].getLength().compute(containing_width) - left + container.width;
		}
		FixedPoint height = container.height;
		if(!css_rect_[3].isAuto()) {
			height = css_rect_[3].getLength().compute(containing_height) - top + container.height;
		}

		// if width/height properties are set they override right/bottom.
		if(!getCssWidth().isAuto()) {
			width = getCssWidth().getLength().compute(containing_width);
		}

		if(!getCssHeight().isAuto()) {
			height = getCssHeight().getLength().compute(containing_height);
		}

		calculateHorzMPB(containing_width);
		calculateVertMPB(containing_height);

		setContentX(left + getMPBLeft());
		setContentY(top + getMPBTop());
		setContentWidth(width - getMBPWidth());
		setContentHeight(height - getMBPHeight());

		NodePtr node = getNode();
		if(node != nullptr) {
			eng.pushOpenBox();
			for(auto& child : node->getChildren()) {
				eng.formatNode(child, shared_from_this(), getDimensions());
			}
			eng.closeOpenBox();
			eng.popOpenBox();
		}
	}

	void AbsoluteBox::handleRender(DisplayListPtr display_list, const point& offset) const
	{
		// XXX
	}
}
