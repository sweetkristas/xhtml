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
#include "xhtml_text_node.hpp"

namespace xhtml
{
	TextBox::TextBox(BoxPtr parent, StyleNodePtr node)
		: Box(BoxId::TEXT, parent, node),
		  line_(),
		  txt_(std::dynamic_pointer_cast<Text>(node->getNode())),
		  it_(),
		  justification_(0)
	{
	}

	std::string TextBox::toString() const
	{
		std::ostringstream ss;
		ss << "TextBox: " << getDimensions().content_;
		for(auto& word : line_->line) {
			ss << " " << word.word;
		}
		if(isEOL()) {
			ss << " ; end-of-line";
		}
		return ss.str();
	}

	Text::iterator TextBox::reflow(LayoutEngine& eng, point& cursor, Text::iterator it)
	{
		it_ = it;
		auto parent = getParent();
		FixedPoint y1 = cursor.y + getParent()->getOffset().y;
		FixedPoint width = eng.getWidthAtPosition(y1, y1 + getLineHeight(), parent->getWidth()) - cursor.x + eng.getXAtPosition(y1, y1 + getLineHeight());

		ASSERT_LOG(it != txt_->end(), "Given an iterator at end of text.");

		bool done = false;
		while(!done) {
			LinePtr line = txt_->reflowText(it, width, getStyleNode()->getFont());
			if(line != nullptr && !line->line.empty()) {
				// is the line larger than available space and are there floats present?
				if(line->line.back().advance.back().x > width && eng.hasFloatsAtPosition(y1, y1 + getLineHeight())) {
					cursor.y += getLineHeight();
					y1 = cursor.y + getParent()->getOffset().y;
					cursor.x = eng.getXAtPosition(y1, y1 + getLineHeight());
					it = it_;
					width = eng.getWidthAtPosition(y1, y1 + getLineHeight(), parent->getWidth());
					continue;
				}
				line_ = line;
			}
			
			done = true;
		}

		if(line_ != nullptr) {
			setContentWidth(calculateWidth());
		}

		return it;
	}

	Text::iterator TextBox::end()
	{
		return txt_->end();
	}

	FixedPoint TextBox::calculateWidth() const
	{
		ASSERT_LOG(line_ != nullptr, "Calculating width of TextBox with no line_ (=nullptr).");
		FixedPoint width = 0;
		for(auto& word : line_->line) {
			width += word.advance.back().x;
		}
		width += line_->space_advance * line_->line.size();
		return width;
	}

	void TextBox::handleLayout(LayoutEngine& eng, const Dimensions& containing)
	{
		// TextBox's have no children to deal with, by definition.	
		//setContentWidth(calculateWidth());
		setContentHeight(getLineHeight());

		calculateHorzMPB(containing.content_.width);
		calculateVertMPB(containing.content_.height);
	}

	void TextBox::justify(FixedPoint containing_width)
	{
		int word_count = line_->line.size() - 1;
		if(word_count <= 2) {
			return;
		}
		justification_ = (containing_width - calculateWidth()) / word_count;
	}

	void TextBox::handleRenderBackground(DisplayListPtr display_list, const point& offset) const
	{
		point offs = offset - point(0, getDimensions().content_.height);
		Box::handleRenderBackground(display_list, offs);
	}

	void TextBox::handleRenderBorder(DisplayListPtr display_list, const point& offset) const
	{
		point offs = offset - point(0, getDimensions().content_.height);
		Box::handleRenderBorder(display_list, offs);
	}

	void TextBox::handleRender(DisplayListPtr display_list, const point& offset) const
	{
		ASSERT_LOG(line_ != nullptr, "TextBox has not had layout done. line_ is nullptr");
		std::vector<point> path;
		std::string text;
		int dim_x = offset.x;
		int dim_y = offset.y + getStyleNode()->getFont()->getDescender();
		for(auto& word : line_->line) {
			for(auto it = word.advance.begin(); it != word.advance.end()-1; ++it) {
				path.emplace_back(it->x + dim_x, it->y + dim_y);
			}
			dim_x += word.advance.back().x + line_->space_advance + justification_;
			text += word.word;
		}
		
		auto& ctx = RenderContext::get();
		auto fontr = getStyleNode()->getFont()->createRenderableFromPath(nullptr, text, path);
		fontr->setColor(getStyleNode()->getColor());
		display_list->addRenderable(fontr);
	}
}
