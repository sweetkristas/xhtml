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

#include "xhtml_layout_engine.hpp"
#include "xhtml_anon_block_box.hpp"
#include "xhtml_block_box.hpp"
#include "xhtml_inline_block_box.hpp"
#include "xhtml_listitem_box.hpp"
#include "xhtml_line_box.hpp"
#include "xhtml_root_box.hpp"
#include "xhtml_text_box.hpp"
#include "xhtml_text_node.hpp"

namespace xhtml
{
	// This is to ensure our fixed-point type will have enough precision.
	static_assert(sizeof(FixedPoint) < 32, "An int must be greater than or equal to 32 bits");

	using namespace css;

	namespace 
	{
		std::string display_string(CssDisplay disp) {
			switch(disp) {
				case CssDisplay::BLOCK:					return "block";
				case CssDisplay::INLINE:				return "inline";
				case CssDisplay::INLINE_BLOCK:			return "inline-block";
				case CssDisplay::LIST_ITEM:				return "list-item";
				case CssDisplay::TABLE:					return "table";
				case CssDisplay::INLINE_TABLE:			return "inline-table";
				case CssDisplay::TABLE_ROW_GROUP:		return "table-row-group";
				case CssDisplay::TABLE_HEADER_GROUP:	return "table-header-group";
				case CssDisplay::TABLE_FOOTER_GROUP:	return "table-footer-group";
				case CssDisplay::TABLE_ROW:				return "table-row";
				case CssDisplay::TABLE_COLUMN_GROUP:	return "table-column-group";
				case CssDisplay::TABLE_COLUMN:			return "table-column";
				case CssDisplay::TABLE_CELL:			return "table-cell";
				case CssDisplay::TABLE_CAPTION:			return "table-caption";
				case CssDisplay::NONE:					return "none";
				default: 
					ASSERT_LOG(false, "illegal display value: " << static_cast<int>(disp));
					break;
			}
			return "none";
		}

		template<typename T>
		struct StackManager
		{
			StackManager(std::stack<T>& counter, const T& defa=T()) : counter_(counter) { counter_.emplace(defa); }
			~StackManager() { counter_.pop(); }
			std::stack<T>& counter_;
		};
	}
	
	// XXX We're not handling text alignment or justification yet.
	LayoutEngine::LayoutEngine() 
		: root_(nullptr), 
			dims_(), 
			ctx_(RenderContext::get()), 
			cursor_(),
			open_(), 
			list_item_counter_(),
			offset_(),
			anon_block_box_()
	{
		cursor_.emplace(0, 0);
		list_item_counter_.emplace(0);
		offset_.emplace(point());
		anon_block_box_.emplace(nullptr);
	}

	void LayoutEngine::formatNode(NodePtr node, BoxPtr parent, const point& container) 
	{
		if(root_ == nullptr) {
			RenderContext::Manager ctx_manager(node->getProperties());

			root_ = std::make_shared<RootBox>(nullptr, node);
			root_->init();
			dims_.content_ = Rect(0, 0, container.x, 0);
			root_->layout(*this, dims_);
			return;
		}
	}

	BoxPtr LayoutEngine::formatNode(NodePtr node, BoxPtr parent, const Dimensions& container, std::function<void(BoxPtr, bool)> pre_layout_fn) 
	{
		StackManager<point> offset_manager(offset_, offset_.top() + point(parent->getDimensions().content_.x, parent->getDimensions().content_.y));

		if(node->id() == NodeId::ELEMENT) {
			RenderContext::Manager ctx_manager(node->getProperties());

			std::unique_ptr<StackManager<int>> li_manager;
			if(node->hasTag(ElementId::UL) || node->hasTag(ElementId::OL)) {
				li_manager.reset(new StackManager<int>(list_item_counter_, 0));
			}
			if(node->hasTag(ElementId::LI) ) {
				auto &top = list_item_counter_.top();
				++top;
			}

			const CssDisplay display = ctx_.getComputedValue(Property::DISPLAY).getValue<CssDisplay>();
			const CssFloat cfloat = ctx_.getComputedValue(Property::FLOAT).getValue<CssFloat>();
			const CssPosition position = ctx_.getComputedValue(Property::POSITION).getValue<CssPosition>();

			if(display == CssDisplay::NONE) {
				// Do not create a box for this or it's children
				// early return
				return nullptr;
			}

			if(position == CssPosition::ABSOLUTE) {
				// absolute positioned elements are taken out of the normal document flow
				parent->addAbsoluteElement(node);
				return nullptr;
			} else if(position == CssPosition::FIXED) {
				// fixed positioned elements are taken out of the normal document flow
				root_->addFixedElement(node);
				return nullptr;
			} else {
				if(cfloat != CssFloat::NONE) {
					bool saved_open = false;
					if(isOpenBox()) {
						saved_open = true;
						pushOpenBox();
					}
					// XXX need to add an offset to position for the float box based on body margin.
					// XXX if the current display is one of the CssDisplay::TABLE* styles then this should be
					// a table box rather than a block box.
					root_->addFloatBox(*this, std::make_shared<BlockBox>(root_, node), cfloat, parent->getDimensions().content_.y + cursor_.top().y);
					if(saved_open) {
						popOpenBox();
						open_.top().open_box_->setContentX(open_.top().open_box_->getDimensions().content_.x + getXAtCursor());
					}
					return nullptr;
				}
				switch(display) {
					case CssDisplay::NONE:
						// Do not create a box for this or it's children
						return nullptr;
					case CssDisplay::INLINE: {
						layoutInlineElement(node, parent, pre_layout_fn);
						return nullptr;
					}
					case CssDisplay::BLOCK: {
						closeOpenBox();
						cursor_.top().x = 0;
						cursor_.top().y = 0;
						auto box = parent->addChild(std::make_shared<BlockBox>(parent, node));
						if(pre_layout_fn) {
							pre_layout_fn(box, false);
						}
						if(node->hasChildBlockBox()) {
							anon_block_box_.emplace(std::make_shared<AnonBlockBox>(box));
						} else {
							anon_block_box_.emplace(nullptr);
						}
						box->layout(*this, container);
						//if(node->hasChildBlockBox()) {
						//	anon_block_box_.top()->layout(*this, box->getDimensions());
						//}
						anon_block_box_.pop();
						return box;
					}
					case CssDisplay::INLINE_BLOCK: {							
						BoxPtr open = getOpenBox(parent);
						pushNewCursor();
						auto box = open->addChild(std::make_shared<InlineBlockBox>(open, node));
						if(pre_layout_fn) {
							pre_layout_fn(box, false);
						}
						box->layout(*this, container);
						cursor_.pop();
						return box;
					}
					case CssDisplay::LIST_ITEM: {
						auto box = parent->addChild(std::make_shared<ListItemBox>(parent, node, list_item_counter_.top()));
						if(pre_layout_fn) {
							pre_layout_fn(box, false);
						}
						box->layout(*this, container);
						cursor_.top().y = box->getMBPHeight() + box->getDimensions().content_.height;
						cursor_.top().x = 0;
						return nullptr;
					}
					case CssDisplay::TABLE:
					case CssDisplay::INLINE_TABLE:
					case CssDisplay::TABLE_ROW_GROUP:
					case CssDisplay::TABLE_HEADER_GROUP:
					case CssDisplay::TABLE_FOOTER_GROUP:
					case CssDisplay::TABLE_ROW:
					case CssDisplay::TABLE_COLUMN_GROUP:
					case CssDisplay::TABLE_COLUMN:
					case CssDisplay::TABLE_CELL:
					case CssDisplay::TABLE_CAPTION:
						ASSERT_LOG(false, "FIXME: LayoutEngine::formatNode(): " << display_string(display));
						break;
					default:
						ASSERT_LOG(false, "illegal display value: " << static_cast<int>(display));
						break;
				}
			}
		} else if(node->id() == NodeId::TEXT) {
			// these nodes are inline/static by definition.
			layoutInlineText(node, parent, pre_layout_fn);
			return nullptr;
		} else {
			ASSERT_LOG(false, "Unhandled node id, only elements and text can be used in layout: " << static_cast<int>(node->id()));
		}
		return nullptr;
	}

	void LayoutEngine::layoutInlineElement(NodePtr node, BoxPtr parent, std::function<void(BoxPtr, bool)> pre_layout_fn) 
	{
		if(node->isReplaced()) {
			BoxPtr open = getOpenBox(parent);
			// This should be adding to the currently open box.
			auto inline_element_box = open->addInlineElement(node);
			pushOpenBox();
			if(pre_layout_fn) {
				pre_layout_fn(inline_element_box, false);
			}
			inline_element_box->layout(*this, open->getDimensions());
			popOpenBox();
		} else {
			if(node->getChildren().empty()) {
				return;
			}

			// XXX we should apply border styles to inline element here (and backgrounds)
			std::vector<FixedPoint> padding = generatePadding();
			std::vector<FixedPoint> border_width = generateBorderWidth();
			std::vector<CssBorderStyle> border_style = generateBorderStyle();
			std::vector<KRE::Color> border_color = generateBorderColor();

			auto first = node->getChildren().cbegin();
			auto last = node->getChildren().cend()-1;

			bool is_first = true;
			for(auto it = first; it != node->getChildren().cend(); ++it) {
				auto& child = *it;
				bool is_last = it == last;
				formatNode(child, parent, parent->getDimensions(), [&border_color, &border_style, &padding, &border_width, &is_first, is_last](BoxPtr box, bool last) {
					if(is_first) {
						is_first = false;
						box->setPaddingLeft(padding[1]);
						box->setBorderLeft(border_width[1]);
						box->getBorderInfo().setBorderStyleLeft(border_style[1]);
						box->getBorderInfo().setBorderColorLeft(border_color[1]);
					}
					if(is_last && last) {
						box->setBorderRight(border_width[3]);
						box->setPaddingRight(padding[3]);
						box->getBorderInfo().setBorderStyleRight(border_style[3]);
						box->getBorderInfo().setBorderColorRight(border_color[3]);
					}
					box->setBorderTop(border_width[0]);
					box->setBorderBottom(border_width[2]);
					box->getBorderInfo().setBorderStyleTop(border_style[0]);
					box->getBorderInfo().setBorderStyleBottom(border_style[2]);
					box->getBorderInfo().setBorderColorTop(border_color[0]);
					box->getBorderInfo().setBorderColorBottom(border_color[2]);
				});
			}
			
		}
	}

	void LayoutEngine::pushNewCursor()
	{
		cursor_.emplace(point());
		cursor_.top().x = 0;
	}

	std::vector<CssBorderStyle> LayoutEngine::generateBorderStyle() 
	{
		std::vector<CssBorderStyle> res;
		res.resize(4);
		res[0] = ctx_.getComputedValue(Property::BORDER_TOP_STYLE).getValue<CssBorderStyle>();
		res[1] = ctx_.getComputedValue(Property::BORDER_LEFT_STYLE).getValue<CssBorderStyle>();
		res[2] = ctx_.getComputedValue(Property::BORDER_BOTTOM_STYLE).getValue<CssBorderStyle>();
		res[3] = ctx_.getComputedValue(Property::BORDER_RIGHT_STYLE).getValue<CssBorderStyle>();
		return res;
	}

	std::vector<KRE::Color> LayoutEngine::generateBorderColor() 
	{
		std::vector<KRE::Color> res;
		res.resize(4);
		res[0] = ctx_.getComputedValue(Property::BORDER_TOP_COLOR).getValue<CssColor>().compute();
		res[1] = ctx_.getComputedValue(Property::BORDER_LEFT_COLOR).getValue<CssColor>().compute();
		res[2] = ctx_.getComputedValue(Property::BORDER_BOTTOM_COLOR).getValue<CssColor>().compute();
		res[3] = ctx_.getComputedValue(Property::BORDER_RIGHT_COLOR).getValue<CssColor>().compute();
		return res;
	}

	std::vector<FixedPoint> LayoutEngine::generateBorderWidth() 
	{
		std::vector<FixedPoint> res;
		res.resize(4);
		res[0] = ctx_.getComputedValue(Property::BORDER_TOP_WIDTH).getValue<Length>().compute();
		res[1] = ctx_.getComputedValue(Property::BORDER_LEFT_WIDTH).getValue<Length>().compute();
		res[2] = ctx_.getComputedValue(Property::BORDER_BOTTOM_WIDTH).getValue<Length>().compute();
		res[3] = ctx_.getComputedValue(Property::BORDER_RIGHT_WIDTH).getValue<Length>().compute();
		return res;
	}

	std::vector<FixedPoint> LayoutEngine::generatePadding() 
	{
		std::vector<FixedPoint> res;
		res.resize(4);
		res[0] = ctx_.getComputedValue(Property::PADDING_TOP).getValue<Length>().compute();
		res[1] = ctx_.getComputedValue(Property::PADDING_LEFT).getValue<Length>().compute();
		res[2] = ctx_.getComputedValue(Property::PADDING_BOTTOM).getValue<Length>().compute();
		res[3] = ctx_.getComputedValue(Property::PADDING_RIGHT).getValue<Length>().compute();
		return res;
	}

	void LayoutEngine::layoutInlineText(NodePtr node, BoxPtr parent, std::function<void(BoxPtr, bool)> pre_layout_fn) 
	{
		TextPtr tnode = std::dynamic_pointer_cast<Text>(node);
		ASSERT_LOG(tnode != nullptr, "Logic error, couldn't up-cast node to Text.");
		ASSERT_LOG(parent->getDimensions().content_.width != 0, "Parent content width is 0.");

		const FixedPoint lh = getLineHeight();
		BoxPtr open = getOpenBox(parent);
		FixedPoint width = getWidthAtCursor(parent->getDimensions().content_.width) - cursor_.top().x;
		while(width == 0) {
			cursor_.top().y += lh;
			width = getWidthAtCursor(parent->getDimensions().content_.width);
			open_.top().open_box_->setContentX(getXAtCursor());
			open_.top().open_box_->setContentY(cursor_.top().y);
		}

		tnode->transformText(true);
		auto it = tnode->begin();
		while(it != tnode->end()) {
			auto saved_it = it;
			LinePtr line = tnode->reflowText(it, width);
			if(line != nullptr && !line->line.empty()) {
				// is the line larger than available space and are there floats present?
				if(line->line.back().advance.back().x > width && hasFloatsAtCursor()) {
					cursor_.top().y += lh;
					cursor_.top().x = 0;
					open_.top().open_box_->setContentX(getXAtCursor());
					open_.top().open_box_->setContentY(cursor_.top().y);
					it = saved_it;
					width = getWidthAtCursor(parent->getDimensions().content_.width) - cursor_.top().x;
					continue;
				}

				auto txt = std::make_shared<TextBox>(open, line);
				open->addChild(txt);
				if(pre_layout_fn) {
					pre_layout_fn(txt, it == tnode->end());
				}
				txt->layout(*this, open->getDimensions());
				FixedPoint x_inc = txt->getDimensions().content_.width + txt->getMBPWidth();
				cursor_.top().x += x_inc;
				width -= x_inc;
			}
			
			if((line != nullptr && line->is_end_line) || width < 0) {
				BoxPtr box = open_.top().open_box_;
				closeOpenBox();
				cursor_.top().y += box->getDimensions().content_.height;
				cursor_.top().x = 0;				
				open = getOpenBox(parent);
				width = getWidthAtCursor(parent->getDimensions().content_.width);
			}
		}
	}
		
	void LayoutEngine::pushOpenBox() 
	{
		open_.push(OpenBox());
	}
	void LayoutEngine::popOpenBox() 
	{
		open_.pop();
	}

	BoxPtr LayoutEngine::getOpenBox(BoxPtr parent) 
	{
		if(open_.empty()) {
			pushOpenBox();				
		}
		if(open_.top().open_box_ == nullptr) {
			auto anon_box = anon_block_box_.top();
			if(anon_box == nullptr) {
				open_.top().parent_ = parent;
				open_.top().open_box_ = parent->addChild(std::make_shared<LineBox>(parent, nullptr));
			} else {
				if(!anon_box->isInit()) {
					parent->addChild(anon_box);
					cursor_.top().y = 0;
					cursor_.top().x = 0;
				}
				open_.top().parent_ = anon_box;
				open_.top().open_box_ = anon_box->addChild(std::make_shared<LineBox>(anon_box, nullptr));
			}
			open_.top().open_box_->setContentX(getXAtCursor());
			open_.top().open_box_->setContentY(cursor_.top().y);
			open_.top().open_box_->setContentWidth(getWidthAtCursor(parent->getDimensions().content_.width));
		}
		return open_.top().open_box_;
	}

	void LayoutEngine::closeOpenBox() 
	{
		if(open_.empty()) {
			return;
		}
		if(open_.top().open_box_ == nullptr) {
			return;
		}
		open_.top().open_box_->layout(*this, open_.top().parent_->getDimensions());
		open_.top().open_box_ = nullptr;
	}

	FixedPoint LayoutEngine::getLineHeight() const 
	{
		const auto lh = ctx_.getComputedValue(Property::LINE_HEIGHT).getValue<Length>();
		FixedPoint line_height = lh.compute();
		if(lh.isPercent() || lh.isNumber()) {
			line_height = static_cast<FixedPoint>(line_height / getFixedPointScaleFloat()
				* ctx_.getComputedValue(Property::FONT_SIZE).getValue<Length>().compute());
		}
		return line_height;
	}

	FixedPoint LayoutEngine::getDescent() const 
	{
		return ctx_.getFontHandle()->getDescender();
	}

	const point& LayoutEngine::getCursor() const 
	{
		if(open_.empty()) {
			static point default_point;
			return default_point;
		}
		return cursor_.top(); 
	}

	void LayoutEngine::incrCursor(FixedPoint x) 
	{ 
		cursor_.top().x += x; 
	}

	FixedPoint LayoutEngine::getWidthAtCursor(FixedPoint width) const 
	{
		return getWidthAtPosition(getCursor().y + offset_.top().y, width);
	}

	FixedPoint LayoutEngine::getXAtCursor() const {
		return getXAtPosition(getCursor().y + offset_.top().y);
	}

	FixedPoint LayoutEngine::getXAtPosition(FixedPoint y) const 
	{
		FixedPoint x = 0;
		// since we expect only a small number of floats per element
		// a linear search through them seems fine at this point.
		for(auto& lf : root_->getLeftFloats()) {
			auto& dims = lf->getDimensions();
			if(y >= (lf->getMBPTop() + dims.content_.y) && y <= (lf->getMBPTop() + lf->getMBPHeight() + dims.content_.height + dims.content_.y)) {
				x = std::max(x, lf->getMBPWidth() + lf->getDimensions().content_.x + dims.content_.width);
			}
		}
		return x;
	}

	FixedPoint LayoutEngine::getX2AtPosition(FixedPoint y) const 
	{
		FixedPoint x2 = dims_.content_.width;
		// since we expect only a small number of floats per element
		// a linear search through them seems fine at this point.
		for(auto& rf : root_->getRightFloats()) {
			auto& dims = rf->getDimensions();
			if(y >= (rf->getMBPTop() + dims.content_.y) && y <= (rf->getMBPTop() + rf->getMBPHeight() + dims.content_.height + dims.content_.y)) {
				x2 = std::min(x2, rf->getMBPWidth() + dims.content_.width);
			}
		}
		return x2;
	}

	FixedPoint LayoutEngine::getWidthAtPosition(FixedPoint y, FixedPoint width) const 
	{
		// since we expect only a small number of floats per element
		// a linear search through them seems fine at this point.
		for(auto& lf : root_->getLeftFloats()) {
			auto& dims = lf->getDimensions();
			if(y >= (lf->getMBPTop() + dims.content_.y) && y <= (lf->getMBPTop() + lf->getMBPHeight() + dims.content_.height + dims.content_.y)) {
				width -= lf->getMBPWidth() + dims.content_.width;
			}
		}
		for(auto& rf : root_->getRightFloats()) {
			auto& dims = rf->getDimensions();
			if(y >= (rf->getMBPTop() + dims.content_.y) && y <= (rf->getMBPTop() + rf->getMBPHeight() + dims.content_.height + dims.content_.y)) {
				width -= rf->getMBPWidth() + dims.content_.width;
			}
		}
		return width < 0 ? 0 : width;
	}

	const point& LayoutEngine::getOffset()
	{
		ASSERT_LOG(!offset_.empty(), "There was no item on the offset stack -- programmer logic bug.");
		return offset_.top();
	}

	void LayoutEngine::moveCursorToClearFloats(CssClear float_clear)
	{
		FixedPoint new_y = cursor_.top().y;
		if(float_clear == CssClear::LEFT || float_clear == CssClear::BOTH) {
			for(auto& lf : root_->getLeftFloats()) {
				new_y = std::max(new_y, lf->getMBPHeight() + lf->getDimensions().content_.y + lf->getDimensions().content_.height);
			}
		}
		if(float_clear == CssClear::RIGHT || float_clear == CssClear::BOTH) {
			for(auto& rf : root_->getRightFloats()) {
				new_y = std::max(new_y, rf->getMBPHeight() + rf->getDimensions().content_.y + rf->getDimensions().content_.height);
			}
		}
		if(new_y != cursor_.top().y) {
			cursor_.top() = point(getXAtPosition(new_y + offset_.top().y), new_y);
		}
	}

	bool LayoutEngine::hasFloatsAtCursor() const
	{
		return hasFloatsAtPosition(cursor_.top().y + offset_.top().y);
	}

	bool LayoutEngine::hasFloatsAtPosition(FixedPoint y) const
	{
		for(auto& lf : root_->getLeftFloats()) {
			auto& dims = lf->getDimensions();
			if(y >= (lf->getMBPTop() + dims.content_.y) && y <= (lf->getMBPTop() + lf->getMBPHeight() + dims.content_.height + dims.content_.y)) {
				return true;
			}
		}
		for(auto& rf : root_->getRightFloats()) {
			auto& dims = rf->getDimensions();
			if(y >= (rf->getMBPTop() + dims.content_.y) && y <= (rf->getMBPTop() + rf->getMBPHeight() + dims.content_.height + dims.content_.y)) {
				return true;
			}
		}
		return false;
	}
}
