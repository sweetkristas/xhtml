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

#include <stack>

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
	
	// XXX todo: floats should be owned by the root element and positioned relative to that, more or less.
	// this requires that the getWidthAtCursor/getXAtcursor functions be moved into LayoutEngine, which is
	// reasonable. They will still need to know about the parent->getDimensions().content_.width for the
	// calculation.
	// We're not handling text alignment or justification yet.
	class LayoutEngine
	{
	public:
		explicit LayoutEngine() : root_(nullptr), dims_(), ctx_(RenderContext::get()), open_() {}

		void formatNode(NodePtr node, BoxPtr parent, const point& container) {
			if(root_ == nullptr) {
				root_ = std::make_shared<BlockBox>(nullptr, node);
				dims_.content_ = Rect(0, 0, container.x, container.y);
				root_->layout(*this, dims_);
				return;
			}
		}

		void formatNode(NodePtr node, BoxPtr parent, const Dimensions& container) {
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
					return;
				} else if(position == CssPosition::FIXED) {
					// fixed positioned elements are taken out of the normal document flow
					auto box = root_->addFixedElement(node);
					box->layout(*this, container);
					return;
				} else {
					if(cfloat != CssFloat::NONE) {
						bool saved_open = false;
						FixedPoint y = 0;
						if(isOpenBox()) {
							//parent->addWaitingFloat(*this, cfloat, node);
							//return nullptr;
							y = open_.top().cursor_.y;
							saved_open = true;
							pushOpenBox();
						}
						// no open box so just add it.
						parent->addFloatBox(*this, std::make_shared<BlockBox>(parent, node), cfloat, y);
						if(saved_open) {
							popOpenBox();
							open_.top().open_box_->setContentX(open_.top().open_box_->getDimensions().content_.x
								+ parent->getXAtCursor(open_.top().cursor_));
							//open_.top().cursor_.x = parent->getXAtCursor(open_.top().cursor_);
						}
						return;
					}
					switch(display) {
						case CssDisplay::NONE:
							// Do not create a box for this or it's children
							return;
						case CssDisplay::INLINE: {
							layoutInlineElement(node, parent);
							return;
						}
						case CssDisplay::BLOCK: {
							auto box = parent->addChild(std::make_shared<BlockBox>(parent, node));
							box->layout(*this, container);
							return;
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
							ASSERT_LOG(false, "FIXME: LayoutEngine::formatNode(): " << display_string(display));
							break;
						default:
							ASSERT_LOG(false, "illegal display value: " << static_cast<int>(display));
							break;
					}
				}
			} else if(node->id() == NodeId::TEXT) {
				// these nodes are inline/static by definition.
				// XXX if parent has any left/right pending floated elements and we're starting a new box apply them here.
				layoutInlineText(node, parent);
				return;
			} else {
				ASSERT_LOG(false, "Unhandled node id, only elements and text can be used in layout: " << static_cast<int>(node->id()));
			}
		}

		void layoutInlineElement(NodePtr node, BoxPtr parent) {
			if(node->isReplaced()) {
				BoxPtr open = getOpenBox(parent);
				// This should be adding to the currently open box.
				auto inline_element_box = open->addInlineElement(node);
				pushOpenBox();
				inline_element_box->layout(*this, open->getDimensions());
				popOpenBox();
			} else {
				for(auto& child : node->getChildren()) {
					formatNode(child, parent, parent->getDimensions());
				}
			
			}
		}

		void layoutInlineText(NodePtr node, BoxPtr parent) {
			TextPtr tnode = std::dynamic_pointer_cast<Text>(node);
			ASSERT_LOG(tnode != nullptr, "Logic error, couldn't up-cast node to Text.");

			BoxPtr open = getOpenBox(parent);
			FixedPoint width = parent->getWidthAtCursor(open_.top().cursor_) - open_.top().cursor_.x;

			tnode->transformText(width >= 0);
			auto it = tnode->begin();
			while(it != tnode->end()) {
				LinePtr line = tnode->reflowText(it, width);
				if(!line->line.empty()) {
					BoxPtr txt = open->addChild(std::make_shared<TextBox>(parent, line));
					txt->setContentX(open_.top().cursor_.x);
					txt->setContentY(0);
					txt->setContentHeight(getLineHeight());
					FixedPoint x_inc = txt->getDimensions().content_.width + txt->getMBPWidth();
					open_.top().cursor_.x += x_inc;
					width -= x_inc;
				}
				if(line->is_end_line) {
					closeOpenBox(parent);
					open = getOpenBox(parent);
					width = parent->getWidthAtCursor(open_.top().cursor_);
				}
			}
		}
		
		void pushOpenBox() {
			open_.push(OpenBox());
		}
		void popOpenBox() {
			open_.pop();
		}

		BoxPtr getOpenBox(BoxPtr parent) {
			if(open_.empty()) {
				pushOpenBox();
			}
			if(open_.top().open_box_ == nullptr) {
				open_.top().open_box_ = parent->addChild(std::make_shared<LineBox>(parent, nullptr));
				open_.top().open_box_->setContentX(parent->getXAtCursor(open_.top().cursor_));
				open_.top().open_box_->setContentY(open_.top().cursor_.y);
				open_.top().open_box_->setContentWidth(parent->getWidthAtCursor(open_.top().cursor_));
			}
			return open_.top().open_box_;
		}

		void closeOpenBox(BoxPtr parent) {
			if(open_.empty()) {
				return;
			}
			if(open_.top().open_box_ == nullptr) {
				return;
			}
			open_.top().open_box_->layout(*this, parent->getDimensions());

			point old_cursor = point(0,
				open_.top().open_box_->getDimensions().content_.height + open_.top().cursor_.y);
			open_.top().open_box_ = nullptr;

			parent->positionWaitingFloats(*this);
			open_.top().cursor_ = old_cursor;
		}

		FixedPoint getLineHeight() const {
			const auto lh = ctx_.getComputedValue(Property::LINE_HEIGHT).getValue<Length>();
			FixedPoint line_height = lh.compute();
			if(lh.isPercent() || lh.isNumber()) {
				line_height = static_cast<FixedPoint>(line_height / fixed_point_scale_float
					* ctx_.getComputedValue(Property::FONT_SIZE).getValue<FixedPoint>());
			}
			return line_height;
		}

		bool isOpenBox() const { return open_.top().open_box_ != nullptr; }

		BoxPtr getRoot() const { return root_; }
		
		const point& getCursor() const { return open_.top().cursor_; }
		void setCursor(const point& cursor) { open_.top().cursor_ = cursor; }
		void incrCursor(FixedPoint x) { open_.top().cursor_.x += x; }
	private:
		BoxPtr root_;
		Dimensions dims_;
		RenderContext& ctx_;
		struct OpenBox {
			OpenBox() : open_box_(), cursor_() {}
			OpenBox(BoxPtr bp, point cursor) : open_box_(bp), cursor_(cursor) {}
			BoxPtr open_box_;
			point cursor_;
		};
		std::stack<OpenBox> open_;
		
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
		  boxes_(),
		  absolute_boxes_(),
		  fixed_boxes_(),
		  float_boxes_to_be_placed_(),
		  left_floats_(),
		  right_floats_(),
		  cfloat_(CssFloat::NONE),
		  font_handle_(xhtml::RenderContext::get().getFontHandle())
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

	void Box::addWaitingFloat(LayoutEngine& eng, CssFloat cfloat, NodePtr node)
	{
		auto box = std::make_shared<BlockBox>(shared_from_this(), node);
		box->cfloat_ = cfloat;
		//box->layout(eng, getDimensions());
		float_boxes_to_be_placed_.emplace_back(box);
	}
	
	void Box::addFloatBox(LayoutEngine& eng, BoxPtr box, CssFloat cfloat, FixedPoint y)
	{
		if(cfloat == CssFloat::LEFT) {
			// XXX Calculate position of box here
			// XXX need to come up with a decent algorithm to move float boxes down
			// if they can't be positioned at the current cursor.
			FixedPoint x = getXAtCursor(eng.getCursor());
			FixedPoint w = getWidthAtCursor(eng.getCursor());
			box->layout(eng, getDimensions());
			box->setContentX(x);
			box->setContentY(y - dimensions_.content_.y);
			left_floats_.emplace_back(box);
		} else {
			// XXX Calculate position of box here
			right_floats_.emplace_back(box);
		}
	}

	void Box::positionWaitingFloats(LayoutEngine& eng)
	{
		for(auto& fb : float_boxes_to_be_placed_) {
			if(fb->cfloat_ == CssFloat::LEFT) {
				// XXX need to come up with a decent algorithm to move float boxes down
				// if they can't be positioned at the current cursor.
				FixedPoint x = getXAtCursor(eng.getCursor());
				FixedPoint w = getWidthAtCursor(eng.getCursor());
				fb->layout(eng, getDimensions());
				fb->setContentX(x);
				fb->setContentY(eng.getCursor().y  - dimensions_.content_.y);
				left_floats_.emplace_back(fb);
			} else {
				/// XXX todo.
			}
		}
		float_boxes_to_be_placed_.clear();
	}

	FixedPoint Box::getWidthAtCursor(const point& cursor) const
	{
		FixedPoint width = dimensions_.content_.width;
		// since we expect only a small number of floats per element
		// a linear search through them seems fine at this point.
		for(auto& lf : left_floats_) {
			auto& dims = lf->getDimensions();
			if(cursor.y > (lf->getMPBTop() + dims.content_.y) && cursor.y <= (lf->getMPBTop() + lf->getMBPHeight() + dims.content_.height + dims.content_.y)) {
				width -= lf->getMBPWidth() + dims.content_.width;
			}
		}
		for(auto& rf : right_floats_) {
			auto& dims = rf->getDimensions();
			if(cursor.y > (rf->getMPBTop() + dims.content_.y) && cursor.y <= (rf->getMPBTop() + rf->getMBPHeight() + dims.content_.height + dims.content_.y)) {
				width -= rf->getMBPWidth() + dims.content_.width;
			}
		}
		return width < 0 ? 0 : width;
	}

	FixedPoint Box::getXAtCursor(const point& cursor) const
	{
		FixedPoint x = 0;
		// since we expect only a small number of floats per element
		// a linear search through them seems fine at this point.
		for(auto& lf : left_floats_) {
			auto& dims = lf->getDimensions();
			if(cursor.y > (lf->getMPBTop() + dims.content_.y) && cursor.y <= (lf->getMPBTop() + lf->getMBPHeight() + dims.content_.height + dims.content_.y)) {
				x = std::max(x, lf->getMBPWidth() + dims.content_.width);
			}
		}
		return x;
	}

	void Box::preOrderTraversal(std::function<void(BoxPtr, int)> fn, int nesting)
	{
		fn(shared_from_this(), nesting);
		// floats, absolutes
		for(auto& child : boxes_) {
			child->preOrderTraversal(fn, nesting+1);
		}
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
		boxes_.emplace_back(std::make_shared<InlineElementBox>(shared_from_this(), node));
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
		bool is_replaced = false;
		if(node != nullptr && node->id() == NodeId::ELEMENT) {
			ctx_manager.reset(new RenderContext::Manager(node->getProperties()));
			is_replaced = node->isReplaced();
		}

		if(is_replaced) {
			setMBP(containing.content_.width);
			setContentRect(node->getDimensions());
			layoutChildren(eng);
		} else {
			layoutWidth(containing);
			layoutPosition(containing);
			layoutChildren(eng);
			layoutHeight(containing);
		}
	}

	void BlockBox::setMBP(FixedPoint containing_width)
	{
		RenderContext& ctx = RenderContext::get();
		setBorderLeft(ctx.getComputedValue(Property::BORDER_LEFT_WIDTH).getValue<Length>().compute());
		setBorderRight(ctx.getComputedValue(Property::BORDER_RIGHT_WIDTH).getValue<Length>().compute());

		setPaddingLeft(ctx.getComputedValue(Property::PADDING_LEFT).getValue<Length>().compute(containing_width));
		setPaddingRight(ctx.getComputedValue(Property::PADDING_RIGHT).getValue<Length>().compute(containing_width));

		auto css_margin_left = ctx.getComputedValue(Property::MARGIN_LEFT).getValue<Width>();
		auto css_margin_right = ctx.getComputedValue(Property::MARGIN_RIGHT).getValue<Width>();
		setMarginLeft(css_margin_left.evaluate(ctx).getValue<Length>().compute(containing_width));
		setMarginRight(css_margin_right.evaluate(ctx).getValue<Length>().compute(containing_width));

	}

	void BlockBox::layoutWidth(const Dimensions& containing)
	{
		RenderContext& ctx = RenderContext::get();
		const FixedPoint containing_width = containing.content_.width;

		auto css_width = ctx.getComputedValue(Property::WIDTH).getValue<Width>();
		FixedPoint width = css_width.evaluate(ctx).getValue<Length>().compute(containing_width);

		setMBP(containing_width);
		auto css_margin_left = ctx.getComputedValue(Property::MARGIN_LEFT).getValue<Width>();
		auto css_margin_right = ctx.getComputedValue(Property::MARGIN_RIGHT).getValue<Width>();

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
				eng.formatNode(child, shared_from_this(), getDimensions());
			}
		}
		// close any open boxes.
		eng.closeOpenBox(shared_from_this());

		// XXX We should assign margin/padding/border as appropriate here.
		for(auto& child : getChildren()) {
			setContentHeight(child->getDimensions().content_.height + child->getMBPHeight());
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
		FixedPoint max_height = 0;
		FixedPoint width = 0;
		for(auto& child : getChildren()) {
			width += child->getMBPWidth() + child->getDimensions().content_.width;
			max_height = std::max(max_height, child->getDimensions().content_.height);
		}
		setContentWidth(width);
		setContentHeight(max_height);
		setContentY(getDimensions().content_.y + max_height);
	}

	TextBox::TextBox(BoxPtr parent, LinePtr line)
		: Box(BoxId::TEXT, parent, nullptr),
		  line_(line),
		  space_advance_(line->space_advance)
	{
		FixedPoint width = 0;
		for(auto& word : line->line) {
			width += word.advance.back().x;
		}
		width += space_advance_ * (line_->line.size()-1);
		setContentWidth(width);
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
		setContentY(eng.getCursor().y);
		setContentX(0);
		setContentWidth(containing.content_.width);
		NodePtr node = getNode();
		if(node != nullptr) {
			for(auto& child : node->getChildren()) {
				eng.formatNode(child, shared_from_this(), getDimensions());
			}
		}
		// XXX We should assign margin/padding/border as appropriate here.
		FixedPoint max_w = 0;
		for(auto& child : getChildren()) {
			setContentHeight(child->getDimensions().content_.height + child->getMBPHeight());
			max_w = std::max(max_w, child->getMBPWidth() + child->getDimensions().content_.width);
		}
		setContentWidth(max_w);
	}

	std::string BlockBox::toString() const
	{
		std::ostringstream ss;
		ss << "BlockBox: " << getDimensions().content_;
		return ss.str();
	}

	std::string LineBox::toString() const
	{
		std::ostringstream ss;
		ss << "LineBox: " << getDimensions().content_;
		return ss.str();
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

	std::string AbsoluteBox::toString() const
	{
		std::ostringstream ss;
		ss << "AbsoluteBox: " << getDimensions().content_;
		return ss.str();
	}

	std::string InlineElementBox::toString() const
	{
		std::ostringstream ss;
		ss << "InlineElementBox: " << getDimensions().content_;
		return ss.str();
	}

	void Box::render(DisplayListPtr display_list, const point& offset) const
	{
		auto node = node_.lock();
		std::unique_ptr<RenderContext::Manager> ctx_manager;
		if(node != nullptr && node->id() == NodeId::ELEMENT) {
			// only instantiate on element nodes.
			ctx_manager.reset(new RenderContext::Manager(node->getProperties()));
		}

		point offs = offset + point(dimensions_.content_.x, dimensions_.content_.y);
		handleRenderBackground(display_list, offs);
		handleRenderBorder(display_list, offs);
		handleRender(display_list, offs);
		for(auto& child : getChildren()) {
			child->render(display_list, offs);
		}
		for(auto& lf : left_floats_) {
			lf->render(display_list, offs);
		}
		for(auto& rf : left_floats_) {
			rf->render(display_list, offs);
		}
		// render absolute boxes.
		// render fixed boxes.
	}

	void Box::handleRenderBackground(DisplayListPtr display_list, const point& offset) const
	{
		/*if(background_color_.ai() != 0) {
			rect r(offset.x + dimensions_.content_.x - dimensions_.padding_.left,
				offset.y + dimensions_.content_.y - dimensions_.padding_.top,
				dimensions_.content_.width + dimensions_.padding_.left + dimensions_.padding_.right,
				dimensions_.content_.height + dimensions_.padding_.top + dimensions_.padding_.bottom);
			display_list->addRenderable(std::make_shared<SolidRenderable>(r, background_color_));
		}*/
	}

	void Box::handleRenderBorder(DisplayListPtr display_list, const point& offset) const
	{
		// XXX
	}

	void BlockBox::handleRender(DisplayListPtr display_list, const point& offset) const
	{
		NodePtr node = getNode();
		if(node != nullptr && node->isReplaced()) {
			auto r = node->getRenderable();
			r->setPosition(glm::vec3(static_cast<float>(offset.x + getDimensions().content_.x)/65536.0f,
				static_cast<float>(offset.y + getDimensions().content_.y)/65536.0f,
				0.0f));
			display_list->addRenderable(r);
		}
	}

	void LineBox::handleRender(DisplayListPtr display_list, const point& offset) const
	{
		// do nothing
	}

	void TextBox::handleRender(DisplayListPtr display_list, const point& offset) const
	{
		std::vector<point> path;
		std::string text;
		int dim_x = offset.x;
		int dim_y = offset.y;
		for(auto& word : line_->line) {
			for(auto it = word.advance.begin(); it != word.advance.end()-1; ++it) {
				path.emplace_back(it->x + dim_x, it->y + dim_y);
			}
			dim_x += word.advance.back().x + space_advance_;
			text += word.word;
		}

		auto& ctx = RenderContext::get();
		//auto fh = ctx.getFontHandle();
		//auto fontr = fh->createRenderableFromPath(nullptr, text, path);
		auto fontr = getFont()->createRenderableFromPath(nullptr, text, path);
		fontr->setColor(ctx.getComputedValue(Property::COLOR).getValue<CssColor>().compute());
		display_list->addRenderable(fontr);
	}

	void AbsoluteBox::handleRender(DisplayListPtr display_list, const point& offset) const
	{
		// XXX
	}

	void InlineElementBox::handleRender(DisplayListPtr display_list, const point& offset) const
	{
		auto node = getNode();
		if(node != nullptr) {
			auto r = node->getRenderable();
			if(r != nullptr) {
				r->setPosition(glm::vec3(offset.x/fixed_point_scale_float, offset.y/fixed_point_scale_float, 0.0f));
				display_list->addRenderable(r);
			}
		}
	}
}
