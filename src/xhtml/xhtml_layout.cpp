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
#include "xhtml_layout.hpp"
#include "xhtml_node.hpp"

namespace xhtml
{
	LayoutBox::LayoutBox(NodePtr node, css::CssDisplay display)
		: node_(node),
		  display_(display),
		  dimensions_(),
		  children_()
	{
		ASSERT_LOG(display_ != css::CssDisplay::NONE, "Tried to create a layout node with a display of none.");
	}

	LayoutBoxPtr LayoutBox::create(NodePtr node)
	{
		auto display = node->getStyle("display").getValue<css::CssDisplay>();
		if(display != css::CssDisplay::NONE) {
			auto root = std::make_shared<LayoutBox>(node, display);

			LayoutBoxPtr inline_container;
			for(auto& c : node->getChildren()) {
				auto disp = c->getStyle("display").getValue<css::CssDisplay>();
				if(disp == css::CssDisplay::NONE) {
					// ignore child nodes with display none.
				} else if(disp == css::CssDisplay::INLINE && root->display_ == css::CssDisplay::BLOCK) {
					if(inline_container == nullptr) {
						inline_container = std::make_shared<LayoutBox>(nullptr, css::CssDisplay::BLOCK);
						root->children_.emplace_back(inline_container);
					}
					inline_container->children_.emplace_back(create(c));
				} else {
					inline_container.reset();
					root->children_.emplace_back(create(c));
				}
			}

			return root;
		}
		return nullptr;
	}

	void LayoutBox::layout(RenderContext* ctx, const Dimensions& containing)
	{
		layoutWidth(ctx, containing);
		layoutPosition(ctx, containing);
		layoutChildren(ctx);
		layoutHeight(ctx, containing);
	}

	void LayoutBox::layoutWidth(RenderContext* ctx, const Dimensions& containing)
	{
		auto node = node_.lock();
		// boxes without nodes are anonymous and thus dimensionless.
		if(node != nullptr) {
			double containing_width = containing.content_.w();
			auto css_width = node->getStyle("width").getValue<css::CssLength>();
			double width = css_width.evaluate(containing_width, ctx);

			dimensions_.border_.left = node->getStyle("border-left-width").getValue<css::CssLength>().evaluate(containing_width, ctx);
			dimensions_.border_.right = node->getStyle("border-right-width").getValue<css::CssLength>().evaluate(containing_width, ctx);

			dimensions_.padding_.left = node->getStyle("padding-left").getValue<css::CssLength>().evaluate(containing_width, ctx);
			dimensions_.padding_.right = node->getStyle("padding-right").getValue<css::CssLength>().evaluate(containing_width, ctx);

			auto css_margin_left = node->getStyle("margin-left").getValue<css::CssLength>();
			auto css_margin_right = node->getStyle("margin-right").getValue<css::CssLength>();
			dimensions_.margin_.left = css_margin_left.evaluate(containing_width, ctx);
			dimensions_.margin_.right = css_margin_right.evaluate(containing_width, ctx);

			double total = dimensions_.border_.left + dimensions_.border_.right
				+ dimensions_.padding_.left + dimensions_.padding_.right
				+ dimensions_.margin_.left + dimensions_.margin_.right
				+ width;
			
			if(!css_width.isAuto() && total > containing.content_.w()) {
				if(css_margin_left.isAuto()) {
					dimensions_.margin_.left = 0;
				}
				if(css_margin_right.isAuto()) {
					dimensions_.margin_.right = 0;
				}
			}

			// If negative is overflow.
			double underflow = containing.content_.w() - total;

			if(css_width.isAuto()) {
				if(css_margin_left.isAuto()) {
					dimensions_.margin_.left = 0;
				}
				if(css_margin_right.isAuto()) {
					dimensions_.margin_.right = 0;
				}
				if(underflow >= 0) {
					width = underflow;
				} else {
					width = 0;
					dimensions_.margin_.right = css_margin_right.evaluate(containing_width, ctx) + underflow;
				}
			} else if(!css_margin_left.isAuto() && !css_margin_right.isAuto()) {
				dimensions_.margin_.right = dimensions_.margin_.right + underflow;
			} else if(!css_margin_left.isAuto() && css_margin_right.isAuto()) {
				dimensions_.margin_.right = underflow;
			} else if(css_margin_left.isAuto() && !css_margin_right.isAuto()) {
				dimensions_.margin_.left = underflow;
			} else if(css_margin_left.isAuto() && css_margin_right.isAuto()) {
				dimensions_.margin_.left = underflow/2.0;
				dimensions_.margin_.right = underflow/2.0;
			} 
		} else {
			dimensions_.content_.set_w(containing.content_.w());
		}
	}

	void LayoutBox::layoutPosition(RenderContext* ctx, const Dimensions& containing)
	{
		auto node = node_.lock();
		if(node != nullptr) {
			double containing_height = containing.content_.h();
			dimensions_.border_.top = node->getStyle("border-top-width").getValue<css::CssLength>().evaluate(containing_height, ctx);
			dimensions_.border_.bottom = node->getStyle("border-bottom-width").getValue<css::CssLength>().evaluate(containing_height, ctx);

			dimensions_.padding_.top = node->getStyle("padding-top").getValue<css::CssLength>().evaluate(containing_height, ctx);
			dimensions_.padding_.bottom = node->getStyle("padding-bottom").getValue<css::CssLength>().evaluate(containing_height, ctx);

			dimensions_.margin_.top = node->getStyle("margin-top").getValue<css::CssLength>().evaluate(containing_height, ctx);
			dimensions_.margin_.bottom = node->getStyle("margin-bottom").getValue<css::CssLength>().evaluate(containing_height, ctx);

			dimensions_.content_.set_x(containing.content_.x() + dimensions_.margin_.left + dimensions_.padding_.left + dimensions_.border_.left);
			dimensions_.content_.set_y(containing.content_.y2() + dimensions_.margin_.top + dimensions_.padding_.top + dimensions_.border_.top);
		} else {
			dimensions_.content_.set_xy(containing.content_.x(), containing.content_.y2());
		}
	}

	void LayoutBox::layoutChildren(RenderContext* ctx)
	{
		for(auto& child : children_) {
			child->layout(ctx, dimensions_);
			dimensions_.content_.set_h(dimensions_.content_.h()
				+ child->dimensions_.content_.h() 
				+ child->dimensions_.margin_.top + child->dimensions_.margin_.bottom
				+ child->dimensions_.padding_.top + child->dimensions_.padding_.bottom
				+ child->dimensions_.border_.top + child->dimensions_.border_.bottom);
		}
	}

	void LayoutBox::layoutHeight(RenderContext* ctx, const Dimensions& containing)
	{
		auto node = node_.lock();
		if(node != nullptr) {
			// a set height value overrides the calculated value.
			auto h = node->getStyle("height");
			if(!h.empty()) {
				dimensions_.content_.set_h(h.getValue<css::CssLength>().evaluate(containing.content_.h(), ctx));
			}
		}		
	}

	void LayoutBox::preOrderTraversal(std::function<void(LayoutBoxPtr)> fn)
	{
		fn(shared_from_this());
		for(auto& child : children_) {
			child->preOrderTraversal(fn);
		}
	}

	std::string LayoutBox::toString() const
	{
		std::stringstream ss;
		auto node = node_.lock();
		ss << "Box(" << (node ? node->toString() : "anonymous") << ", content: " << dimensions_.content_ << ")";
		return ss.str();
	}

	Object LayoutBox::getNodeStyle(const std::string& style)
	{
		auto node = node_.lock();
		if(node != nullptr) {
			return node->getStyle(style);
		}
		return Object();
	}
}

/*dimensions_.border_ = EdgeSize(
	node->getStyle("border-left-width").getValue<css::CssLength>().evaluate(ctx),
	node->getStyle("border-top-width").getValue<css::CssLength>().evaluate(ctx),
	node->getStyle("border-right-width").getValue<css::CssLength>().evaluate(ctx),
	node->getStyle("border-bottom-width").getValue<css::CssLength>().evaluate(ctx));
dimensions_.margin_ = EdgeSize(
	node->getStyle("margin-left").getValue<css::CssLength>().evaluate(ctx),
	node->getStyle("margin-top").getValue<css::CssLength>().evaluate(ctx),
	node->getStyle("margin-right").getValue<css::CssLength>().evaluate(ctx),
	node->getStyle("margin-bottom").getValue<css::CssLength>().evaluate(ctx));
dimensions_.padding_ = EdgeSize(
	node->getStyle("padding-left").getValue<css::CssLength>().evaluate(ctx),
	node->getStyle("padding-top").getValue<css::CssLength>().evaluate(ctx),
	node->getStyle("padding-right").getValue<css::CssLength>().evaluate(ctx),
	node->getStyle("padding-bottom").getValue<css::CssLength>().evaluate(ctx));*/
