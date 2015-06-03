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

#include "to_roman.hpp"
#include "utf8_to_codepoint.hpp"

#include "xhtml_block_box.hpp"
#include "xhtml_listitem_box.hpp"
#include "xhtml_layout_engine.hpp"

namespace xhtml
{
	using namespace css;

	namespace 
	{
		const char32_t marker_disc = 0x2022;
		const char32_t marker_circle = 0x25e6;
		const char32_t marker_square = 0x25a0;
		const char32_t marker_lower_greek = 0x03b1;
		const char32_t marker_lower_greek_end = 0x03c9;
		const char32_t marker_lower_latin = 0x0061;
		const char32_t marker_lower_latin_end = 0x007a;
		const char32_t marker_upper_latin = 0x0041;
		const char32_t marker_upper_latin_end = 0x005A;
		const char32_t marker_armenian = 0x0531;
		const char32_t marker_armenian_end = 0x0556;
		const char32_t marker_georgian = 0x10d0;
		const char32_t marker_georgian_end = 0x10f6;
	}

	ListItemBox::ListItemBox(BoxPtr parent, NodePtr node, int count)
		: Box(BoxId::LIST_ITEM, parent, node),
		  content_(std::make_shared<BlockBox>(parent, node)),
		  count_(count),
		  marker_(utils::codepoint_to_utf8(marker_disc))
	{
		content_->init();
	}

	std::string ListItemBox::toString() const 
	{
		std::ostringstream ss;
		ss << "ListItemBox: " << getDimensions().content_ << "\n";
		content_->preOrderTraversal([&ss](BoxPtr box, int n) {
			std::string indent(n*2, ' ');
			ss << indent << box->toString() << "\n";
		}, 2);
		return ss.str();
	}

	void ListItemBox::handleLayout(LayoutEngine& eng, const Dimensions& containing) 
	{
		RenderContext& ctx = RenderContext::get();
		CssListStyleType lst = ctx.getComputedValue(Property::LIST_STYLE_TYPE).getValue<CssListStyleType>();
			switch(lst) {
			case CssListStyleType::DISC: /* is the default */ break;
			case CssListStyleType::CIRCLE:
				marker_ = utils::codepoint_to_utf8(marker_circle);
				break;
			case CssListStyleType::SQUARE:
				marker_ = utils::codepoint_to_utf8(marker_square);
				break;
			case CssListStyleType::DECIMAL: {
				std::ostringstream ss;
				ss << std::dec << count_ << ".";
				marker_ = ss.str();
				break;
			}
			case CssListStyleType::DECIMAL_LEADING_ZERO: {
				std::ostringstream ss;
				ss << std::dec << std::setfill('0') << std::setw(2) << count_ << ".";
				marker_ = ss.str();
				break;
			}
			case CssListStyleType::LOWER_ROMAN:
				if(count_ < 4000) {
					marker_ = to_roman(count_, true) + ".";
				}
				break;
			case CssListStyleType::UPPER_ROMAN:
				if(count_ < 4000) {
					marker_ = to_roman(count_, false) + ".";
				}
				break;
			case CssListStyleType::LOWER_GREEK:
				if(count_ <= (marker_lower_greek_end - marker_lower_greek + 1)) {
					marker_ = utils::codepoint_to_utf8(marker_lower_greek + count_) + ".";
				}
				break;
			case CssListStyleType::LOWER_ALPHA:
			case CssListStyleType::LOWER_LATIN:
				if(count_ <= (marker_lower_latin_end - marker_lower_latin + 1)) {
					marker_ = utils::codepoint_to_utf8(marker_lower_latin + count_) + ".";
				}
				break;
			case CssListStyleType::UPPER_ALPHA:
			case CssListStyleType::UPPER_LATIN:
				if(count_ <= (marker_upper_latin_end - marker_upper_latin + 1)) {
					marker_ = utils::codepoint_to_utf8(marker_upper_latin + count_) + ".";
				}
				break;
			case CssListStyleType::ARMENIAN:
				if(count_ <= (marker_armenian_end - marker_armenian + 1)) {
					marker_ = utils::codepoint_to_utf8(marker_armenian + count_) + ".";
				}
				break;
			case CssListStyleType::GEORGIAN:
				if(count_ <= (marker_georgian_end - marker_georgian + 1)) {
					marker_ = utils::codepoint_to_utf8(marker_georgian + count_) + ".";
				}
				break;
			case CssListStyleType::NONE:
			default: 
				marker_.clear();
				break;
		}
		content_->layout(eng, containing);
		
		setContentX(content_->getDimensions().content_.x);
		setContentY(content_->getDimensions().content_.y);
		setContentWidth(content_->getDimensions().content_.width);
		setContentHeight(content_->getDimensions().content_.height);
	}

	void ListItemBox::handleReLayout(LayoutEngine& eng, const Dimensions& containing) 
	{
		ASSERT_LOG(false, "XXX ListItemBox::handleReLayout()");
	}

	void ListItemBox::handleRender(DisplayListPtr display_list, const point& offset) const 
	{
		auto& ctx = RenderContext::get();

		auto path = getFont()->getGlyphPath(marker_);
		std::vector<point> new_path;
		FixedPoint path_width = path.back().x - path.front().x + getFont()->calculateCharAdvance(' ');
		// XXX should figure out if there is a cleaner way of doing this, basically we want the list marker to be offset by the 
		// content's first child's position.
		auto y = 0;
		//if(content_->getChildren().size() > 0) {
		//	y = content_->getChildren().front()->getDimensions().content_.y;
		//}
		for(auto& p : path) {
			new_path.emplace_back(p.x + offset.x - 5 - path_width, p.y + offset.y + y);
		}
		auto fontr = getFont()->createRenderableFromPath(nullptr, marker_, new_path);
		fontr->setColor(getColor());
		//fontr->setPosition(static_cast<float>(offset.x - path_width - 5) / fixed_point_scale_float, static_cast<float>(offset.y) / fixed_point_scale_float);
		display_list->addRenderable(fontr);

		// XXX
		content_->render(display_list, offset);
	}

}
