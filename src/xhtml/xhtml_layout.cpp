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

#include "asserts.hpp"
#include "css_styles.hpp"
#include "xhtml_layout.hpp"
#include "xhtml_node.hpp"
#include "xhtml_text_node.hpp"
#include "xhtml_render_ctx.hpp"

#include "AttributeSet.hpp"
#include "DisplayDevice.hpp"
#include "SceneObject.hpp"
#include "Shaders.hpp"


namespace xhtml
{
	// This is to ensure our fixed-point type will have enough precision.
	static_assert(sizeof(FixedPoint) < 32, "An int must be greater than or equal to 32 bits");

	using namespace css;

	namespace 
	{
		const int fixed_point_scale = 65536;
		const float fixed_point_scale_float = static_cast<float>(fixed_point_scale);

		std::string display_string(CssDisplay disp) {
			switch(disp) {
				case CssDisplay::BLOCK:					return "block";
				case CssDisplay::INLINE:				return "inline";
				case CssDisplay::INLINE_BLOCK:			return "inline-block";
				case CssDisplay::LIST_ITEM:				return "list-item";
				case CssDisplay::TABLE:					return "table";
				case CssDisplay::INLINE_TABLE:			return "inline-table";
				case CssDisplay::TABLE_ROW_GROUP:		return "table-row-group";
				case CssDisplay::TABLE_HEADER_GROUP:	return "table-header-group";
				case CssDisplay::TABLE_FOOTER_GROUP:	return "table-footer-group";
				case CssDisplay::TABLE_ROW:				return "table-row";
				case CssDisplay::TABLE_COLUMN_GROUP:	return "table-column-group";
				case CssDisplay::TABLE_COLUMN:			return "table-column";
				case CssDisplay::TABLE_CELL:			return "table-cell";
				case CssDisplay::TABLE_CAPTION:			return "table-caption";
				case CssDisplay::NONE:					return "none";
				default: 
					ASSERT_LOG(false, "illegal display value: " << static_cast<int>(disp));
					break;
			}
			return "none";
		}

		class SolidRenderable : public KRE::SceneObject
		{
		public:
			SolidRenderable() 
				: KRE::SceneObject("SolidRenderable")
			{
				init();
			}
			SolidRenderable(const rect& r, const KRE::Color& color)
				: KRE::SceneObject("SolidRenderable")
			{
				init();

				const float vx1 = static_cast<float>(r.x1())/fixed_point_scale_float;
				const float vy1 = static_cast<float>(r.y1())/fixed_point_scale_float;
				const float vx2 = static_cast<float>(r.x2())/fixed_point_scale_float;
				const float vy2 = static_cast<float>(r.y2())/fixed_point_scale_float;

				std::vector<KRE::vertex_color> vc;
				vc.emplace_back(glm::vec2(vx1, vy2), color.as_u8vec4());
				vc.emplace_back(glm::vec2(vx1, vy1), color.as_u8vec4());
				vc.emplace_back(glm::vec2(vx2, vy1), color.as_u8vec4());

				vc.emplace_back(glm::vec2(vx2, vy1), color.as_u8vec4());
				vc.emplace_back(glm::vec2(vx2, vy2), color.as_u8vec4());
				vc.emplace_back(glm::vec2(vx1, vy2), color.as_u8vec4());
				attribs_->update(&vc);
			}
			void init()
			{
				using namespace KRE;
				setShader(ShaderProgram::getProgram("attr_color_shader"));

				auto as = DisplayDevice::createAttributeSet();
				attribs_.reset(new KRE::Attribute<KRE::vertex_color>(AccessFreqHint::DYNAMIC, AccessTypeHint::DRAW));
				attribs_->addAttributeDesc(AttributeDesc(AttrType::POSITION, 2, AttrFormat::FLOAT, false, sizeof(vertex_color), offsetof(vertex_color, vertex)));
				attribs_->addAttributeDesc(AttributeDesc(AttrType::COLOR,  4, AttrFormat::UNSIGNED_BYTE, true, sizeof(vertex_color), offsetof(vertex_color, color)));
				as->addAttribute(AttributeBasePtr(attribs_));
				as->setDrawMode(DrawMode::TRIANGLES);
		
				addAttributeSet(as);
			}
			void update(std::vector<KRE::vertex_color>* coords)
			{
				attribs_->update(coords);
			}
		private:
			std::shared_ptr<KRE::Attribute<KRE::vertex_color>> attribs_;
		};

		std::string fp_to_str(const FixedPoint& fp)
		{
			std::ostringstream ss;
			ss << (static_cast<float>(fp)/fixed_point_scale_float);
			return ss.str();
		}
	}

	class LayoutEngine
	{
	public:
		explicit LayoutEngine() : root_(nullptr), cursor_(), dims_(), ctx_(RenderContext::get()) {}
		BoxPtr formatNode(NodePtr node, BoxPtr parent, const Dimensions& container) {
			auto& boxes = parent->getChildren();
			if(node->id() == NodeId::ELEMENT) {
				std::unique_ptr<RenderContext::Manager> ctx_manager;
				ctx_manager.reset(new RenderContext::Manager(node->getProperties()));

				const CssDisplay display = ctx_.getComputedValue(Property::DISPLAY).getValue<CssDisplay>();
				const CssFloat cfloat = ctx_.getComputedValue(Property::FLOAT).getValue<CssFloat>();
				const CssPosition position = ctx_.getComputedValue(Property::POSITION).getValue<CssPosition>();

				if(position == CssPosition::ABSOLUTE) {
					// absolute positioned elements are taken out of the normal document flow
					auto box = parent->addAbsoluteElement(node);
					box->layout(*this, container);
					return nullptr;
				} else if(position == CssPosition::FIXED) {
					// fixed positioned elements are taken out of the normal document flow
					auto box = root_->addFixedElement(node);
					box->layout(*this, container);
					return nullptr;
				} else {
					switch(display) {
						case CssDisplay::NONE:
							// Do not create a box for this or it's children
							return nullptr;
						case CssDisplay::INLINE: {
							BoxPtr open = nullptr;
							if(boxes.empty() || (!boxes.empty() && boxes.back()->id() == BoxId::BLOCK)) {
								open = parent->addChild(std::make_shared<LineBox>(parent, nullptr));
							} else {
								open = boxes.back();
							}
							open->addInlineElement(node);
							open->layout(*this, container);
							return open;
						}
						case CssDisplay::BLOCK: {
							auto box = parent->addChild(std::make_shared<BlockBox>(parent, node));
							box->layout(*this, container);
							return box;
						}
						case CssDisplay::INLINE_BLOCK:
						case CssDisplay::LIST_ITEM:
						case CssDisplay::TABLE:
						case CssDisplay::INLINE_TABLE:
						case CssDisplay::TABLE_ROW_GROUP:
						case CssDisplay::TABLE_HEADER_GROUP:
						case CssDisplay::TABLE_FOOTER_GROUP:
						case CssDisplay::TABLE_ROW:
						case CssDisplay::TABLE_COLUMN_GROUP:
						case CssDisplay::TABLE_COLUMN:
						case CssDisplay::TABLE_CELL:
						case CssDisplay::TABLE_CAPTION:
							ASSERT_LOG(false, "FIXME: LayoutBox::factory(): " << display_string(display));
							break;
						default:
							ASSERT_LOG(false, "illegal display value: " << static_cast<int>(display));
							break;
					}
				}
			} else if(node->id() == NodeId::TEXT) {
				// these nodes are inline/static by definition.
				BoxPtr open = nullptr;
				if(boxes.empty() || (!boxes.empty() && boxes.back()->id() != BoxId::LINE)) {
					// add a line box.
					open = parent->addChild(std::make_shared<LineBox>(parent, nullptr));
				} else {
					open = boxes.back();
				}
				auto box = open->addInlineElement(node);
				box->layout(*this, container);
				return open;
			} else {
				ASSERT_LOG(false, "Unhandled node id, only elements and text can be used in layout: " << static_cast<int>(node->id()));
			}
			return nullptr;
		}
		void formatNode(NodePtr node, BoxPtr parent, const point& container) {
			if(root_ == nullptr) {
				root_ = std::make_shared<BlockBox>(nullptr, node);
				dims_.content_ = Rect(0, 0, container.x, container.y);
				root_->layout(*this, dims_);
				return;
			}
		}
		BoxPtr getRoot() const { return root_; }
	private:
		BoxPtr root_;
		point cursor_;
		Dimensions dims_;
		RenderContext& ctx_;
	};


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
		  boxes_()
	{
	}

	BoxPtr Box::createLayout(NodePtr node, int containing_width)
	{
		LayoutEngine e;
		// search for the body element then render that content.
		node->preOrderTraversal([&e, containing_width](NodePtr node){
			if(node->id() == NodeId::ELEMENT && node->hasTag(ElementId::BODY)) {
				e.formatNode(node, nullptr, point(containing_width * fixed_point_scale, 0));
				return false;
			}
			return true;
		});
		return e.getRoot();
	}

	BoxPtr Box::addAbsoluteElement(NodePtr node)
	{
		absolute_boxes_.emplace_back(std::make_shared<AbsoluteBox>(shared_from_this(), node));
		return absolute_boxes_.back();
	}

	BoxPtr Box::addFixedElement(NodePtr node)
	{
		fixed_boxes_.emplace_back(std::make_shared<BlockBox>(shared_from_this(), node));
		return fixed_boxes_.back();
	}

	BoxPtr Box::addInlineElement(NodePtr node)
	{
		ASSERT_LOG(id() == BoxId::LINE, "Tried to add inline element to non-line box.");
		if(node->id() == NodeId::TEXT) {
			boxes_.emplace_back(std::make_shared<TextBox>(shared_from_this(), node));
		} else {
			boxes_.emplace_back(std::make_shared<InlineElementBox>(shared_from_this(), node));
		}
		return boxes_.back();
	}

	BlockBox::BlockBox(BoxPtr parent, NodePtr node)
		: Box(BoxId::BLOCK, parent, node)
	{
	}

	void BlockBox::layout(LayoutEngine& eng, const Dimensions& containing)
	{
		std::unique_ptr<RenderContext::Manager> ctx_manager;
		NodePtr node = getNode();
		if(node != nullptr && node->id() == NodeId::ELEMENT) {
			ctx_manager.reset(new RenderContext::Manager(node->getProperties()));
		}

		layoutWidth(containing);
		layoutPosition(containing);
		layoutChildren(eng);
		layoutHeight(containing);
	}

	void BlockBox::layoutWidth(const Dimensions& containing)
	{
		RenderContext& ctx = RenderContext::get();
		const FixedPoint containing_width = containing.content_.width;

		auto css_width = ctx.getComputedValue(Property::WIDTH).getValue<Width>();
		FixedPoint width = css_width.evaluate(ctx).getValue<Length>().compute(containing_width);

		setBorderLeft(ctx.getComputedValue(Property::BORDER_LEFT_WIDTH).getValue<Length>().compute());
		setBorderRight(ctx.getComputedValue(Property::BORDER_RIGHT_WIDTH).getValue<Length>().compute());

		setPaddingLeft(ctx.getComputedValue(Property::PADDING_LEFT).getValue<Length>().compute(containing_width));
		setPaddingRight(ctx.getComputedValue(Property::PADDING_RIGHT).getValue<Length>().compute(containing_width));

		auto css_margin_left = ctx.getComputedValue(Property::MARGIN_LEFT).getValue<Width>();
		auto css_margin_right = ctx.getComputedValue(Property::MARGIN_RIGHT).getValue<Width>();
		setMarginLeft(css_margin_left.evaluate(ctx).getValue<Length>().compute(containing_width));
		setMarginRight(css_margin_right.evaluate(ctx).getValue<Length>().compute(containing_width));

		FixedPoint total = getMBPWidth() + width;
			
		if(!css_width.isAuto() && total > containing.content_.width) {
			if(css_margin_left.isAuto()) {
				setMarginLeft(0);
			}
			if(css_margin_right.isAuto()) {
				setMarginRight(0);
			}
		}

		// If negative is overflow.
		FixedPoint underflow = containing.content_.width - total;

		if(css_width.isAuto()) {
			if(css_margin_left.isAuto()) {
				setMarginLeft(0);
			}
			if(css_margin_right.isAuto()) {
				setMarginRight(0);
			}
			if(underflow >= 0) {
				width = underflow;
			} else {
				width = 0;
				const auto rmargin = css_margin_right.evaluate(ctx).getValue<Length>().compute(containing_width);
				setMarginRight(rmargin + underflow);
			}
			setContentWidth(width);
		} else if(!css_margin_left.isAuto() && !css_margin_right.isAuto()) {
			setMarginRight(getDimensions().margin_.right + underflow);
		} else if(!css_margin_left.isAuto() && css_margin_right.isAuto()) {
			setMarginRight(underflow);
		} else if(css_margin_left.isAuto() && !css_margin_right.isAuto()) {
			setMarginLeft(underflow);
		} else if(css_margin_left.isAuto() && css_margin_right.isAuto()) {
			setMarginLeft(underflow / 2);
			setMarginRight(underflow / 2);
		} 
	}

	void BlockBox::layoutPosition(const Dimensions& containing)
	{
		RenderContext& ctx = RenderContext::get();
		const FixedPoint containing_height = containing.content_.height;
			
		setBorderTop(ctx.getComputedValue(Property::BORDER_TOP_WIDTH).getValue<Length>().compute());
		setBorderBottom(ctx.getComputedValue(Property::BORDER_BOTTOM_WIDTH).getValue<Length>().compute());

		setPaddingTop(ctx.getComputedValue(Property::PADDING_TOP).getValue<Length>().compute(containing_height));
		setPaddingBottom(ctx.getComputedValue(Property::PADDING_BOTTOM).getValue<Length>().compute(containing_height));

		setMarginTop(ctx.getComputedValue(Property::MARGIN_TOP).getValue<Width>().evaluate(ctx).getValue<Length>().compute(containing_height));
		setMarginBottom(ctx.getComputedValue(Property::MARGIN_BOTTOM).getValue<Width>().evaluate(ctx).getValue<Length>().compute(containing_height));

		setContentX(getMPBLeft());
		setContentY(containing.content_.height + getMPBTop());
	}

	void BlockBox::layoutChildren(LayoutEngine& eng)
	{
		NodePtr node = getNode();
		if(node != nullptr) {
			for(auto& child : node->getChildren()) {
				auto box = eng.formatNode(node, shared_from_this(), getDimensions());
				if(box) {
					setContentHeight(box->getDimensions().content_.height + box->getMBPHeight());
				}
			}
		}
	}

	void BlockBox::layoutHeight(const Dimensions& containing)
	{
		RenderContext& ctx = RenderContext::get();
		// a set height value overrides the calculated value.
		auto css_h = ctx.getComputedValue(Property::HEIGHT).getValue<Width>();
		if(!css_h.isAuto()) {
			setContentHeight(css_h.evaluate(ctx).getValue<Length>().compute(containing.content_.height));
		}
		// XXX deal with min-height and max-height
		//auto min_h = ctx.getComputedValue(Property::MIN_HEIGHT).getValue<Length>().compute(containing.content_.height);
		//if(getDimensions().content_.height() < min_h) {
		//	setContentHeight(min_h);
		//}
		//auto css_max_h = ctx.getComputedValue(Property::MAX_HEIGHT).getValue<Height>();
		//if(!css_max_h.isNone()) {
		//	FixedPoint max_h = css_max_h.getValue<Length>().compute(containing.content_.height);
		//	if(getDimensions().content_.height() > max_h) {
		//		setContentHeight(max_h);
		//	}
		//}
	}

	LineBox::LineBox(BoxPtr parent, NodePtr node)
		: Box(BoxId::LINE, parent, node)
	{
	}

	void LineBox::layout(LayoutEngine& eng, const Dimensions& containing)
	{
	}

	TextBox::TextBox(BoxPtr parent, NodePtr node)
		: Box(BoxId::TEXT, parent, node)
	{
	}

	void TextBox::layout(LayoutEngine& eng, const Dimensions& containing)
	{

	}

	AbsoluteBox::AbsoluteBox(BoxPtr parent, NodePtr node)
		: Box(BoxId::ABSOLUTE, parent, node)
	{
	}

	void AbsoluteBox::layout(LayoutEngine& eng, const Dimensions& containing)
	{
	}

	InlineElementBox::InlineElementBox(BoxPtr parent, NodePtr node)
		: Box(BoxId::INLINE_ELEMENT, parent, node)
	{
	}

	void InlineElementBox::layout(LayoutEngine& eng, const Dimensions& containing)
	{
	}
}
