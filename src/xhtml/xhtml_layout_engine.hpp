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

		void layoutRoot(NodePtr node, BoxPtr parent, const point& container);
		
		std::vector<BoxPtr> layoutChildren(const std::vector<NodePtr>& children, BoxPtr parent, LineBoxPtr& open_box);

		FixedPoint getDescent() const;

		RootBoxPtr getRoot() const { return root_; }
		
		FixedPoint getXAtPosition(FixedPoint y) const;
		FixedPoint getX2AtPosition(FixedPoint y) const;

		FixedPoint getWidthAtPosition(FixedPoint y, FixedPoint width) const;

		bool hasFloatsAtPosition(FixedPoint y) const;

		void moveCursorToClearFloats(css::CssClear float_clear, point& cursor);

		const Dimensions& getDimensions() const { return dims_; }

		static FixedPoint getFixedPointScale() { return 65536; }
		static float getFixedPointScaleFloat() { return 65536.0f; }

		const point& getOffset();
	private:
		RootBoxPtr root_;
		Dimensions dims_;
		RenderContext& ctx_;
		
		std::stack<int> list_item_counter_;
		std::stack<point> offset_;
	};

}
