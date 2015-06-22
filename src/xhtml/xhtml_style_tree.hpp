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

#include <array>

#include "xhtml_node.hpp"

namespace xhtml
{
	class StyleNode;
	typedef std::shared_ptr<StyleNode> StyleNodePtr;
	typedef std::weak_ptr<StyleNode> WeakStyleNodePtr;

	class StyleNode : public std::enable_shared_from_this<StyleNode>
	{
	public:
		StyleNode(const NodePtr& node);
		NodePtr getNode() { return node_.lock(); }
		void parseNode(const NodePtr& node);
		void setStyles(std::vector<css::StylePtr>&& styles);
		static StyleNodePtr createStyleTree(const DocumentPtr& doc);
		css::StylePtr getStyle(css::Property p);

		css::BackgroundAttachment getBackgroundAttachment() const { return background_attachment_; }
		const KRE::Color& getBackgroundColor() { return background_color_; }
	private:
		void processStyles();
		WeakNodePtr node_;
		std::vector<css::StylePtr> styles_;

		//BACKGROUND_ATTACHMENT
		css::BackgroundAttachment background_attachment_;
		//BACKGROUND_COLOR
		KRE::Color background_color_;
		//BACKGROUND_IMAGE
		//std::shared_ptr<css::ImageSource> background_image_;
		//BACKGROUND_POSITION
		std::shared_ptr<css::BackgroundPosition> background_position_;
		//BACKGROUND_REPEAT
		css::BackgroundRepeat background_repeat_;
		//BORDER_TOP_COLOR / BORDER_LEFT_COLOR / BORDER_BOTTOM_COLOR / BORDER_RIGHT_COLOR
		std::array<KRE::Color, 4> border_color_;
		//BORDER_TOP_STYLE / BORDER_LEFT_STYLE / BORDER_BOTTOM_STYLE / BORDER_RIGHT_STYLE
		std::array<css::BorderStyle, 4> border_style_;
		//BORDER_TOP_WIDTH / BORDER_LEFT_WIDTH / BORDER_BOTTOM_WIDTH / BORDER_RIGHT_WIDTH
		std::array<std::shared_ptr<css::Length>, 4> border_width_;
		//TOP / LEFT / BOTTOM / RIGHT
		std::array<std::shared_ptr<css::Width>, 4> tlbr_;
		//CLEAR
		css::Clear clear_;
		//CLIP
		std::shared_ptr<css::Clip> clip_;
		//COLOR
		KRE::Color color_;
		//CONTENT
		std::shared_ptr<css::Content> content_;
		//COUNTER_INCREMENT
		std::shared_ptr<css::Counter> counter_increment_;
		//COUNTER_RESET
		std::shared_ptr<css::Counter> counter_reset_;
		//CURSOR
		std::shared_ptr<css::Cursor> cursor_;
		//DIRECTION
		css::Direction direction_;
		//DISPLAY
		css::Display display_;
		//FLOAT
		css::Float float_;
		//FONT_FAMILY / FONT_SIZE / FONT_STYLE / FONT_VARIANT / FONT_WEIGHT
		KRE::FontHandlePtr font_handle_;
		//WIDTH / HEIGHT
		std::array<std::shared_ptr<css::Width>, 2> width_height_;
		//LETTER_SPACING
		std::shared_ptr<css::Length> letter_spacing_;
		//LINE_HEIGHT
		std::shared_ptr<css::Length> line_height_;
		//LIST_STYLE_IMAGE
		//std::shared_ptr<css::ImageSource> list_style_image_;
		//LIST_STYLE_POSITION
		css::ListStylePosition list_style_position_;
		//LIST_STYLE_TYPE
		css::ListStyleType list_style_type_;
		//MARGIN_TOP / MARGIN_LEFT / MARGIN_BOTTOM / MARGIN_RIGHT
		std::array<std::shared_ptr<css::Width>, 4> margin_;
		//MIN_HEIGHT / MAX_HEIGHT
		std::array<std::shared_ptr<css::Width>, 2> minmax_height_;
		//MIN_WIDTH / MAX_WIDTH
		std::array<std::shared_ptr<css::Width>, 2> minmax_width_;
		//OUTLINE_COLOR
		KRE::Color outline_color_;
		//OUTLINE_STYLE
		css::BorderStyle outline_style_;
		//OUTLINE_WIDTH
		std::shared_ptr<css::Length> outline_width_;
		//CSS_OVERFLOW
		css::Overflow overflow_;
		//PADDING_TOP/PADDING_LEFT/PADDING_RIGHT/PADDING_BOTTOM
		std::array<std::shared_ptr<css::Length>, 4> padding_;
		//POSITION
		css::Position position_;
		//QUOTES
		std::shared_ptr<css::Quotes> quotes_;
		//TEXT_ALIGN
		css::TextAlign text_align_;
		//TEXT_DECORATION
		css::TextDecoration text_decoration_;
		//TEXT_INDENT
		std::shared_ptr<css::Width> text_indent_;
		//TEXT_TRANSFORM
		css::TextTransform text_transform_;
		//UNICODE_BIDI
		css::UnicodeBidi unicode_bidi_;
		//VERTICAL_ALIGN
		std::shared_ptr<css::VerticalAlign> vertical_align_;
		//VISIBILITY
		css::Visibility visibility_;
		//WHITE_SPACE
		css::Whitespace white_space_;
		//WORD_SPACING
		std::shared_ptr<css::Length> word_spacing_;
		//Z_INDEX
		std::shared_ptr<css::Zindex> zindex_;

		//BORDER_COLLAPSE
		//CAPTION_SIDE
		//EMPTY_CELLS
		//ORPHANS
		//TABLE_LAYOUT
		//WIDOWS

		//BOX_SHADOW
		//TEXT_SHADOW
		//TRANSITION_PROPERTY
		//TRANSITION_DURATION
		//TRANSITION_TIMING_FUNCTION
		//TRANSITION_DELAY
		//BORDER_TOP_LEFT_RADIUS
		//BORDER_TOP_RIGHT_RADIUS
		//BORDER_BOTTOM_LEFT_RADIUS
		//BORDER_BOTTOM_RIGHT_RADIUS
		//BORDER_SPACING
		//OPACITY
		//BORDER_IMAGE_SOURCE
		//BORDER_IMAGE_SLICE
		//BORDER_IMAGE_WIDTH
		//BORDER_IMAGE_OUTSET
		//BORDER_IMAGE_REPEAT
		//BACKGROUND_CLIP

	};
}
