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

	BlockBox::BlockBox(BoxPtr parent, NodePtr node)
		: Box(BoxId::BLOCK, parent, node)
	{
	}

	std::string BlockBox::toString() const
	{
		std::ostringstream ss;
		ss << "BlockBox: " << getDimensions().content_;
		auto node = getNode();
		if(node != nullptr) {
			ss << " has child block boxes: " << (node->hasChildBlockBox() ? "yes" : "no");
		}
		return ss.str();
	}

	void BlockBox::handleLayout(LayoutEngine& eng, const Dimensions& containing)
	{
		std::unique_ptr<RenderContext::Manager> ctx_manager;
		NodePtr node = getNode();
		bool is_replaced = false;
		if(node != nullptr && node->id() == NodeId::ELEMENT) {
			ctx_manager.reset(new RenderContext::Manager(node->getProperties()));
			is_replaced = node->isReplaced();
		}

		if(is_replaced) {
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
			layoutPosition(eng, containing);
			layoutChildren(eng);
		} else {
			layoutWidth(containing);
			layoutPosition(eng, containing);
			layoutChildren(eng);
			layoutHeight(containing);
		}
	}

	void BlockBox::layoutWidth(const Dimensions& containing)
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

	void BlockBox::layoutPosition(LayoutEngine& eng, const Dimensions& containing)
	{
		const FixedPoint containing_height = containing.content_.height;

		calculateVertMPB(containing_height);

		setContentX(getMBPLeft());
		setContentY(containing.content_.height + getMBPTop());

		if(getPosition() == CssPosition::FIXED) {
			if(!getCssLeft().isAuto()) {
				setContentX(getCssLeft().getLength().compute(containing.content_.width) + getMBPLeft());
			}
			if(!getCssTop().isAuto()) {
				setContentY(getCssTop().getLength().compute(containing.content_.height) + getMBPTop());
			}
		} 
	}

	void BlockBox::layoutChildren(LayoutEngine& eng)
	{
		NodePtr node = getNode();
		if(node != nullptr) {
			for(auto& child : node->getChildren()) {
				if(getPosition() == CssPosition::FIXED) {
					eng.pushOpenBox();
				}
 				BoxPtr box_child = eng.formatNode(child, shared_from_this(), getDimensions());
				//if(box_child != nullptr) {
				//	setContentHeight(box_child->getDimensions().content_.y + box_child->getDimensions().content_.height + box_child->getMBPBottom());
				//}
				if(getPosition() == CssPosition::FIXED) {
					eng.closeOpenBox();
					eng.popOpenBox();
				}
			}
		}

		// close any open boxes.
		eng.closeOpenBox();

		// horrible ugly kludge.
		for(auto& child : getChildren()) {
			if(child->isBlockBox()) {
				if(child->id() == BoxId::ANON_BLOCK_BOX) {
					child->layout(eng, getDimensions());
				}
				setContentHeight(child->getDimensions().content_.y + child->getDimensions().content_.height + child->getMBPBottom());
			}
		}

		FixedPoint width = 0;
		//bool has_block_box = false;
		for(auto& child : getChildren()) {
			width = std::max(width, child->getDimensions().content_.width + child->getMBPWidth());
			setContentHeight(child->getDimensions().content_.y + child->getDimensions().content_.height + child->getMBPHeight());

			/*if(child->isBlockBox()) {
				has_block_box = true;
			}*/
		}

		/*if(has_block_box) {
			//  if a block container box has a block-level box inside it, then we force it to have only block-level boxes inside it.
			// e.g. <div>Some Text<p>More Text</p></div> then the "Some Text" shall be wrapped in a block box.
			std::vector<BoxPtr> children = getChildren();
			getChildren().clear();
			std::shared_ptr<BlockBox> open = nullptr;
			for(auto child : children) {
				if(child->isBlockBox()) {
					if(open != nullptr) {
						getChildren().emplace_back(open);
						open.reset();
					}
					getChildren().emplace_back(child);
				} else {
					if(open == nullptr) {
						open = std::make_shared<BlockBox>(shared_from_this(), nullptr);
						open->init();
					}
					open->getChildren().emplace_back(child);
				}
			}
			if(open != nullptr) {
				getChildren().emplace_back(open);
			}

			for(auto& child : getChildren()) {
				child->reLayout(eng, getDimensions());
			}
		}*/
	}
	
	void BlockBox::layoutHeight(const Dimensions& containing)
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
