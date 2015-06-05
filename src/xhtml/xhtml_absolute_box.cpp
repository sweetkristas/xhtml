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
		if(!getCssLeft().isAuto()) {
			left = getCssLeft().getLength().compute(containing_width);
		}
		FixedPoint top = container.y;
		if(!getCssTop().isAuto()) {
			top = getCssTop().getLength().compute(containing_height);
		}
		FixedPoint width = container.width;
		if(!getCssRight().isAuto()) {
			width = getCssRight().getLength().compute(containing_width) - left + container.width;
		}
		FixedPoint height = container.height;
		if(!getCssBottom().isAuto()) {
			height = getCssBottom().getLength().compute(containing_height) - top + container.height;
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

		setContentX(left + getMBPLeft());
		setContentY(top + getMBPTop());
		setContentWidth(width - getMBPWidth());
		setContentHeight(height - getMBPHeight());
	}

	void AbsoluteBox::handleRender(DisplayListPtr display_list, const point& offset) const
	{
		// XXX
	}
}
