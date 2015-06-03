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
#include "xhtml_text_box.hpp"

namespace xhtml
{
	TextBox::TextBox(BoxPtr parent, LinePtr line)
		: Box(BoxId::TEXT, parent, nullptr),
		  line_(line),
		  space_advance_(line->space_advance)
	{
	}

	std::string TextBox::toString() const
	{
		std::ostringstream ss;
		ss << "TextBox: " << getDimensions().content_;
		for(auto& word : line_->line) {
			ss << " " << word.word;
		}
		return ss.str();
	}

	void TextBox::handleLayout(LayoutEngine& eng, const Dimensions& containing)
	{
		FixedPoint width = 0;
		for(auto& word : line_->line) {
			width += word.advance.back().x;
		}
		width += space_advance_ * (line_->line.size()-1);
		setContentWidth(width);

		const FixedPoint lh = eng.getLineHeight();
		setContentX(eng.getCursor().x + getMBPLeft());
		setContentY(lh);
		setContentHeight(lh);
	}

	void TextBox::handleRenderBackground(DisplayListPtr display_list, const point& offset) const
	{
		//point offs = offset - point(0, getDimensions().content_.height + getFont()->getDescender());		
		//Box::handleRenderBackground(display_list, offs);
	}

	void TextBox::handleRenderBorder(DisplayListPtr display_list, const point& offset) const
	{
		point offs = offset - point(0, getDimensions().content_.height);
		Box::handleRenderBorder(display_list, offs);
	}

	void TextBox::handleRender(DisplayListPtr display_list, const point& offset) const
	{
		std::vector<point> path;
		std::string text;
		int dim_x = offset.x;
		int dim_y = offset.y + getFont()->getDescender();
		for(auto& word : line_->line) {
			for(auto it = word.advance.begin(); it != word.advance.end()-1; ++it) {
				path.emplace_back(it->x + dim_x, it->y + dim_y);
			}
			dim_x += word.advance.back().x + space_advance_;
			text += word.word;
		}
		
		auto& ctx = RenderContext::get();
		auto fontr = getFont()->createRenderableFromPath(nullptr, text, path);
		fontr->setColor(getColor());
		display_list->addRenderable(fontr);
	}
}
