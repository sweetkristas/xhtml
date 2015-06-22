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

#include "xhtml_render_ctx.hpp"
#include "xhtml_style_tree.hpp"

namespace xhtml
{
	using namespace css;

	StyleNode::StyleNode(const NodePtr& node)
		: node_(node)
	{
	}

	void StyleNode::parseNode(const NodePtr& node)
	{
		std::unique_ptr<RenderContext::Manager> rcm;
		if(node->id() == NodeId::ELEMENT) {
			rcm.reset(new RenderContext::Manager(node->getProperties()));
		}
		StyleNodePtr style_child = std::make_shared<StyleNode>(node);
		style_child->setStyles(RenderContext::get().getCurrentStyles());

		for(auto& child : node->getChildren()) {
			style_child->parseNode(child);
		}
	}

	void StyleNode::setStyles(std::vector<css::StylePtr>&& styles)
	{
		styles_ = styles;
		processStyles();
	}

	void StyleNode::processStyles()
	{
		background_attachment_ = getStyle(Property::BACKGROUND_ATTACHMENT)->getEnum<BackgroundAttachment>();
		background_color_ = getStyle(Property::BACKGROUND_COLOR)->asType<CssColor>()->compute();
		//background_image_ = getStyle(Property::BACKGROUND_IMAGE)->asType<ImageSource>();
		background_position_ = getStyle(Property::BACKGROUND_POSITION)->asType<BackgroundPosition>();
		background_repeat_ = getStyle(Property::BACKGROUND_REPEAT)->getEnum<BackgroundRepeat>();
		border_color_[0] = getStyle(Property::BORDER_TOP_COLOR)->asType<CssColor>()->compute();
		border_color_[1] = getStyle(Property::BORDER_LEFT_COLOR)->asType<CssColor>()->compute();
		border_color_[2] = getStyle(Property::BORDER_BOTTOM_COLOR)->asType<CssColor>()->compute();
		border_color_[3] = getStyle(Property::BORDER_RIGHT_COLOR)->asType<CssColor>()->compute();
		border_style_[0] = getStyle(Property::BORDER_TOP_STYLE)->getEnum<BorderStyle>();
		border_style_[1] = getStyle(Property::BORDER_LEFT_STYLE)->getEnum<BorderStyle>();
		border_style_[2] = getStyle(Property::BORDER_BOTTOM_STYLE)->getEnum<BorderStyle>();
		border_style_[3] = getStyle(Property::BORDER_RIGHT_STYLE)->getEnum<BorderStyle>();
		border_width_[0] = getStyle(Property::BORDER_TOP_WIDTH)->asType<Length>();
		border_width_[1] = getStyle(Property::BORDER_LEFT_WIDTH)->asType<Length>();
		border_width_[2] = getStyle(Property::BORDER_BOTTOM_WIDTH)->asType<Length>();
		border_width_[3] = getStyle(Property::BORDER_RIGHT_WIDTH)->asType<Length>();
		tlbr_[0] = getStyle(Property::TOP)->asType<Width>();
		tlbr_[1] = getStyle(Property::LEFT)->asType<Width>();
		tlbr_[2] = getStyle(Property::BOTTOM)->asType<Width>();
		tlbr_[3] = getStyle(Property::RIGHT)->asType<Width>();
		clear_ = getStyle(Property::CLEAR)->getEnum<Clear>();
		clip_ = getStyle(Property::CLIP)->asType<Clip>();
		color_ = getStyle(Property::COLOR)->asType<CssColor>()->compute();
		content_ = getStyle(Property::CONTENT)->asType<Content>();
		counter_increment_ = getStyle(Property::COUNTER_INCREMENT)->asType<Counter>();
		counter_reset_ = getStyle(Property::COUNTER_RESET)->asType<Counter>();
		cursor_ = getStyle(Property::CURSOR)->asType<Cursor>();
		direction_ = getStyle(Property::DIRECTION)->getEnum<Direction>();
		display_ = getStyle(Property::DISPLAY)->getEnum<Display>();
		float_ = getStyle(Property::FLOAT)->getEnum<Float>();
		font_handle_ = RenderContext::get().getFontHandle();
		width_height_[0] = getStyle(Property::WIDTH)->asType<Width>();
		width_height_[1] = getStyle(Property::HEIGHT)->asType<Width>();
		letter_spacing_ = getStyle(Property::LETTER_SPACING)->asType<Length>();
		line_height_ = getStyle(Property::LINE_HEIGHT)->asType<Length>();
		//list_style_image_ = getStyle(Property::LIST_STYLE_IMAGE)->asType<ImageSource>();
		list_style_position_ = getStyle(Property::LIST_STYLE_POSITION)->getEnum<ListStylePosition>();
		list_style_type_ = getStyle(Property::LIST_STYLE_TYPE)->getEnum<ListStyleType>();
		margin_[0] = getStyle(Property::MARGIN_TOP)->asType<Width>();
		margin_[1] = getStyle(Property::MARGIN_LEFT)->asType<Width>();
		margin_[2] = getStyle(Property::MARGIN_BOTTOM)->asType<Width>();
		margin_[3] = getStyle(Property::MARGIN_RIGHT)->asType<Width>();
		minmax_height_[0] = getStyle(Property::MIN_HEIGHT)->asType<Width>();
		minmax_height_[1] = getStyle(Property::MAX_HEIGHT)->asType<Width>();
		minmax_width_[0] = getStyle(Property::MIN_WIDTH)->asType<Width>();
		minmax_width_[1] = getStyle(Property::MAX_WIDTH)->asType<Width>();
		outline_color_ = getStyle(Property::OUTLINE_COLOR)->asType<CssColor>()->compute();
		outline_style_ = getStyle(Property::OUTLINE_STYLE)->getEnum<BorderStyle>();
		outline_width_ = getStyle(Property::OUTLINE_WIDTH)->asType<Length>();
		overflow_ = getStyle(Property::CSS_OVERFLOW)->getEnum<Overflow>();
		padding_[0] = getStyle(Property::PADDING_TOP)->asType<Length>();
		padding_[1] = getStyle(Property::PADDING_LEFT)->asType<Length>();
		padding_[2] = getStyle(Property::PADDING_BOTTOM)->asType<Length>();
		padding_[3] = getStyle(Property::PADDING_RIGHT)->asType<Length>();
		position_ = getStyle(Property::POSITION)->getEnum<Position>();
		quotes_ = getStyle(Property::QUOTES)->asType<Quotes>();
		text_align_ = getStyle(Property::TEXT_ALIGN)->getEnum<TextAlign>();
		text_decoration_ = getStyle(Property::TEXT_DECORATION)->getEnum<TextDecoration>();
		text_indent_ = getStyle(Property::TEXT_INDENT)->asType<Width>();
		text_transform_ = getStyle(Property::TEXT_TRANSFORM)->getEnum<TextTransform>();
		unicode_bidi_ = getStyle(Property::UNICODE_BIDI)->getEnum<UnicodeBidi>();
		visibility_ = getStyle(Property::VISIBILITY)->getEnum<Visibility>();
		white_space_ = getStyle(Property::WHITE_SPACE)->getEnum<Whitespace>();
		vertical_align_ = getStyle(Property::VERTICAL_ALIGN)->asType<VerticalAlign>();
		word_spacing_ = getStyle(Property::WORD_SPACING)->asType<Length>();
		zindex_ = getStyle(Property::Z_INDEX)->asType<Zindex>();
	}

	StyleNodePtr StyleNode::createStyleTree(const DocumentPtr& doc)
	{
		StyleNodePtr root = std::make_shared<StyleNode>(doc);
		for(auto& child : doc->getChildren()) {
			root->parseNode(child);
		}
		return root;
	}

	css::StylePtr StyleNode::getStyle(css::Property p)
	{
		int ndx = static_cast<int>(p);
		ASSERT_LOG(ndx < static_cast<int>(styles_.size()), "Index in property list: " << ndx << " is outside of legal bounds: 0-" << (styles_.size()-1));
		return styles_[ndx];
	}

}
