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
#include "variant_object.hpp"

namespace xhtml
{
	class LayoutBox;
	typedef std::shared_ptr<LayoutBox> LayoutBoxPtr;

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

	class LayoutBox : public std::enable_shared_from_this<LayoutBox>
	{
	public:
		typedef std::vector<LayoutBoxPtr>::iterator child_iterator;
		typedef std::vector<LayoutBoxPtr>::const_iterator const_child_iterator;

		LayoutBox(LayoutBoxPtr parent, NodePtr node);
		virtual ~LayoutBox() {}
		static LayoutBoxPtr create(NodePtr node, int containing_width);
		void layout(const Dimensions& containing, FixedPoint& width);
		
		void preOrderTraversal(std::function<void(LayoutBoxPtr, int)> fn, int nesting);
		virtual std::string toString() const = 0;
		const Rect& getContentDimensions() const { return dimensions_.content_; }

		const Dimensions& getDimensions() const { return dimensions_; }
		NodePtr getNode() const { return node_.lock(); }

		void insertChildren(const_child_iterator pos, const std::vector<LayoutBoxPtr>& children);
		void setBorderStyle(Side n, css::CssBorderStyle bs) { border_style_[static_cast<int>(n)] = bs; }
		void setBorderColor(Side n, const KRE::Color& color) { border_color_[static_cast<int>(n)] = color; }
		void setBorder(Side n, FixedPoint value);
		FixedPoint getBorder(Side n);
		FixedPoint getPadding(Side n);
		css::CssBorderStyle getBorderStyle(Side n) { return border_style_[static_cast<int>(n)]; }
		const KRE::Color& getBorderColor(Side n) { return border_color_[static_cast<int>(n)]; }
		void setPadding(Side n, FixedPoint value);
		void setMargins(Side n, FixedPoint value);
		void setContentX(FixedPoint value) { dimensions_.content_.x = value; }
		void setContentY(FixedPoint value) { dimensions_.content_.y = value; }

		FixedPoint getLineHeight() const;

		void render(DisplayListPtr display_list, const point& offset) const;
		virtual point reflow(const point& offset) { return offset; }
	protected:
		std::vector<LayoutBoxPtr>& getChildren() { return children_; }
		Dimensions& getDims() { return dimensions_; }
	private:
		WeakNodePtr node_;
		std::vector<LayoutBoxPtr> children_;
		Dimensions dimensions_;

		virtual void renderBackground(DisplayListPtr display_list, const point& offset) const;
		virtual void renderBorder(DisplayListPtr display_list, const point& offset) const;
		virtual void handleRender(DisplayListPtr display_list, const point& offset) const = 0;

		virtual void handleLayout(const Dimensions& containing, FixedPoint& width) = 0;
		static LayoutBoxPtr handleCreate(NodePtr node, LayoutBoxPtr parent);
		static LayoutBoxPtr factory(NodePtr node, css::CssDisplay display, LayoutBoxPtr parent);

		std::array<css::CssBorderStyle, 4> border_style_;
		std::array<KRE::Color, 4> border_color_;

		KRE::Color background_color_;
	};

	class AnonymousLayoutBox : public LayoutBox
	{
	public:
		AnonymousLayoutBox(LayoutBoxPtr parent);
		std::string toString() const override;
	private:
		void handleLayout(const Dimensions& containing, FixedPoint& width) override;
		void handleRender(DisplayListPtr display_list, const point& offset) const override {}
		void renderBackground(DisplayListPtr display_list, const point& offset) const override {}
		void renderBorder(DisplayListPtr display_list, const point& offset) const override {}
	};
	typedef std::shared_ptr<AnonymousLayoutBox> AnonymousLayoutBoxPtr;

	class InlineLayoutBox : public LayoutBox
	{
	public:
		InlineLayoutBox(LayoutBoxPtr parent, NodePtr node);
		std::string toString() const override;
		point reflow(const point& offset) override;
	private:
		void handleLayout(const Dimensions& containing, FixedPoint& width) override;
		std::vector<LayoutBoxPtr> layoutInlineWidth(const Dimensions& containing, FixedPoint& width);
		void handleRender(DisplayListPtr display_list, const point& offset) const override;
		void renderBackground(DisplayListPtr display_list, const point& offset) const override {}
		void renderBorder(DisplayListPtr display_list, const point& offset) const override {}
		LinesPtr lines_;
	};
	
	// A child class created to handle a single line, or partial line of text.
	class InlineTextBox : public LayoutBox
	{
	public:
		InlineTextBox(LayoutBoxPtr parent, const Line& line, long space_advance);
		std::string toString() const override;
		point reflow(const point& offset) override;
	private:
		void handleLayout(const Dimensions& containing, FixedPoint& width) override;
		void handleRender(DisplayListPtr display_list, const point& offset) const override;
		Line line_;
		long space_advance_;
	};

	// Wraps a single line of InlineTextBoxes.
	class LineBox : public LayoutBox
	{
	public:
		LineBox(LayoutBoxPtr parent);
		std::string toString() const override;
		point reflow(const point& offset) override;
	private:
		void handleLayout(const Dimensions& containing, FixedPoint& width) override;
		void handleRender(DisplayListPtr display_list, const point& offset) const override;
	};

	class BlockLayoutBox : public LayoutBox
	{
	public:
		BlockLayoutBox(LayoutBoxPtr parent, NodePtr node);
		std::string toString() const override;
	private:
		void handleLayout(const Dimensions& containing, FixedPoint& width) override;
		void layoutBlockWidth(const Dimensions& containing);
		void layoutBlockPosition(const Dimensions& containing);
		void layoutBlockChildren(FixedPoint& width);
		void layoutBlockHeight(const Dimensions& containing);
		void handleRender(DisplayListPtr display_list, const point& offset) const override;
	};

	FixedPoint convert_pt_to_pixels(double pt);
}