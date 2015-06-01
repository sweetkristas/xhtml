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

#include "solid_renderable.hpp"

#include "xhtml_absolute_box.hpp"
#include "xhtml_block_box.hpp"
#include "xhtml_box.hpp"
#include "xhtml_inline_element_box.hpp"
#include "xhtml_layout_engine.hpp"

namespace xhtml
{
	using namespace css;

	namespace
	{
		std::string fp_to_str(const FixedPoint& fp)
		{
			std::ostringstream ss;
			ss << (static_cast<float>(fp)/LayoutEngine::getFixedPointScaleFloat());
			return ss.str();
		}
	}

	std::ostream& operator<<(std::ostream& os, const Rect& r)
	{
		os << "(" << fp_to_str(r.x) << ", " << fp_to_str(r.y) << ", " << fp_to_str(r.width) << ", " << fp_to_str(r.height) << ")";
		return os;
	}

	Box::Box(BoxId id, BoxPtr parent, NodePtr node)
		: id_(id),
		  node_(node),
		  parent_(parent),
		  dimensions_(),
		  boxes_(),
		  absolute_boxes_(),
		  cfloat_(CssFloat::NONE),
		  font_handle_(xhtml::RenderContext::get().getFontHandle()),
		  background_info_(),
		  border_info_(),
		  css_position_(CssPosition::STATIC),
		  padding_{},
		  border_{},
		  margin_{},
		  color_(),
		  css_left_(),
		  css_top_(),
		  css_width_(),
		  css_height_(),
		  init_done_(id_ == BoxId::LINE || id_ == BoxId::TEXT ? true : false),
		  float_clear_(CssClear::NONE),
		  vertical_align_(CssVerticalAlign::BASELINE),
		  text_align_(CssTextAlign::NORMAL)
	{
	}

	void Box::init()
	{
		RenderContext& ctx = RenderContext::get();
		color_ = ctx.getComputedValue(Property::COLOR).getValue<CssColor>().compute();

		background_info_.setColor(ctx.getComputedValue(Property::BACKGROUND_COLOR).getValue<CssColor>().compute());
		// We set repeat before the filename so we can correctly set the background texture wrap mode.
		background_info_.setRepeat(ctx.getComputedValue(Property::BACKGROUND_REPEAT).getValue<CssBackgroundRepeat>());
		background_info_.setPosition(ctx.getComputedValue(Property::BACKGROUND_POSITION).getValue<BackgroundPosition>());
		auto uri = ctx.getComputedValue(Property::BACKGROUND_IMAGE).getValue<UriStyle>();
		if(!uri.isNone()) {
			background_info_.setFile(uri.getUri());
		}
		css_position_ = ctx.getComputedValue(Property::POSITION).getValue<CssPosition>();

		const Property b[4]  = { Property::BORDER_TOP_WIDTH, Property::BORDER_LEFT_WIDTH, Property::BORDER_BOTTOM_WIDTH, Property::BORDER_RIGHT_WIDTH };
		const Property p[4]  = { Property::PADDING_TOP, Property::PADDING_LEFT, Property::PADDING_BOTTOM, Property::PADDING_RIGHT };
		const Property m[4]  = { Property::MARGIN_TOP, Property::MARGIN_LEFT, Property::MARGIN_BOTTOM, Property::MARGIN_RIGHT };

		for(int n = 0; n != 4; ++n) {
			border_[n] = ctx.getComputedValue(b[n]).getValue<Length>();
			padding_[n] = ctx.getComputedValue(p[n]).getValue<Length>();
			margin_[n] = ctx.getComputedValue(m[n]).getValue<Width>();
		}

		css_left_ = ctx.getComputedValue(Property::LEFT).getValue<Width>();
		css_top_ = ctx.getComputedValue(Property::TOP).getValue<Width>();
		css_width_ = ctx.getComputedValue(Property::WIDTH).getValue<Width>();
		css_height_ = ctx.getComputedValue(Property::HEIGHT).getValue<Width>();

		float_clear_ = ctx.getComputedValue(Property::CLEAR).getValue<css::Clear>().clr_;

		init_done_ = true;
	}

	RootBoxPtr Box::createLayout(NodePtr node, int containing_width, int containing_height)
	{
		LayoutEngine e;
		// search for the body element then render that content.
		node->preOrderTraversal([&e, containing_width, containing_height](NodePtr node){
			if(node->id() == NodeId::ELEMENT && node->hasTag(ElementId::BODY)) {
				e.formatNode(node, nullptr, point(containing_width * LayoutEngine::getFixedPointScale(), containing_height * LayoutEngine::getFixedPointScale()));
				return false;
			}
			return true;
		});
		node->layoutComplete();
		return e.getRoot();
	}

	bool Box::ancestralTraverse(std::function<bool(const ConstBoxPtr&)> fn) const
	{
		if(fn(shared_from_this())) {
			return true;
		}
		auto parent = getParent();
		if(parent != nullptr) {
			return parent->ancestralTraverse(fn);
		}
		return false;
	}

	void Box::preOrderTraversal(std::function<void(BoxPtr, int)> fn, int nesting)
	{
		fn(shared_from_this(), nesting);
		// floats, absolutes
		for(auto& child : boxes_) {
			child->preOrderTraversal(fn, nesting+1);
		}
		for(auto& abs : absolute_boxes_) {
			abs->preOrderTraversal(fn, nesting+1);
		}
	}

	BoxPtr Box::addAbsoluteElement(NodePtr node)
	{
		absolute_boxes_.emplace_back(std::make_shared<AbsoluteBox>(shared_from_this(), node));
		absolute_boxes_.back()->init();
		return absolute_boxes_.back();
	}

	void Box::layoutAbsolute(LayoutEngine& eng, const Dimensions& containing)
	{
		for(auto& abs : absolute_boxes_) {
			abs->layout(eng, containing);
		}
	}

	void Box::layout(LayoutEngine& eng, const Dimensions& containing)
	{
		ASSERT_LOG(init_done_, "init() wasn't called before layout -- this is super bad and a serious programming bug.");

		// If we have a clear flag set, then mvoe the cursor in the layout engine to clear appropriate floats.
		eng.moveCursorToClearFloats(float_clear_);

		handleLayout(eng, containing);
		layoutAbsolute(eng, containing);

		// need to call this after doing layout, since we need to now what the computed padding/border values are.
		border_info_.init(dimensions_);
	}

	BoxPtr Box::addInlineElement(NodePtr node)
	{
		ASSERT_LOG(id() == BoxId::LINE, "Tried to add inline element to non-line box.");
		boxes_.emplace_back(std::make_shared<InlineElementBox>(shared_from_this(), node));
		boxes_.back()->init();
		return boxes_.back();
	}

	point Box::getOffset() const
	{
		point offset;
		ancestralTraverse([&offset](const ConstBoxPtr& box) {
			offset += point(box->getDimensions().content_.x, box->getDimensions().content_.y);
			return true;
		});
		return offset;
	}

	void Box::calculateVertMPB(FixedPoint containing_height)
	{
		if(border_info_.isValid(Side::TOP)) {
			setBorderTop(getCssBorder(Side::TOP).compute());
		}
		if(border_info_.isValid(Side::BOTTOM)) {
			setBorderBottom(getCssBorder(Side::BOTTOM).compute());
		}

		setPaddingTop(getCssPadding(Side::TOP).compute(containing_height));
		setPaddingBottom(getCssPadding(Side::BOTTOM).compute(containing_height));

		setMarginTop(getCssMargin(Side::TOP).getLength().compute(containing_height));
		setMarginBottom(getCssMargin(Side::BOTTOM).getLength().compute(containing_height));
	}

	void Box::calculateHorzMPB(FixedPoint containing_width)
	{		
		if(border_info_.isValid(Side::LEFT)) {
			setBorderLeft(getCssBorder(Side::LEFT).compute());
		}
		if(border_info_.isValid(Side::RIGHT)) {
			setBorderRight(getCssBorder(Side::RIGHT).compute());
		}

		setPaddingLeft(getCssPadding(Side::LEFT).compute(containing_width));
		setPaddingRight(getCssPadding(Side::RIGHT).compute(containing_width));

		if(!getCssMargin(Side::LEFT).isAuto()) {
			setMarginLeft(getCssMargin(Side::LEFT).getLength().compute(containing_width));
		}
		if(!getCssMargin(Side::RIGHT).isAuto()) {
			setMarginRight(getCssMargin(Side::RIGHT).getLength().compute(containing_width));
		}
	}

	void Box::render(DisplayListPtr display_list, const point& offset) const
	{
		auto node = node_.lock();
		std::unique_ptr<RenderContext::Manager> ctx_manager;
		if(node != nullptr && node->id() == NodeId::ELEMENT) {
			// only instantiate on element nodes.
			ctx_manager.reset(new RenderContext::Manager(node->getProperties()));
		}

		point offs = offset;
		//if(id() != BoxId::TEXT) {
			offs += point(dimensions_.content_.x, dimensions_.content_.y);
		//}
		handleRenderBackground(display_list, offs);
		handleRenderBorder(display_list, offs);
		//if(id() == BoxId::TEXT) {
		//	offs += point(dimensions_.content_.x, dimensions_.content_.y);
		//}
		handleRender(display_list, offs);
		for(auto& child : getChildren()) {
			child->render(display_list, offs);
		}
		for(auto& ab : absolute_boxes_) {
			ab->render(display_list, point(0, 0));
		}

		// set the active rect on any parent node.
		if(node != nullptr) {
			auto& dims = getDimensions();
			const int x = (offs.x - dims.padding_.left - dims.border_.left) / LayoutEngine::getFixedPointScale();
			const int y = (offs.y - dims.padding_.top - dims.border_.top) / LayoutEngine::getFixedPointScale();
			const int w = (dims.content_.width + dims.padding_.left + dims.padding_.right + dims.border_.left + dims.border_.right) / LayoutEngine::getFixedPointScale();
			const int h = (dims.content_.height + dims.padding_.top + dims.padding_.bottom + dims.border_.top + dims.border_.bottom) / LayoutEngine::getFixedPointScale();
			node->setActiveRect(rect(x, y, w, h));
		}
	}

	void Box::handleRenderBackground(DisplayListPtr display_list, const point& offset) const
	{
		auto& dims = getDimensions();
		NodePtr node = getNode();
		if(node != nullptr && node->hasTag(ElementId::BODY)) {
			//dims = getRootDimensions();
		}
		background_info_.render(display_list, offset, dims);
	}

	void Box::handleRenderBorder(DisplayListPtr display_list, const point& offset) const
	{
		border_info_.render(display_list, offset, getDimensions());
		// we try and render border image first, if we can't then we default to the any given
		// style.
		if(!border_info_.render(display_list, offset, getDimensions())) {
		}
	}

}
