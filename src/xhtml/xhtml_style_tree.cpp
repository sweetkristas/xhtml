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
		style_child->processStyles();

		for(auto& child : node->getChildren()) {
			style_child->parseNode(child);
		}
	}

	void StyleNode::processStyles()
	{
		RenderContext& ctx = RenderContext::get();
		background_attachment_ = ctx.getComputedValue(Property::BACKGROUND_ATTACHMENT)->getEnum<BackgroundAttachment>();
		background_color_ = ctx.getComputedValue(Property::BACKGROUND_COLOR)->asType<CssColor>()->compute();
		//background_image_ = ctx.getComputedValue(Property::BACKGROUND_IMAGE)->asType<ImageSource>();
		auto bp = ctx.getComputedValue(Property::BACKGROUND_POSITION)->asType<BackgroundPosition>();
		background_position_[0] = bp->getTop();
		background_position_[1] = bp->getLeft();
		background_repeat_ = ctx.getComputedValue(Property::BACKGROUND_REPEAT)->getEnum<BackgroundRepeat>();
		border_color_[0] = ctx.getComputedValue(Property::BORDER_TOP_COLOR)->asType<CssColor>()->compute();
		border_color_[1] = ctx.getComputedValue(Property::BORDER_LEFT_COLOR)->asType<CssColor>()->compute();
		border_color_[2] = ctx.getComputedValue(Property::BORDER_BOTTOM_COLOR)->asType<CssColor>()->compute();
		border_color_[3] = ctx.getComputedValue(Property::BORDER_RIGHT_COLOR)->asType<CssColor>()->compute();
		border_style_[0] = ctx.getComputedValue(Property::BORDER_TOP_STYLE)->getEnum<BorderStyle>();
		border_style_[1] = ctx.getComputedValue(Property::BORDER_LEFT_STYLE)->getEnum<BorderStyle>();
		border_style_[2] = ctx.getComputedValue(Property::BORDER_BOTTOM_STYLE)->getEnum<BorderStyle>();
		border_style_[3] = ctx.getComputedValue(Property::BORDER_RIGHT_STYLE)->getEnum<BorderStyle>();
		border_width_[0] = ctx.getComputedValue(Property::BORDER_TOP_WIDTH)->asType<Length>();
		border_width_[1] = ctx.getComputedValue(Property::BORDER_LEFT_WIDTH)->asType<Length>();
		border_width_[2] = ctx.getComputedValue(Property::BORDER_BOTTOM_WIDTH)->asType<Length>();
		border_width_[3] = ctx.getComputedValue(Property::BORDER_RIGHT_WIDTH)->asType<Length>();
		tlbr_[0] = ctx.getComputedValue(Property::TOP)->asType<Width>();
		tlbr_[1] = ctx.getComputedValue(Property::LEFT)->asType<Width>();
		tlbr_[2] = ctx.getComputedValue(Property::BOTTOM)->asType<Width>();
		tlbr_[3] = ctx.getComputedValue(Property::RIGHT)->asType<Width>();
		clear_ = ctx.getComputedValue(Property::CLEAR)->getEnum<Clear>();
		clip_ = ctx.getComputedValue(Property::CLIP)->asType<Clip>();
		color_ = ctx.getComputedValue(Property::COLOR)->asType<CssColor>()->compute();
		content_ = ctx.getComputedValue(Property::CONTENT)->asType<Content>();
		counter_increment_ = ctx.getComputedValue(Property::COUNTER_INCREMENT)->asType<Counter>();
		counter_reset_ = ctx.getComputedValue(Property::COUNTER_RESET)->asType<Counter>();
		cursor_ = ctx.getComputedValue(Property::CURSOR)->asType<Cursor>();
		direction_ = ctx.getComputedValue(Property::DIRECTION)->getEnum<Direction>();
		display_ = ctx.getComputedValue(Property::DISPLAY)->getEnum<Display>();
		float_ = ctx.getComputedValue(Property::FLOAT)->getEnum<Float>();
		font_handle_ = RenderContext::get().getFontHandle();
		width_height_[0] = ctx.getComputedValue(Property::WIDTH)->asType<Width>();
		width_height_[1] = ctx.getComputedValue(Property::HEIGHT)->asType<Width>();
		letter_spacing_ = ctx.getComputedValue(Property::LETTER_SPACING)->asType<Length>();
		line_height_ = ctx.getComputedValue(Property::LINE_HEIGHT)->asType<Length>();
		//list_style_image_ = ctx.getComputedValue(Property::LIST_STYLE_IMAGE)->asType<ImageSource>();
		list_style_position_ = ctx.getComputedValue(Property::LIST_STYLE_POSITION)->getEnum<ListStylePosition>();
		list_style_type_ = ctx.getComputedValue(Property::LIST_STYLE_TYPE)->getEnum<ListStyleType>();
		margin_[0] = ctx.getComputedValue(Property::MARGIN_TOP)->asType<Width>();
		margin_[1] = ctx.getComputedValue(Property::MARGIN_LEFT)->asType<Width>();
		margin_[2] = ctx.getComputedValue(Property::MARGIN_BOTTOM)->asType<Width>();
		margin_[3] = ctx.getComputedValue(Property::MARGIN_RIGHT)->asType<Width>();
		minmax_height_[0] = ctx.getComputedValue(Property::MIN_HEIGHT)->asType<Width>();
		minmax_height_[1] = ctx.getComputedValue(Property::MAX_HEIGHT)->asType<Width>();
		minmax_width_[0] = ctx.getComputedValue(Property::MIN_WIDTH)->asType<Width>();
		minmax_width_[1] = ctx.getComputedValue(Property::MAX_WIDTH)->asType<Width>();
		outline_color_ = ctx.getComputedValue(Property::OUTLINE_COLOR)->asType<CssColor>()->compute();
		outline_style_ = ctx.getComputedValue(Property::OUTLINE_STYLE)->getEnum<BorderStyle>();
		outline_width_ = ctx.getComputedValue(Property::OUTLINE_WIDTH)->asType<Length>();
		overflow_ = ctx.getComputedValue(Property::CSS_OVERFLOW)->getEnum<Overflow>();
		padding_[0] = ctx.getComputedValue(Property::PADDING_TOP)->asType<Length>();
		padding_[1] = ctx.getComputedValue(Property::PADDING_LEFT)->asType<Length>();
		padding_[2] = ctx.getComputedValue(Property::PADDING_BOTTOM)->asType<Length>();
		padding_[3] = ctx.getComputedValue(Property::PADDING_RIGHT)->asType<Length>();
		position_ = ctx.getComputedValue(Property::POSITION)->getEnum<Position>();
		quotes_ = ctx.getComputedValue(Property::QUOTES)->asType<Quotes>();
		text_align_ = ctx.getComputedValue(Property::TEXT_ALIGN)->getEnum<TextAlign>();
		text_decoration_ = ctx.getComputedValue(Property::TEXT_DECORATION)->getEnum<TextDecoration>();
		text_indent_ = ctx.getComputedValue(Property::TEXT_INDENT)->asType<Width>();
		text_transform_ = ctx.getComputedValue(Property::TEXT_TRANSFORM)->getEnum<TextTransform>();
		unicode_bidi_ = ctx.getComputedValue(Property::UNICODE_BIDI)->getEnum<UnicodeBidi>();
		visibility_ = ctx.getComputedValue(Property::VISIBILITY)->getEnum<Visibility>();
		white_space_ = ctx.getComputedValue(Property::WHITE_SPACE)->getEnum<Whitespace>();
		vertical_align_ = ctx.getComputedValue(Property::VERTICAL_ALIGN)->asType<VerticalAlign>();
		word_spacing_ = ctx.getComputedValue(Property::WORD_SPACING)->asType<Length>();
		zindex_ = ctx.getComputedValue(Property::Z_INDEX)->asType<Zindex>();

		box_shadow_ = ctx.getComputedValue(Property::BOX_SHADOW)->asType<BoxShadowStyle>();
		//text_shadow_ = ctx.getComputedValue(Property::TEXT_SHADOW)->asType<TextShadowStyle>();
		// XXX should we turn these styles into the proper list of properties here?
		transition_properties_ = ctx.getComputedValue(Property::TRANSITION_PROPERTY)->asType<TransitionProperties>();
		transition_duration_ = ctx.getComputedValue(Property::TRANSITION_DURATION)->asType<TransitionTiming>();
		transition_timing_function_ = ctx.getComputedValue(Property::TRANSITION_TIMING_FUNCTION)->asType<TransitionTimingFunctions>();
		transition_delay_ = ctx.getComputedValue(Property::TRANSITION_DELAY)->asType<TransitionTiming>();
		border_radius_[0] = ctx.getComputedValue(Property::BORDER_TOP_LEFT_RADIUS)->asType<BorderRadius>();
		border_radius_[1] = ctx.getComputedValue(Property::BORDER_TOP_RIGHT_RADIUS)->asType<BorderRadius>();
		border_radius_[2] = ctx.getComputedValue(Property::BORDER_BOTTOM_RIGHT_RADIUS)->asType<BorderRadius>();
		border_radius_[3] = ctx.getComputedValue(Property::BORDER_BOTTOM_LEFT_RADIUS)->asType<BorderRadius>();
		opacity_ = ctx.getComputedValue(Property::OPACITY)->asType<Length>()->compute() / 65536.0f;
		//border_image_ = ctx.getComputedValue(Property::BORDER_IMAGE_SOURCE)->asType<ImageSource>();
		auto bis = ctx.getComputedValue(Property::BORDER_IMAGE_SLICE)->asType<BorderImageSlice>();
		border_image_fill_ = bis->isFilled();
		border_image_slice_ = bis->getWidths();
		border_image_width_ = ctx.getComputedValue(Property::BORDER_IMAGE_WIDTH)->asType<WidthList>()->getWidths();
		border_image_outset_ = ctx.getComputedValue(Property::BORDER_IMAGE_OUTSET)->asType<WidthList>()->getWidths();
		auto bir = ctx.getComputedValue(Property::BORDER_IMAGE_REPEAT)->asType<BorderImageRepeat>();
		border_image_repeat_horiz_ = bir->image_repeat_horiz_;
		border_image_repeat_vert_ = bir->image_repeat_vert_;
		background_clip_ = ctx.getComputedValue(Property::BACKGROUND_CLIP)->getEnum<BackgroundClip>();
	}

	StyleNodePtr StyleNode::createStyleTree(const DocumentPtr& doc)
	{
		StyleNodePtr root = std::make_shared<StyleNode>(doc);
		for(auto& child : doc->getChildren()) {
			root->parseNode(child);
		}
		return root;
	}
}
