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

#include <stack>

#include "xhtml_box.hpp"
#include "xhtml_render_ctx.hpp"

namespace xhtml
{
	class LayoutEngine
	{
	public:
		explicit LayoutEngine();

		void formatNode(NodePtr node, BoxPtr parent, const point& container);
		BoxPtr formatNode(NodePtr node, BoxPtr parent, const Dimensions& container, std::function<void(BoxPtr, bool)> pre_layout_fn=nullptr);

		void layoutInlineElement(NodePtr node, BoxPtr parent, std::function<void(BoxPtr, bool)> pre_layout_fn);

		std::vector<css::CssBorderStyle> generateBorderStyle();
		std::vector<KRE::Color> generateBorderColor();
		std::vector<FixedPoint> generateBorderWidth();
		std::vector<FixedPoint> generatePadding();

		void layoutInlineText(NodePtr node, BoxPtr parent, std::function<void(BoxPtr, bool)> pre_layout_fn);
		void pushOpenBox();
		void popOpenBox();

		BoxPtr getOpenBox(BoxPtr parent);
		void closeOpenBox();

		FixedPoint getLineHeight() const;

		FixedPoint getDescent() const;

		bool isOpenBox() const { return !open_.empty() && open_.top().open_box_ != nullptr; }
		RootBoxPtr getRoot() const { return root_; }
		
		const point& getCursor() const;
		void incrCursor(FixedPoint x);
		void pushNewCursor();
		void popCursor();

		FixedPoint getWidthAtCursor(FixedPoint width) const;
		FixedPoint getXAtCursor() const;
		FixedPoint getXAtPosition(FixedPoint y) const;
		FixedPoint getX2AtPosition(FixedPoint y) const;

		FixedPoint getWidthAtPosition(FixedPoint y, FixedPoint width) const;

		bool hasFloatsAtCursor() const;
		bool hasFloatsAtPosition(FixedPoint y) const;

		void moveCursorToClearFloats(css::CssClear float_clear);

		const Dimensions& getDimensions() const { return dims_; }

		static FixedPoint getFixedPointScale() { return 65536; }
		static float getFixedPointScaleFloat() { return 65536.0f; }

		const point& getOffset();

		BoxPtr getAnonBox() const { return !anon_block_box_.empty() ? anon_block_box_.top() : nullptr; }
	private:
		RootBoxPtr root_;
		Dimensions dims_;
		RenderContext& ctx_;
		struct OpenBox {
			OpenBox() : open_box_(), parent_(nullptr) {}
			OpenBox(BoxPtr bp, point cursor) : open_box_(bp), parent_(nullptr) {}
			BoxPtr open_box_;
			BoxPtr parent_;
		};
		std::stack<point> cursor_;
		std::stack<OpenBox> open_;

		std::stack<int> list_item_counter_;
		std::stack<point> offset_;
		// Additional anonymous block box that may be needed during layout.
		std::stack<BoxPtr> anon_block_box_;
	};

}
