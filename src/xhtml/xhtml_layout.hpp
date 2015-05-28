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

#include "geometry.hpp"

#include "css_styles.hpp"
#include "display_list.hpp"
#include "xhtml.hpp"
#include "xhtml_node.hpp"
#include "xhtml_render_ctx.hpp"
#include "variant_object.hpp"

namespace xhtml
{
	class Box;
	typedef std::shared_ptr<Box> BoxPtr;
	typedef std::shared_ptr<const Box> ConstBoxPtr;
	class LayoutEngine;

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

	enum class Side {
		TOP,
		LEFT,
		BOTTOM, 
		RIGHT,
	};

	enum class BoxId {
		BLOCK,
		LINE,
		TEXT,
		INLINE_ELEMENT,
		ABSOLUTE,
		FIXED,
		LIST_ITEM,
	};

	class BackgroundInfo
	{
	public:
		BackgroundInfo();
		void setColor(const KRE::Color& color) { color_ = color; }
		void setFile(const std::string& filename);
		void setPosition(const css::BackgroundPosition& pos) { position_ = pos; }
		void setRepeat(css::CssBackgroundRepeat repeat) { repeat_ = repeat; }
		void render(DisplayListPtr display_list, const point& offset, const Dimensions& dims) const;
	private:
		KRE::Color color_;
		KRE::TexturePtr texture_;
		css::CssBackgroundRepeat repeat_;
		css::BackgroundPosition position_;
	};

	class BorderInfo
	{
	public:
		BorderInfo();
		void init(const Dimensions& dims);
		bool render(DisplayListPtr display_list, const point& offset, const Dimensions& dims) const;
		void setFile(const std::string& filename);
		void setRepeat(css::CssBorderImageRepeat horiz, css::CssBorderImageRepeat vert) { repeat_horiz_ = horiz; repeat_vert_ = vert; }
		void setWidths(const std::array<float,4>& widths) { widths_ = widths; }
		void setOutset(const std::array<float,4>& outset) { outset_ = outset; }
		void setSlice(const std::array<float,4>& slice) { slice_ = slice; }
		void setFill(bool fill) { fill_ = fill; }
	private:
		// CSS3 properties
		KRE::TexturePtr image_;
		bool fill_;
		std::array<float,4> slice_;
		std::array<float,4> outset_;
		std::array<float,4> widths_;
		css::CssBorderImageRepeat repeat_horiz_;
		css::CssBorderImageRepeat repeat_vert_;
	};

	class Box : public std::enable_shared_from_this<Box>
	{
	public:
		Box(BoxId id, BoxPtr parent, NodePtr node);
		virtual ~Box() {}
		BoxId id() const { return id_; }
		const Dimensions& getDimensions() const { return dimensions_; }
		const std::vector<BoxPtr>& getChildren() const { return boxes_; }

		virtual void init();

		NodePtr getNode() const { return node_.lock(); }
		BoxPtr getParent() const { return parent_.lock(); }

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

		void setBorderStyleTop(css::CssBorderStyle bs) { border_style_[0] = bs; }
		void setBorderStyleLeft(css::CssBorderStyle bs) { border_style_[1] = bs; }
		void setBorderStyleBottom(css::CssBorderStyle bs) { border_style_[2] = bs; }
		void setBorderStyleRight(css::CssBorderStyle bs) { border_style_[3] = bs; }

		void setBorderColorTop(const KRE::Color& color ) { border_color_[0] = color; }
		void setBorderColorLeft(const KRE::Color& color) { border_color_[1] = color; }
		void setBorderColorBottom(const KRE::Color& color) { border_color_[2] = color; }
		void setBorderColorRight(const KRE::Color& color) { border_color_[3] = color; }

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

		FixedPoint getMPBLeft() {
			return dimensions_.margin_.left
				+ dimensions_.padding_.left
				+ dimensions_.border_.left;
		}

		FixedPoint getMPBTop() {
			return dimensions_.margin_.top
				+ dimensions_.padding_.top
				+ dimensions_.border_.top;
		}

		FixedPoint getMBPBottom() {
			return dimensions_.margin_.bottom
				+ dimensions_.padding_.bottom
				+ dimensions_.border_.bottom;
		}

		static BoxPtr createLayout(NodePtr node, int containing_width, int containing_height);

		void layout(LayoutEngine& eng, const Dimensions& containing);
		virtual std::string toString() const = 0;

		BoxPtr addAbsoluteElement(NodePtr node);
		BoxPtr addFixedElement(NodePtr node);
		BoxPtr addInlineElement(NodePtr node);
		void addFloatBox(LayoutEngine& eng, BoxPtr box, css::CssFloat cfloat, FixedPoint y, const point& offset);
		void layoutAbsolute(LayoutEngine& eng, const Dimensions& containing);
		void layoutFixed(LayoutEngine& eng, const Dimensions& containing);
		
		BoxPtr addChild(BoxPtr box) { boxes_.emplace_back(box); box->init(); return box; }

		void preOrderTraversal(std::function<void(BoxPtr, int)> fn, int nesting);
		bool ancestralTraverse(std::function<bool(const ConstBoxPtr&)> fn) const;

		css::CssPosition getPosition() const { return css_position_; }
		point getOffset() const;

		void render(DisplayListPtr display_list, const point& offset) const;
		KRE::FontHandlePtr getFont() const { return font_handle_; }
		const std::vector<BoxPtr>& getLeftFloats() const { return left_floats_; }
		const std::vector<BoxPtr>& getRightFloats() const { return right_floats_; }

		const css::Width& getCssLeft() const { return css_left_; }
		const css::Width& getCssTop() const { return css_top_; }
		const css::Width& getCssWidth() const { return css_width_; }
		const css::Width& getCssHeight() const { return css_height_; }
		const css::Width& getCssMargin(Side n) const { return margin_[static_cast<int>(n)]; }
		const css::Length& getCssBorder(Side n) const { return border_[static_cast<int>(n)]; }
		const css::Length& getCssPadding(Side n) const { return padding_[static_cast<int>(n)]; }
		const css::CssBorderStyle& getCssBorderStyle(Side n) const { return border_style_[static_cast<int>(n)]; }
		const KRE::Color& getColor() const { return color_; }

	protected:
		virtual void handleRenderBackground(DisplayListPtr display_list, const point& offset) const;
		virtual void handleRenderBorder(DisplayListPtr display_list, const point& offset) const;
	private:
		virtual void handleLayout(LayoutEngine& eng, const Dimensions& containing) = 0;
		virtual void handleRender(DisplayListPtr display_list, const point& offset) const = 0;

		BoxId id_;
		WeakNodePtr node_;
		std::weak_ptr<Box> parent_;
		Dimensions dimensions_;
		std::vector<BoxPtr> boxes_;
		std::vector<BoxPtr> absolute_boxes_;
		std::vector<BoxPtr> fixed_boxes_;
		std::vector<BoxPtr> left_floats_;
		std::vector<BoxPtr> right_floats_;
		css::CssFloat cfloat_;
		KRE::FontHandlePtr font_handle_;

		BackgroundInfo binfo_;
		BorderInfo border_info_;
		css::CssPosition css_position_;

		css::Length padding_[4];
		std::array<css::Length,4> border_;
		css::Width margin_[4];

		css::CssBorderStyle border_style_[4];
		KRE::Color border_color_[4];
		KRE::Color color_;

		css::Width css_left_;
		css::Width css_top_;
		css::Width css_width_;
		css::Width css_height_;
	};

	class BlockBox : public Box
	{
	public:
		BlockBox(BoxPtr parent, NodePtr node);
		std::string toString() const override;
		void setMBP(FixedPoint containing_width);
	private:
		void layoutWidth(const Dimensions& containing);
		void layoutPosition(const Dimensions& containing);
		void layoutChildren(LayoutEngine& eng);
		void layoutHeight(const Dimensions& containing);
		void handleLayout(LayoutEngine& eng, const Dimensions& containing) override;
		void handleRender(DisplayListPtr display_list, const point& offset) const override;
	};

	class AbsoluteBox : public Box
	{
	public:
		AbsoluteBox(BoxPtr parent, NodePtr node);
		std::string toString() const override;
	private:
		void handleLayout(LayoutEngine& eng, const Dimensions& containing) override;
		void handleRender(DisplayListPtr display_list, const point& offset) const override;

		// need to store these, since by the time layout happens we no longer have the right
		// render context available.
		css::Width css_rect_[4]; // l,t,r,b
	};

	class ListItemBox : public Box
	{
	public:
		explicit ListItemBox(BoxPtr parent, NodePtr node, int count);
		std::string toString() const override;
	private:
		void handleLayout(LayoutEngine& eng, const Dimensions& containing) override;
		void handleRender(DisplayListPtr display_list, const point& offset) const override;
		std::shared_ptr<BlockBox> content_;
		int count_;
		std::string marker_;
	};

	class LineBox : public Box
	{
	public:
		LineBox(BoxPtr parent, NodePtr node);
		void init() override {}
		std::string toString() const override;
	private:
		void handleLayout(LayoutEngine& eng, const Dimensions& containing) override;
		void handleRender(DisplayListPtr display_list, const point& offset) const override;
	};

	class InlineElementBox : public Box
	{
	public:
		InlineElementBox(BoxPtr parent, NodePtr node);
		std::string toString() const override;
	private:
		void handleLayout(LayoutEngine& eng, const Dimensions& containing) override;
		void handleRender(DisplayListPtr display_list, const point& offset) const override;
	};

	class TextBox : public Box
	{
	public:
		TextBox(BoxPtr parent, LinePtr line);
		std::string toString() const override;
		void setDescent(FixedPoint descent) { descent_ = descent; }
		FixedPoint getDescent() const { return descent_; }
	private:
		void handleLayout(LayoutEngine& eng, const Dimensions& containing) override;
		void handleRender(DisplayListPtr display_list, const point& offset) const override;
		void handleRenderBackground(DisplayListPtr display_list, const point& offset) const override;
		void handleRenderBorder(DisplayListPtr display_list, const point& offset) const override;
		LinePtr line_;
		FixedPoint space_advance_;
		FixedPoint descent_;
	};

	FixedPoint convert_pt_to_pixels(double pt);
}
