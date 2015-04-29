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
#include "css_styles.hpp"
#include "xhtml_layout.hpp"
#include "xhtml_node.hpp"
#include "xhtml_render_ctx.hpp"

namespace xhtml
{
	namespace 
	{
		std::string display_string(css::CssDisplay disp) {
			switch(disp) {
				case css::CssDisplay::BLOCK:				return "block";
				case css::CssDisplay::INLINE:				return "inline";
				case css::CssDisplay::INLINE_BLOCK:			return "inline-block";
				case css::CssDisplay::LIST_ITEM:			return "list-item";
				case css::CssDisplay::TABLE:				return "table";
				case css::CssDisplay::INLINE_TABLE:			return "inline-table";
				case css::CssDisplay::TABLE_ROW_GROUP:		return "table-row-group";
				case css::CssDisplay::TABLE_HEADER_GROUP:	return "table-header-group";
				case css::CssDisplay::TABLE_FOOTER_GROUP:	return "table-footer-group";
				case css::CssDisplay::TABLE_ROW:			return "table-row";
				case css::CssDisplay::TABLE_COLUMN_GROUP:	return "table-column-group";
				case css::CssDisplay::TABLE_COLUMN:			return "table-column";
				case css::CssDisplay::TABLE_CELL:			return "table-cell";
				case css::CssDisplay::TABLE_CAPTION:		return "table-caption";
				case css::CssDisplay::NONE:					return "none";
				default: 
					ASSERT_LOG(false, "illegal display value: " << static_cast<int>(disp));
					break;
			}
			return "none";
		}
	}

	double convert_pt_to_pixels(double pt)
	{
		return pt / 72.0 * RenderContext::get().getDPI();
	}

	LayoutBox::LayoutBox(LayoutBoxPtr parent, NodePtr node, css::CssDisplay display, DisplayListPtr display_list)
		: node_(node),
		  display_(display),
		  dimensions_(),
		  display_list_(display_list),
		  children_(),
		  fonts_(),
		  font_size_(0),
		  line_height_(0),
		  font_style_(css::FontStyle::NORMAL),
		  color_(),
		  background_color_(),
		  border_color_(),
		  border_width_(),
		  border_style_()
	{
		ASSERT_LOG(display_ != css::CssDisplay::NONE, "Tried to create a layout node with a display of none.");
		if(node != nullptr) {
			auto fonts = node->getStyle("font-family").getValue<std::vector<std::string>>();
			if(!fonts.empty()) {
				setFonts(fonts);
			}
			auto font_size = node->getStyle("font-size");
			if(font_size.shouldInherit()) {
				font_size_ = parent->font_size_;
			} else {
				font_size_ = font_size.getValue<css::FontSize>().getFontSize(parent == nullptr ? 12.0 : parent->font_size_);
			}

			auto line_height = node->getStyle("line-height");
			if(line_height.shouldInherit()) {
				line_height_ = parent->line_height_;
			} else {
				css::CssLength& len = line_height.getValue<css::CssLength>();
				if(len.isNumber() || len.isPercent()) {
					line_height_ = convert_pt_to_pixels(len.evaluate(1.0) * font_size_);
				} else {
					line_height_ = len.evaluate(0);
				}
			}

			auto font_style = node->getStyle("font-style");
			if(font_style.shouldInherit()) {
				font_style_ = parent->font_style_;
			} else {
				font_style_ = font_style.getValue<css::FontStyle>();
			}

			auto color = node->getStyle("color");
			if(color.shouldInherit()) {
				color_ = parent->color_;
			} else {
				color_ = color.getValue<css::CssColor>();
			}

			color = node->getStyle("background-color");
			if(color.shouldInherit()) {
				background_color_ = parent->background_color_;
			} else {
				background_color_ = color.getValue<css::CssColor>();
			}

			std::vector<std::string> directions = { "top", "left", "bottom", "right" };
			for(int n = 0; n != 4; ++n) {
				color = node->getStyle("border-" + directions[n] + "-color");
				if(color.shouldInherit()) {
					border_color_[n] = parent->border_color_[n];
				} else {
					border_color_[n] = color.getValue<css::CssColor>();
				}

				auto bwidth = node->getStyle("border-" + directions[n] + "-width");
				if(bwidth.shouldInherit()) {
					border_width_[n] = parent->border_width_[n];
				} else {
					border_width_[n] =  bwidth.getValue<css::CssLength>().evaluate(parent != nullptr ? parent->border_width_[n] : 0);
				}

				auto bstyle = node->getStyle("border-" + directions[n] + "-style");
				if(bwidth.shouldInherit()) {
					border_style_[n] = parent->border_style_[n];
				} else {
					border_style_[n] =  bstyle.getValue<css::BorderStyle>();
				}
			}
		}
	}

	LayoutBoxPtr LayoutBox::create(NodePtr node, DisplayListPtr display_list, LayoutBoxPtr parent)
	{
		auto display = node->getStyle("display").getValue<css::CssDisplay>();
		if(display != css::CssDisplay::NONE) {
			auto root = std::make_shared<LayoutBox>(parent, node, display, display_list);

			LayoutBoxPtr inline_container;
			for(auto& c : node->getChildren()) {
				auto disp = c->getStyle("display").getValue<css::CssDisplay>();
				if(disp == css::CssDisplay::NONE) {
					// ignore child nodes with display none.
				} else if(disp == css::CssDisplay::INLINE && root->display_ == css::CssDisplay::BLOCK) {
					if(inline_container == nullptr) {
						inline_container = std::make_shared<LayoutBox>(root, nullptr, css::CssDisplay::BLOCK, display_list);
						root->children_.emplace_back(inline_container);
					}
					inline_container->children_.emplace_back(create(c, display_list, root));
				} else {
					inline_container.reset();
					root->children_.emplace_back(create(c, display_list, root));
				}
			}

			return root;
		}
		return nullptr;
	}

	void LayoutBox::layout(const Dimensions& containing, point& offset)
	{
		// XXX need to deal with non-static positioned elements
		if(node_.lock() == nullptr) {
			// anonymous
			layoutInline(containing, offset);
			return;
		}
		switch(display_) {
			case css::CssDisplay::BLOCK:
				layoutBlock(containing);
				break;
			case css::CssDisplay::INLINE:
				layoutInline(containing, offset);
				break;
			case css::CssDisplay::INLINE_BLOCK:
			case css::CssDisplay::LIST_ITEM:
			case css::CssDisplay::TABLE:
			case css::CssDisplay::INLINE_TABLE:
			case css::CssDisplay::TABLE_ROW_GROUP:
			case css::CssDisplay::TABLE_HEADER_GROUP:
			case css::CssDisplay::TABLE_FOOTER_GROUP:
			case css::CssDisplay::TABLE_ROW:
			case css::CssDisplay::TABLE_COLUMN_GROUP:
			case css::CssDisplay::TABLE_COLUMN:
			case css::CssDisplay::TABLE_CELL:
			case css::CssDisplay::TABLE_CAPTION:
				ASSERT_LOG(false, "Implement display layout type: " << static_cast<int>(display_));
				break;
			case css::CssDisplay::NONE:
			default: 
				break;
		}
	}

	void LayoutBox::layoutBlock(const Dimensions& containing)
	{
		bool font_pushed = false;
		RenderContext& rc = RenderContext::get();
		if(!fonts_.empty() || (font_size_ != rc.getFontSize() && font_size_ != 0)) {
			double fs = font_size_;
			if(fs == 0) {
				fs = rc.getFontSize();
			}			
			// XXX we should implement this push/pop as an RAII pattern.
			if(fonts_.empty()) {
				rc.pushFont(rc.getFontName(), fs);
			} else {
				rc.pushFont(fonts_, fs);
			}
			font_pushed = true;
		}

		layoutBlockWidth(containing);
		layoutBlockPosition(containing);
		layoutBlockChildren();
		layoutBlockHeight(containing);

		if(font_pushed) {
			RenderContext::get().popFont();
		}
	}

	void LayoutBox::layoutBlockWidth(const Dimensions& containing)
	{
		auto node = node_.lock();
		// boxes without nodes are anonymous and thus dimensionless.
		if(node != nullptr) {
			double containing_width = containing.content_.w();
			auto css_width = node->getStyle("width").getValue<css::CssLength>();
			double width = css_width.evaluate(containing_width);

			dimensions_.border_.left = node->getStyle("border-left-width").getValue<css::CssLength>().evaluate(containing_width);
			dimensions_.border_.right = node->getStyle("border-right-width").getValue<css::CssLength>().evaluate(containing_width);

			dimensions_.padding_.left = node->getStyle("padding-left").getValue<css::CssLength>().evaluate(containing_width);
			dimensions_.padding_.right = node->getStyle("padding-right").getValue<css::CssLength>().evaluate(containing_width);

			auto css_margin_left = node->getStyle("margin-left").getValue<css::CssLength>();
			auto css_margin_right = node->getStyle("margin-right").getValue<css::CssLength>();
			dimensions_.margin_.left = css_margin_left.evaluate(containing_width);
			dimensions_.margin_.right = css_margin_right.evaluate(containing_width);

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
					dimensions_.margin_.right = css_margin_right.evaluate(containing_width) + underflow;
				}
				dimensions_.content_.set_w(width);
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

	void LayoutBox::layoutBlockPosition(const Dimensions& containing)
	{
		auto node = node_.lock();
		if(node != nullptr) {
			double containing_height = containing.content_.h();
			dimensions_.border_.top = node->getStyle("border-top-width").getValue<css::CssLength>().evaluate(containing_height);
			dimensions_.border_.bottom = node->getStyle("border-bottom-width").getValue<css::CssLength>().evaluate(containing_height);

			dimensions_.padding_.top = node->getStyle("padding-top").getValue<css::CssLength>().evaluate(containing_height);
			dimensions_.padding_.bottom = node->getStyle("padding-bottom").getValue<css::CssLength>().evaluate(containing_height);

			dimensions_.margin_.top = node->getStyle("margin-top").getValue<css::CssLength>().evaluate(containing_height);
			dimensions_.margin_.bottom = node->getStyle("margin-bottom").getValue<css::CssLength>().evaluate(containing_height);

			dimensions_.content_.set_x(containing.content_.x() + dimensions_.margin_.left + dimensions_.padding_.left + dimensions_.border_.left);
			dimensions_.content_.set_y(containing.content_.y2() + dimensions_.margin_.top + dimensions_.padding_.top + dimensions_.border_.top);
		} else {
			dimensions_.content_.set_xy(containing.content_.x(), containing.content_.y2());
		}
	}

	void LayoutBox::layoutBlockChildren()
	{		
		for(auto& child : children_) {
			child->layout(dimensions_, point());
			dimensions_.content_.set_h(dimensions_.content_.h()
				+ child->dimensions_.content_.h() 
				+ child->dimensions_.margin_.top + child->dimensions_.margin_.bottom
				+ child->dimensions_.padding_.top + child->dimensions_.padding_.bottom
				+ child->dimensions_.border_.top + child->dimensions_.border_.bottom);
		}
	}

	void LayoutBox::layoutBlockHeight(const Dimensions& containing)
	{
		auto node = node_.lock();
		if(node != nullptr) {
			// a set height value overrides the calculated value.
			auto h = node->getStyle("height");
			if(!h.empty()) {
				dimensions_.content_.set_h(h.getValue<css::CssLength>().evaluate(containing.content_.h()));
			}
		}		
	}

	void LayoutBox::layoutInline(const Dimensions& containing, point& offset)
	{
		bool font_pushed = false;
		RenderContext& rc = RenderContext::get();
		if(!fonts_.empty() || (font_size_ != rc.getFontSize() && font_size_ != 0)) {
			double fs = font_size_;
			if(fs == 0) {
				fs = rc.getFontSize();
			}			
			// XXX we should implement this push/pop as an RAII pattern.
			if(fonts_.empty()) {
				rc.pushFont(rc.getFontName(), fs);
			} else {
				rc.pushFont(fonts_, fs);
			}
			font_pushed = true;
		}

		layoutInlineWidth(containing, offset);
		for(auto& c : children_) {
			c->layout(containing, offset);
		}

		if(font_pushed) {
			RenderContext::get().popFont();
		}
	}

	void LayoutBox::layoutInlineWidth(const Dimensions& containing, point& offset)
	{
		KRE::FontRenderablePtr fontr = nullptr;
		auto node = node_.lock();
		if(node != nullptr && node->id() == NodeId::TEXT) {
			long font_coord_factor = RenderContext::get().getFont()->getScaleFactor();
			auto lines = node->generateLines(offset.x/font_coord_factor, containing.content_.w());

			// Generate a renderable object from the lines.
			// XXX move this to its own function.
			std::vector<geometry::Point<long>> path;
			std::string text;

			// XXX need to apply margins/padding/border.

			// XXX we need to store in RenderContext a variable specifying the max-line-height of the last line
			// to account for the case that we have mixed fonts on one line.
			// XXX also need to handle the case that we have an element with a different line height on the same line
			// oh, I suppose that is the same case I just mentioned. mea-culpa.
			// XXX I think this still needs to be passed up to a higher layer and concatenate more text flows together
			// which would allow better handling of justified text when you have elements or breaks on a line near the
			// end of a run. 
			// e.g. <span>Some text to render <em>NeedThisRenderedToday!</em> more text.</span>
			// If the screen was like this |----------------------------|
			// then the text might render  |Some text to render         |
			//                             |NeedThisRenderedToday! more |
			//                             |text.                       |
			//                             |----------------------------|
			// Where it breaks the em element on to another line, but doesn't justify the first line correctly to account 
			// for it. Yes it's a contrived example, but I saw it happen in practice. The other example is if it did
			// |----------------------------|
			// | This is a msg. me this  one| 
			// |----------------------------|
			// i.e. <span>This is a msg.<em>me</em> This one</span>
			// What happens is that the justify algoritm only applies to the text after the em tag finishes,
			// which can look kind of silly if there are only a few words being justified, when the entire 
			// rest of the line could be used.


			// since the Renderable returned from createRenderableFromPath is relative to the font baseline
			// we offset by the line-height to start.
			LOG_DEBUG("line-height: " << line_height_);
			if(offset.y == 0) {
				offset.y = static_cast<long>(line_height_ * font_coord_factor);
			}
			auto last_line = lines.lines.end()-1;
			for(auto line_it = lines.lines.begin(); line_it != lines.lines.end(); ++line_it) {
				auto& line = *line_it;
				for(auto& word : line) {
					for(auto it = word.advance.begin(); it != word.advance.end()-1; ++it) {
						path.emplace_back(it->x + offset.x, it->y + offset.y);
					}
					offset.x += word.advance.back().x + lines.space_advance;
					text += word.word;
				}
				// We exclude the last line from generating a newline.
				if(line_it != last_line) {
					offset.y += static_cast<long>(line_height_ * font_coord_factor);
					offset.x = 0;
				}
			}

			auto fh = RenderContext::get().getFont();
			fontr = fh->createRenderableFromPath(fontr, text, path);
			fontr->setColor(color_.getColor());
			display_list_->addRenderable(fontr);
		}
	}

	void LayoutBox::preOrderTraversal(std::function<void(LayoutBoxPtr, int)> fn, int nesting)
	{
		fn(shared_from_this(), nesting);
		for(auto& child : children_) {
			child->preOrderTraversal(fn, nesting+1);
		}
	}

	std::string LayoutBox::toString() const
	{
		std::stringstream ss;
		auto node = node_.lock();
		//ss << "Box(" << (node ? display_string(display_) : "anonymous") << ", content: " << dimensions_.content_ << ")";
		ss << "Box(" << (node ? display_string(display_) : "anonymous") << (node ? ", " + node->toString() : "") << ")";
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
