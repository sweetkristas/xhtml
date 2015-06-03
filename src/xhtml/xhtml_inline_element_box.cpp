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

#include "xhtml_inline_element_box.hpp"
#include "xhtml_layout_engine.hpp"

namespace xhtml
{
	InlineElementBox::InlineElementBox(BoxPtr parent, NodePtr node)
		: Box(BoxId::INLINE_ELEMENT, parent, node)
	{
	}

	void InlineElementBox::handleLayout(LayoutEngine& eng, const Dimensions& containing)
	{
		setContentY(eng.getCursor().y);
		setContentX(0);
		setContentWidth(containing.content_.width);
		NodePtr node = getNode();
		if(node != nullptr) {
			for(auto& child : node->getChildren()) {
				eng.formatNode(child, shared_from_this(), getDimensions());
			}
		}
		// XXX We should assign margin/padding/border as appropriate here.
		FixedPoint max_w = 0;
		for(auto& child : getChildren()) {
			setContentHeight(child->getDimensions().content_.height + child->getMBPHeight());
			max_w = std::max(max_w, child->getMBPWidth() + child->getDimensions().content_.width);
		}
		setContentWidth(max_w);
	}

	std::string InlineElementBox::toString() const
	{
		std::ostringstream ss;
		ss << "InlineElementBox: " << getDimensions().content_;
		return ss.str();
	}

	void InlineElementBox::handleRender(DisplayListPtr display_list, const point& offset) const
	{
		auto node = getNode();
		if(node != nullptr) {
			auto r = node->getRenderable();
			if(r != nullptr) {
				r->setPosition(glm::vec3(offset.x/LayoutEngine::getFixedPointScaleFloat(), offset.y/LayoutEngine::getFixedPointScaleFloat(), 0.0f));
				display_list->addRenderable(r);
			}
		}
	}
}
