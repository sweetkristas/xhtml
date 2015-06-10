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
#include "xhtml_absolute_box.hpp"
#include "xhtml_block_box.hpp"
#include "xhtml_inline_block_box.hpp"
#include "xhtml_inline_element_box.hpp"
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
		  list_item_counter_(),
		  offset_()
	{
		list_item_counter_.emplace(0);
		offset_.emplace(point());
	}

	void LayoutEngine::layoutRoot(NodePtr node, BoxPtr parent, const point& container) 
	{
		if(root_ == nullptr) {
			RenderContext::Manager ctx_manager(node->getProperties());

			root_ = std::make_shared<RootBox>(nullptr, node);
			dims_.content_ = Rect(0, 0, container.x, 0);
			root_->layout(*this, dims_);
			return;
		}
	}
	
	std::vector<BoxPtr> LayoutEngine::layoutChildren(const std::vector<NodePtr>& children, BoxPtr parent, LineBoxPtr& open_box)
	{
		StackManager<point> offset_manager(offset_, point(parent->getLeft(), parent->getTop()) + offset_.top());

		std::vector<BoxPtr> res;
		for(auto it = children.begin(); it != children.end(); ++it) {
			auto child = *it;
			if(child->id() == NodeId::ELEMENT) {
				RenderContext::Manager ctx_manager(child->getProperties());

				// Adjust counters for list items as needed
				std::unique_ptr<StackManager<int>> li_manager;
				if(child->hasTag(ElementId::UL) || child->hasTag(ElementId::OL)) {
					li_manager.reset(new StackManager<int>(list_item_counter_, 0));
				}
				if(child->hasTag(ElementId::LI) ) {
					auto &top = list_item_counter_.top();
					++top;
				}

				const CssDisplay display = ctx_.getComputedValue(Property::DISPLAY).getValue<CssDisplay>();
				const CssFloat cfloat = ctx_.getComputedValue(Property::FLOAT).getValue<CssFloat>();
				const CssPosition position = ctx_.getComputedValue(Property::POSITION).getValue<CssPosition>();

				if(display == CssDisplay::NONE) {
					// Do not create a box for this or it's children
					// early return
					continue;
				}

				if(position == CssPosition::ABSOLUTE) {
					// absolute positioned elements are taken out of the normal document flow
					parent->addAbsoluteElement(std::make_shared<AbsoluteBox>(parent, child));
				} else if(position == CssPosition::FIXED) {
					// fixed positioned elements are taken out of the normal document flow
					root_->addFixed(std::make_shared<BlockBox>(parent, child));
				} else {
					if(cfloat != CssFloat::NONE) {
						// XXX need to add an offset to position for the float box based on body margin.
						// N.B. if the current display is one of the CssDisplay::TABLE* styles then this should be
						// a table box rather than a block box.
						root_->addFloatBox(*this, std::make_shared<BlockBox>(root_, child), cfloat, offset_.top().y + (open_box != nullptr ? open_box->getCursor().y : 0));
						continue;
					}
					switch(display) {
						case CssDisplay::NONE:
							// Do not create a box for this or it's children
							break;
						case CssDisplay::INLINE: {
							if(child->isReplaced()) {
								// replaced elements should generate a box.
								// XXX should these go into open_box?
								res.emplace_back(std::make_shared<InlineElementBox>(parent, child));
							} else {
								// non-replaced elements we just generate children and add them.
								std::vector<BoxPtr> new_children = layoutChildren(child->getChildren(), parent, open_box);
								res.insert(res.end(), new_children.begin(), new_children.end());
							}
							break;
						}
						case CssDisplay::BLOCK: {
							if(open_box) {
								if(!open_box->getChildren().empty()) {
									res.emplace_back(open_box);
								}
								open_box.reset();
							}
							res.emplace_back(std::make_shared<BlockBox>(parent, child));
							break;
						}
						case CssDisplay::INLINE_BLOCK: {
							auto ibb = std::make_shared<InlineBlockBox>(parent, child);
							ibb->layout(*this, parent->getDimensions());
							open_box->addChild(ibb);
							break;
						}
						case CssDisplay::LIST_ITEM: {
							if(open_box) {
								if(!open_box->getChildren().empty()) {
									res.emplace_back(open_box);
								}
								open_box.reset();
							}
							res.emplace_back(std::make_shared<ListItemBox>(parent, child, list_item_counter_.top()));
							break;
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
			} else if(child->id() == NodeId::TEXT) {
				//const static PropertyList empty;
				//RenderContext::Manager ctx_manager(empty);
				TextPtr tnode = std::dynamic_pointer_cast<Text>(child);
				ASSERT_LOG(tnode != nullptr, "Logic error, couldn't up-cast node to Text.");

				tnode->transformText(true);
				if(open_box == nullptr) {
					open_box = std::make_shared<LineBox>(parent);
				}
				auto txt = std::make_shared<TextBox>(open_box, tnode);
				open_box->addChild(txt);
			} else {
				ASSERT_LOG(false, "Unhandled node id, only elements and text can be used in layout: " << static_cast<int>(child->id()));
			}
		}
		return res;
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

	void LayoutEngine::moveCursorToClearFloats(CssClear float_clear, point& cursor)
	{
		FixedPoint new_y = cursor.y;
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
		if(new_y != cursor.y) {
			cursor = point(getXAtPosition(new_y + offset_.top().y), new_y);
		}
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
