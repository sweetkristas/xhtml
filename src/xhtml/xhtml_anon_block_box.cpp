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

#include "xhtml_anon_block_box.hpp"

namespace xhtml
{
	AnonBlockBox::AnonBlockBox(BoxPtr parent)
		: Box(BoxId::ANON_BLOCK_BOX, parent, nullptr)
	{
	}

	std::string AnonBlockBox::toString() const 
	{
		std::ostringstream ss;
		ss << "AnonBlockBox: " << getDimensions().content_;
		return ss.str();
	}

	void AnonBlockBox::handleLayout(LayoutEngine& eng, const Dimensions& containing)
	{
		setContentWidth(containing.content_.width);
		
		setContentX(0);
		setContentY(containing.content_.height);

		FixedPoint width = 0;
		for(auto& child : getChildren()) {
			width = std::max(width, child->getDimensions().content_.width + child->getMBPWidth());
			setContentHeight(child->getDimensions().content_.y + child->getDimensions().content_.height + child->getMBPHeight());
		}
		setContentWidth(width);
	}

	void AnonBlockBox::handleRender(DisplayListPtr display_list, const point& offset) const
	{
		// nothing need be done.
	}

	void AnonBlockBox::handleReLayout(LayoutEngine& eng, const Dimensions& containing)
	{
		ASSERT_LOG(false, "AnonBlockBox::handleReLayout");
	}
}
