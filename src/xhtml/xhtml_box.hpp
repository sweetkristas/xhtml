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

#pragma once

#include "xhtml_fwd.hpp"

#include "geometry.hpp"

#include "display_list.hpp"
#include "xhtml.hpp"
#include "xhtml_background_info.hpp"
#include "xhtml_border_info.hpp"
#include "xhtml_node.hpp"
#include "xhtml_render_ctx.hpp"
#include "variant_object.hpp"

namespace xhtml
{
	struct EdgeSize
	{
		EdgeSize() : left(0), top(0), right(0), bottom(0) {}
		EdgeSize(FixedPoint l, FixedPoint t, FixedPoint r, FixedPoint b) : left(l), top(t), right(r), bottom(b) {}
		FixedPoint left;
		FixedPoint top;
		FixedPoint right;
		FixedPoint bottom;
	};

	struct Dimensions
	{
		Rect content_;
		EdgeSize padding_;
		EdgeSize border_;
		EdgeSize margin_;
	};

	enum class BoxId {
		BLOCK,
		LINE,
		TEXT,
		INLINE_BLOCK,
		INLINE_ELEMENT,
		ABSOLUTE,
		FIXED,
		LIST_ITEM,
		TABLE,
	};

	struct FloatList
	{
		FloatList() : left_(), right_() {}
		std::vector<BoxPtr> left_;
		std::vector<BoxPtr> right_;
	};

	class Box : public std::enable_shared_from_this<Box>
	{
	public:
		Box(BoxId id, BoxPtr parent, NodePtr node);
		virtual ~Box() {}
		BoxId id() const { return id_; }
		const Dimensions& getDimensions() const { return dimensions_; }
		const std::vector<BoxPtr>& getChildren() const { return boxes_; }
		bool isBlockBox() const { return id_ == BoxId::BLOCK || id_ == BoxId::LIST_ITEM || id_ == BoxId::TABLE; }

		bool hasChildBlockBox() const;

		NodePtr getNode() const { return node_.lock(); }
		BoxPtr getParent() const { return parent_.lock(); }

		void addChild(BoxPtr box) { boxes_.emplace_back(box); }
		void addChildren(const std::vector<BoxPtr>& children) { boxes_.insert(boxes_.end(), children.begin(), children.end()); }
		void addAnonymousBoxes();

		void setContentRect(const Rect& r) { dimensions_.content_ = r; }
		void setContentX(FixedPoint x) { dimensions_.content_.x = x; }
		void setContentY(FixedPoint y) { dimensions_.content_.y = y; }
		void setContentWidth(FixedPoint w) { dimensions_.content_.width = w; }
		void setContentHeight(FixedPoint h) { dimensions_.content_.height = h; }

		void setPadding(const EdgeSize& e) { dimensions_.padding_ = e; }
		void setBorder(const EdgeSize& e) { dimensions_.border_ = e; }
		void setMargin(const EdgeSize& e) { dimensions_.margin_ = e; }

		void setBorderLeft(FixedPoint fp) { dimensions_.border_.left = fp; }
		void setBorderTop(FixedPoint fp) { dimensions_.border_.top = fp; }
		void setBorderRight(FixedPoint fp) { dimensions_.border_.right = fp; }
		void setBorderBottom(FixedPoint fp) { dimensions_.border_.bottom = fp; }

		void setPaddingLeft(FixedPoint fp) { dimensions_.padding_.left = fp; }
		void setPaddingTop(FixedPoint fp) { dimensions_.padding_.top = fp; }
		void setPaddingRight(FixedPoint fp) { dimensions_.padding_.right = fp; }
		void setPaddingBottom(FixedPoint fp) { dimensions_.padding_.bottom = fp; }

		void setMarginLeft(FixedPoint fp) { dimensions_.margin_.left = fp; }
		void setMarginTop(FixedPoint fp) { dimensions_.margin_.top = fp; }
		void setMarginRight(FixedPoint fp) { dimensions_.margin_.right = fp; }
		void setMarginBottom(FixedPoint fp) { dimensions_.margin_.bottom = fp; }

		void calculateVertMPB(FixedPoint containing_height);
		void calculateHorzMPB(FixedPoint containing_width);

		// These all refer to the content parameters
		FixedPoint getLeft() const { return dimensions_.content_.x; }
		FixedPoint getTop() const { return dimensions_.content_.y; }
		FixedPoint getWidth() const { return dimensions_.content_.width; }
		FixedPoint getHeight() const { return dimensions_.content_.height; }

		FixedPoint getMBPWidth() { 
			return dimensions_.margin_.left + dimensions_.margin_.right
				+ dimensions_.padding_.left + dimensions_.padding_.right
				+ dimensions_.border_.left + dimensions_.border_.right;
		}

		FixedPoint getMBPHeight() { 
			return dimensions_.margin_.top + dimensions_.margin_.bottom
				+ dimensions_.padding_.top + dimensions_.padding_.bottom
				+ dimensions_.border_.top + dimensions_.border_.bottom;
		}

		FixedPoint getMBPLeft() {
			return dimensions_.margin_.left
				+ dimensions_.padding_.left
				+ dimensions_.border_.left;
		}

		FixedPoint getMBPTop() {
			return dimensions_.margin_.top
				+ dimensions_.padding_.top
				+ dimensions_.border_.top;
		}

		FixedPoint getMBPBottom() {
			return dimensions_.margin_.bottom
				+ dimensions_.padding_.bottom
				+ dimensions_.border_.bottom;
		}

		FixedPoint getMBPRight() {
			return dimensions_.margin_.right
				+ dimensions_.padding_.right
				+ dimensions_.border_.right;
		}

		Rect getAbsBoundingBox() {
			return Rect(dimensions_.content_.x - getMBPLeft() + getOffset().x, 
				dimensions_.content_.y - getMBPTop() + getOffset().y, 
				getMBPWidth() + getWidth(),
				getMBPHeight() + getHeight());
		}

		static RootBoxPtr createLayout(NodePtr node, int containing_width, int containing_height);

		void layout(LayoutEngine& eng, const Dimensions& containing, const FloatList& floats);
		virtual std::string toString() const = 0;

		void addAbsoluteElement(LayoutEngine& eng, const Dimensions& containing, BoxPtr abs);
		void layoutAbsolute(LayoutEngine& eng, const Dimensions& containing);

		void preOrderTraversal(std::function<void(BoxPtr, int)> fn, int nesting);
		bool ancestralTraverse(std::function<bool(const ConstBoxPtr&)> fn) const;

		css::CssPosition getPosition() const { return css_position_; }
		const point& getOffset() const { return offset_; }

		void render(DisplayListPtr display_list, const point& offset) const;
		KRE::FontHandlePtr getFont() const { return font_handle_; }

		const css::Width& getCssLeft() const { return css_sides_[1]; }
		const css::Width& getCssTop() const { return css_sides_[0]; }
		const css::Width& getCssRight() const { return css_sides_[3]; }
		const css::Width& getCssBottom() const { return css_sides_[2]; }
		const css::Width& getCssWidth() const { return css_width_; }
		const css::Width& getCssHeight() const { return css_height_; }
		const css::Width& getCssMargin(css::Side n) const { return margin_[static_cast<int>(n)]; }
		const css::Length& getCssBorder(css::Side n) const { return border_[static_cast<int>(n)]; }
		const css::Length& getCssPadding(css::Side n) const { return padding_[static_cast<int>(n)]; }
		const KRE::Color& getColor() const { return color_; }

		css::CssVerticalAlign getVerticalAlign() const { return vertical_align_; }
		css::CssTextAlign getTextAlign() const { return text_align_; }

		BorderInfo& getBorderInfo() { return border_info_; }
		const BorderInfo& getBorderInfo() const { return border_info_; }

		virtual FixedPoint getBaselineOffset() const { return dimensions_.content_.height; }
		virtual FixedPoint getBottomOffset() const { return dimensions_.content_.height; }

		FixedPoint getLineHeight() const { return line_height_; }
		bool isEOL() const { return end_of_line_; }
		void setEOL(bool eol=true) { end_of_line_ = eol; }
		virtual bool isMultiline() const { return false; }
		bool isFloat() const { return cfloat_ != css::CssFloat::NONE; }
		css::CssFloat getFloatValue() const { return cfloat_; }

		void addFloat(BoxPtr box);
		const FloatList& getFloatList() const { return floats_; }

		virtual std::vector<NodePtr> getChildNodes() const;

	protected:
		void clearChildren() { boxes_.clear(); } 
		virtual void handleRenderBackground(DisplayListPtr display_list, const point& offset) const;
		virtual void handleRenderBorder(DisplayListPtr display_list, const point& offset) const;
	private:
		virtual void handleLayout(LayoutEngine& eng, const Dimensions& containing, const FloatList& floats) = 0;
		virtual void handlePreChildLayout2(LayoutEngine& eng, const Dimensions& containing, const FloatList& floats) {}
		virtual void handlePreChildLayout(LayoutEngine& eng, const Dimensions& containing, const FloatList& floats) {}
		virtual void handlePostChildLayout(LayoutEngine& eng, BoxPtr child) {}
		virtual void handleRender(DisplayListPtr display_list, const point& offset) const = 0;
		virtual void handleEndRender(DisplayListPtr display_list, const point& offset) const {}

		void setParent(BoxPtr parent) { parent_ = parent; }
		void init();

		BoxId id_;
		WeakNodePtr node_;
		std::weak_ptr<Box> parent_;
		Dimensions dimensions_;
		std::vector<BoxPtr> boxes_;
		std::vector<BoxPtr> absolute_boxes_;
		css::CssFloat cfloat_;
		KRE::FontHandlePtr font_handle_;

		BackgroundInfo background_info_;
		BorderInfo border_info_;
		css::CssPosition css_position_;

		std::array<css::Length, 4> padding_;
		std::array<css::Length,4> border_;
		std::array<css::Width, 4> margin_;

		KRE::Color color_;

		std::array<css::Width, 4> css_sides_;
		css::Width css_width_;
		css::Width css_height_;
		css::CssClear float_clear_;
		css::CssVerticalAlign vertical_align_;
		css::CssTextAlign text_align_;
		css::CssDirection css_direction_;

		point offset_;

		FixedPoint line_height_;

		// Helper marker when doing LineBox layout
		bool end_of_line_;

		FloatList floats_;
	};

	std::ostream& operator<<(std::ostream& os, const Rect& r);
}
