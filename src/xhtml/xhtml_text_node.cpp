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

#include <boost/locale.hpp>

#include "asserts.hpp"
#include "xhtml_text_node.hpp"
#include "xhtml_render_ctx.hpp"
#include "utf8_to_codepoint.hpp"
#include "unit_test.hpp"

#include "WindowManager.hpp"

namespace xhtml
{
	namespace
	{
		struct TextImpl : public Text
		{
			TextImpl(const std::string& txt, WeakDocumentPtr owner) : Text(txt, owner) {}
		};

		bool is_white_space(char32_t cp) {  return cp == '\r' || cp == '\t' || cp == ' ' || cp == '\n'; }

		Line tokenize_text(const std::string& text, bool collapse_ws, bool break_at_newline) 
		{
			Line res;
			bool in_ws = false;
			for(auto cp : utils::utf8_to_codepoint(text)) {
				if(cp == '\n' && break_at_newline) {
					if(res.empty() || !res.back().word.empty()) {
						res.emplace_back(std::string(1, '\n'));
						res.emplace_back(std::string());
					} else {
						res.back().word = "\n";
						res.emplace_back(std::string());
					}
					continue;
				}

				if(is_white_space(cp) && collapse_ws) {
					in_ws = true;
				} else {
					if(in_ws) {
						in_ws = false;
						if(!res.empty() && !res.back().word.empty()) {
							res.emplace_back(std::string());
						}
					}					
					if(res.empty()) {
						res.emplace_back(std::string());
					}
					res.back().word += utils::codepoint_to_utf8(cp);
				}
			}
			return res;
		}

	}

	Text::Text(const std::string& txt, WeakDocumentPtr owner)
		: Node(NodeId::TEXT, owner),
		  text_(txt)
	{
	}

	TextPtr Text::create(const std::string& txt, WeakDocumentPtr owner)
	{
		return std::make_shared<TextImpl>(txt, owner);
	}

	std::string Text::toString() const 
	{
		std::ostringstream ss;
		ss << "Text('" << text_ << "' " << nodeToString() << ")";
		return ss.str();
	}

	// XXX we need to add a variable to the Lines and turn it into a struct. This
	// variable is the advance per space character.
	Lines Text::generateLines(int current_line_width, int maximum_line_width)
	{
		auto parent = getParent();
		if(parent == nullptr) {
			return Lines();
		}
		ASSERT_LOG(parent != nullptr, "Can't un-parented Text node.");
		auto ctx = RenderContext::get();

		// adjust the current_line_width to be the space remaining on the line.
		long font_coord_factor = ctx.getFontHandle()->getScaleFactor();
		current_line_width = (maximum_line_width - current_line_width) * font_coord_factor;
		
		css::CssWhitespace ws = ctx.getComputedValue(css::Property::WHITE_SPACE).getValue<css::CssWhitespace>();

		// Apply transform text_ based on "text-transform" property		
		css::CssTextTransform text_transform = ctx.getComputedValue(css::Property::TEXT_TRANSFORM).getValue<css::CssTextTransform>();
		std::string transformed_text = text_;
		switch(text_transform) {
			case css::CssTextTransform::CAPITALIZE: {
				bool first_letter = true;
				transformed_text.clear();
				for(auto cp : utils::utf8_to_codepoint(text_)) {
					if(is_white_space(cp)) {
						first_letter = true;
						transformed_text += utils::codepoint_to_utf8(cp);
					} else {
						if(first_letter) {
							first_letter = false;
							transformed_text += boost::locale::to_upper(utils::codepoint_to_utf8(cp));
						} else {
							transformed_text += utils::codepoint_to_utf8(cp);
						}
					}
				}
				break;
			}
			case css::CssTextTransform::UPPERCASE:
				transformed_text = boost::locale::to_upper(text_);
				break;
			case css::CssTextTransform::LOWERCASE:
				transformed_text = boost::locale::to_lower(text_);
				break;
			case css::CssTextTransform::NONE:
			default: break;
		}

		// indicates whitespace should be collapsed together.
		bool collapse_whitespace = ws == css::CssWhitespace::NORMAL || ws == css::CssWhitespace::NOWRAP || ws == css::CssWhitespace::PRE_LINE;
		// indicates we should break at the boxes line width
		bool break_at_line = maximum_line_width >= 0 &&
			(ws == css::CssWhitespace::NORMAL || ws == css::CssWhitespace::PRE_LINE || ws == css::CssWhitespace::PRE_WRAP);
		// indicates we should break on newline characters.
		bool break_at_newline = ws == css::CssWhitespace::PRE || ws == css::CssWhitespace::PRE_LINE || ws == css::CssWhitespace::PRE_WRAP;

		// Apply letter-spacing and word-spacing here.
		auto line = xhtml::tokenize_text(transformed_text, collapse_whitespace, break_at_newline);
		long space_advance = ctx.getFontHandle()->calculateCharAdvance(' ');
		long word_spacing = static_cast<long>(ctx.getComputedValue(css::Property::WORD_SPACING).getValue<double>() * font_coord_factor);
		space_advance += word_spacing;
		long letter_spacing = static_cast<long>(ctx.getComputedValue(css::Property::LETTER_SPACING).getValue<double>() * font_coord_factor);
		space_advance += letter_spacing;

		css::CssDirection dir = ctx.getComputedValue(css::Property::DIRECTION).getValue<css::CssDirection>();
		css::CssTextAlign text_align = ctx.getComputedValue(css::Property::TEXT_ALIGN).getValue<css::CssTextAlign>();
		if(text_align == css::CssTextAlign::NORMAL) {
			text_align = dir == css::CssDirection::LTR ? css::CssTextAlign::LEFT : css::CssTextAlign::RIGHT;
		}

		// accumulator for current line lenth
		long length_acc = 0;
		
		Lines lines;
		lines.space_advance = space_advance;
		bool last_line_was_auto_break = false;
		bool is_first_line = true;
		for(auto& word : line) {
			// "\n" by itself in the word stream indicates a forced line break.
			if(word.word == "\n") {
				if(!(last_line_was_auto_break && length_acc == 0)) {
					last_line_was_auto_break = false;
					lines.lines.emplace_back(Line());
					length_acc = 0;
				}
				continue;
			}
			RenderContext::get().getFontHandle()->getGlyphPath(word.word, &word.advance);
			if(letter_spacing != 0) {
				long ls_acc = 0;
				for(auto& pt : word.advance) {
					pt.x += ls_acc;
					ls_acc += letter_spacing;
				}
			}
			// XXX we should enforce a minimum of one-word per line even if it overflows.
			if(break_at_line && length_acc + word.advance.back().x + space_advance > current_line_width) {
				// if text-align is set to justify we can add more spaces to bring the outer word aligned to the maximum_line_width
				// XXX this code is still slightly wrong as it will make the advance of the next character align with the edge
				// rather than the bounding box of the last glyph.
				if(text_align == css::CssTextAlign::JUSTIFY) {
					long space_to_add = current_line_width - length_acc;
					// only add padding if more than one word per line.
					// XXX if this is the last line we don't justify it.
					if(lines.lines.back().size() > 1) {
						space_to_add /= (lines.lines.back().size() - 1);
						for(auto& w : lines.lines.back()) {
							w.advance.back().x += space_to_add;
						}
					}
				}

				// XXX add new line to be rendered here.
				lines.lines.emplace_back(Line(1, word));
				length_acc = word.advance.back().x + space_advance;
				last_line_was_auto_break = true;
				current_line_width = maximum_line_width * font_coord_factor;
			} else {
				length_acc += word.advance.back().x + space_advance;
				// XXX render word glyph here.
				lines.lines.back().emplace_back(word);
				last_line_was_auto_break = false;
			}		
		}

		return lines;
	}
}

std::ostream& operator<<(std::ostream& os, const xhtml::Line& line) 
{
	for(auto& word : line) {
		os << word.word << " ";
	}
	return os;
}

bool operator==(const xhtml::Line& lhs, const xhtml::Line& rhs)
{
	if(lhs.size() != rhs.size()) {
		return false;
	}
	for(int n = 0; n != lhs.size(); ++n) {
		if(lhs[n].word != rhs[n].word) {
			return false;
		}
	}
	return true;
}

UNIT_TEST(text_tokenize)
{
	auto res = xhtml::tokenize_text("This \t\nis \t a \ntest \t", true, false);
	CHECK(res == xhtml::Line({xhtml::Word("This"), xhtml::Word("is"), xhtml::Word("a"), xhtml::Word("test")}), "collapse white-space test failed.");
	
	res = xhtml::tokenize_text("This \t\nis \t a \ntest \t", true, true);
	CHECK(res == xhtml::Line({xhtml::Word("This"), xhtml::Word("\n"), xhtml::Word("is"), xhtml::Word("a"), xhtml::Word("\n"), xhtml::Word("test")}), "collapse white-space+break at newline test failed.");
	
	res = xhtml::tokenize_text("This \t\nis \t a \ntest", false, false);	
	CHECK(res == xhtml::Line({xhtml::Word("This \t\nis \t a \ntest")}), "no collapse, no break at newline test failed.");
	
	res = xhtml::tokenize_text("This \t\nis \t a \ntest \t", false, true);
	CHECK(res == xhtml::Line({xhtml::Word("This \t"), xhtml::Word("\n"), xhtml::Word("is \t a "), xhtml::Word("\n"), xhtml::Word("test \t")}), "no collapse, break at newline test failed.");

	res = xhtml::tokenize_text("Lorem \n\t\n\tipsum", true, true);
	CHECK(res == xhtml::Line({xhtml::Word("Lorem"), xhtml::Word("\n"), xhtml::Word("\n"), xhtml::Word("ipsum")}), "collapse white-space test failed.");
}

