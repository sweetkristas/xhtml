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
#include "variant_object.hpp"

namespace xhtml
{
	class LayoutBox;
	typedef std::shared_ptr<LayoutBox> LayoutBoxPtr;
	struct Lines;
	typedef std::shared_ptr<Lines> LinesPtr;

	struct EdgeSize
	{
		EdgeSize() : left(0), top(0), right(0), bottom(0) {}
		EdgeSize(double l, double t, double r, double b) : left(l), top(t), right(r), bottom(b) {}
		double left;
		double top;
		double right;
		double bottom;
	};

	struct Dimensions
	{
		geometry::Rect<double> content_;
		EdgeSize padding_;
		EdgeSize border_;
		EdgeSize margin_;
	};

	struct inline_offset
	{
		inline_offset() : line_height(0), offset() {}
		double line_height;
		point offset;
	};

	class LayoutBox : public std::enable_shared_from_this<LayoutBox>
	{
	public:
		typedef std::vector<LayoutBoxPtr>::iterator child_iterator;
		typedef std::vector<LayoutBoxPtr>::const_iterator const_child_iterator;

		LayoutBox(LayoutBoxPtr parent, NodePtr node);
		virtual ~LayoutBox() {}
		static LayoutBoxPtr create(NodePtr node, DisplayListPtr display_list, LayoutBoxPtr parent=nullptr);
		void layout(const Dimensions& containing, inline_offset& offset);
		
		void preOrderTraversal(std::function<void(LayoutBoxPtr, int)> fn, int nesting);
		virtual std::string toString() const = 0;
		const geometry::Rect<double>& getContentDimensions() const { return dimensions_.content_; }

		const Dimensions& getDimensions() const { return dimensions_; }
		NodePtr getNode() const { return node_.lock(); }

		void insertChildren(const_child_iterator pos, const std::vector<LayoutBoxPtr>& children);
	protected:
		std::vector<LayoutBoxPtr>& getChildren() { return children_; }
		Dimensions& getDims() { return dimensions_; }
	private:
		WeakNodePtr node_;
		std::vector<LayoutBoxPtr> children_;
		Dimensions dimensions_;

		virtual void handleLayout(const Dimensions& containing, inline_offset& offset) = 0;
		static LayoutBoxPtr handleCreate(NodePtr node, LayoutBoxPtr parent);
		static LayoutBoxPtr factory(NodePtr node, css::CssDisplay display, LayoutBoxPtr parent);

		std::array<CssBorderStyle, 4> border_style_;
		std::array<KRE::Color, 4> border_color_;
	};

	class AnonymousLayoutBox : public LayoutBox
	{
	public:
		AnonymousLayoutBox(LayoutBoxPtr parent);
		std::string toString() const override;
	private:
		void handleLayout(const Dimensions& containing, inline_offset& offset) override;
	};
	typedef std::shared_ptr<AnonymousLayoutBox> AnonymousLayoutBoxPtr;

	class InlineLayoutBox : public LayoutBox
	{
	public:
		InlineLayoutBox(LayoutBoxPtr parent, NodePtr node);
		std::string toString() const override;
	private:
		void handleLayout(const Dimensions& containing, inline_offset& offset) override;
		std::vector<LayoutBoxPtr> layoutInlineWidth(const Dimensions& containing, inline_offset& offset);
		LinesPtr lines_;
	};

	class BlockLayoutBox : public LayoutBox
	{
	public:
		BlockLayoutBox(LayoutBoxPtr parent, NodePtr node);
		std::string toString() const override;
	private:
		void handleLayout(const Dimensions& containing, inline_offset& offset) override;
		void layoutBlockWidth(const Dimensions& containing);
		void layoutBlockPosition(const Dimensions& containing);
		void layoutBlockChildren();
		void layoutBlockHeight(const Dimensions& containing);
	};

	double convert_pt_to_pixels(double pt);
}
