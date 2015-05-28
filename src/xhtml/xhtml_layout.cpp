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
#include "profile_timer.hpp"
#include "to_roman.hpp"
#include "xhtml_layout.hpp"
#include "xhtml_node.hpp"
#include "xhtml_text_node.hpp"
#include "xhtml_render_ctx.hpp"
#include "utf8_to_codepoint.hpp"

#include "AttributeSet.hpp"
#include "Blittable.hpp"
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

		class TextureRenderable : public KRE::SceneObject
		{
		public:
			explicit TextureRenderable(KRE::TexturePtr tex) 
				: KRE::SceneObject("TextureRenderable"), 
				  texture_(), 
				  attribs_() 
			{
				using namespace KRE;
				auto as = DisplayDevice::createAttributeSet(true);
				attribs_.reset(new KRE::Attribute<vertex_texcoord>(AccessFreqHint::DYNAMIC, AccessTypeHint::DRAW));
				attribs_->addAttributeDesc(AttributeDesc(AttrType::POSITION, 2, AttrFormat::FLOAT, false, sizeof(vertex_texcoord), offsetof(vertex_texcoord, vtx)));
				attribs_->addAttributeDesc(AttributeDesc(AttrType::TEXTURE,  2, AttrFormat::FLOAT, false, sizeof(vertex_texcoord), offsetof(vertex_texcoord, tc)));
				as->addAttribute(AttributeBasePtr(attribs_));
				as->setDrawMode(DrawMode::TRIANGLES);
		
				addAttributeSet(as);
			}
			void update(std::vector<KRE::vertex_texcoord>* coords)
			{
				attribs_->update(coords);
			}
		private:
			KRE::TexturePtr texture_;
			std::shared_ptr<KRE::Attribute<KRE::vertex_texcoord>> attribs_;
		};

		std::string fp_to_str(const FixedPoint& fp)
		{
			std::ostringstream ss;
			ss << (static_cast<float>(fp)/fixed_point_scale_float);
			return ss.str();
		}

		struct BorderDimensions
		{
			float x11;	// left side
			float x12;	// left side + width of left

			float y11;	// top side
			float y12;	// top side + width of top

			float x21;	// right side (further left co-ordinate).
			float x22;	// right side + width of right

			float y21;	// bottom side (furthest up co-ordinate).
			float y22;	// bottom side + width of bottom.
		};

		void generate_solid_left_side(std::vector<KRE::vertex_color>* vc, const BorderDimensions& dims, const glm::u8vec4& color)
		{
			vc->emplace_back(glm::vec2(dims.x11, dims.y11), color);
			vc->emplace_back(glm::vec2(dims.x11, dims.y12), color);
			vc->emplace_back(glm::vec2(dims.x12, dims.y12), color);

			vc->emplace_back(glm::vec2(dims.x12, dims.y12), color);
			vc->emplace_back(glm::vec2(dims.x11, dims.y12), color);
			vc->emplace_back(glm::vec2(dims.x11, dims.y21), color);

			vc->emplace_back(glm::vec2(dims.x11, dims.y21), color);
			vc->emplace_back(glm::vec2(dims.x12, dims.y12), color);
			vc->emplace_back(glm::vec2(dims.x12, dims.y21), color);

			vc->emplace_back(glm::vec2(dims.x12, dims.y21), color);
			vc->emplace_back(glm::vec2(dims.x11, dims.y21), color);
			vc->emplace_back(glm::vec2(dims.x11, dims.y22), color);
		}

		void generate_solid_left_side(std::vector<KRE::vertex_color>* vc, float x, float w, float y, float yw, float y2, float y2w, const glm::u8vec4& color)
		{
			vc->emplace_back(glm::vec2(x, y), color);
			vc->emplace_back(glm::vec2(x, y+yw), color);
			vc->emplace_back(glm::vec2(x+w, y+yw), color);

			vc->emplace_back(glm::vec2(x+w, y+yw), color);
			vc->emplace_back(glm::vec2(x, y+yw), color);
			vc->emplace_back(glm::vec2(x, y2), color);

			vc->emplace_back(glm::vec2(x, y2), color);
			vc->emplace_back(glm::vec2(x+w, y+yw), color);
			vc->emplace_back(glm::vec2(x+w, y2), color);

			vc->emplace_back(glm::vec2(x+w, y2), color);
			vc->emplace_back(glm::vec2(x, y2), color);
			vc->emplace_back(glm::vec2(x, y2+y2w), color);
		}

		void generate_solid_right_side(std::vector<KRE::vertex_color>* vc, float x, float w, float y, float yw, float y2, float y2w, const glm::u8vec4& color)
		{
			vc->emplace_back(glm::vec2(x+w, y), color);
			vc->emplace_back(glm::vec2(x, y+yw), color);
			vc->emplace_back(glm::vec2(x+w, y+yw), color);

			vc->emplace_back(glm::vec2(x+w, y+yw), color);
			vc->emplace_back(glm::vec2(x, y+yw), color);
			vc->emplace_back(glm::vec2(x, y2), color);

			vc->emplace_back(glm::vec2(x, y2), color);
			vc->emplace_back(glm::vec2(x+w, y+yw), color);
			vc->emplace_back(glm::vec2(x+w, y2), color);

			vc->emplace_back(glm::vec2(x+w, y2), color);
			vc->emplace_back(glm::vec2(x, y2), color);
			vc->emplace_back(glm::vec2(x+w, y2+y2w), color);
		}
		
		void generate_solid_top_side(std::vector<KRE::vertex_color>* vc, float x, float xw, float x2, float x2w, float y, float yw, const glm::u8vec4& color)
		{
			vc->emplace_back(glm::vec2(x, y), color);
			vc->emplace_back(glm::vec2(x+xw, y+yw), color);
			vc->emplace_back(glm::vec2(x+xw, y), color);

			vc->emplace_back(glm::vec2(x+xw, y+yw), color);
			vc->emplace_back(glm::vec2(x+xw, y), color);
			vc->emplace_back(glm::vec2(x2, y+yw), color);

			vc->emplace_back(glm::vec2(x+xw, y), color);
			vc->emplace_back(glm::vec2(x2, y+yw), color);
			vc->emplace_back(glm::vec2(x2, y), color);

			vc->emplace_back(glm::vec2(x2, y+yw), color);
			vc->emplace_back(glm::vec2(x2, y), color);
			vc->emplace_back(glm::vec2(x2+x2w, y), color);
		}

		void generate_solid_bottom_side(std::vector<KRE::vertex_color>* vc, float x, float xw, float x2, float x2w, float y, float yw, const glm::u8vec4& color)
		{
			vc->emplace_back(glm::vec2(x, y+yw), color);
			vc->emplace_back(glm::vec2(x+xw, y), color);
			vc->emplace_back(glm::vec2(x+xw, y+yw), color);

			vc->emplace_back(glm::vec2(x+xw, y+yw), color);
			vc->emplace_back(glm::vec2(x+xw, y), color);
			vc->emplace_back(glm::vec2(x2, y+yw), color);

			vc->emplace_back(glm::vec2(x+xw, y), color);
			vc->emplace_back(glm::vec2(x2, y+yw), color);
			vc->emplace_back(glm::vec2(x2, y), color);

			vc->emplace_back(glm::vec2(x2, y+yw), color);
			vc->emplace_back(glm::vec2(x2, y), color);
			vc->emplace_back(glm::vec2(x2+x2w, y+yw), color);
		}
	}
	
	// XXX We're not handling text alignment or justification yet.
	class LayoutEngine
	{
	public:
		explicit LayoutEngine() 
			: root_(nullptr), 
			  dims_(), 
			  ctx_(RenderContext::get()), 
			  open_(), 
			  offset_(),
			  list_item_counter_()
		{
			list_item_counter_.emplace(1);
		}

		void formatNode(NodePtr node, BoxPtr parent, const point& container) {
			if(root_ == nullptr) {
				RenderContext::Manager ctx_manager(node->getProperties());

				offset_.emplace(point());
				root_ = std::make_shared<BlockBox>(nullptr, node);
				root_->init();
				dims_.content_ = Rect(0, 0, container.x, 0);
				root_->layout(*this, dims_);
				return;
			}
		}

		template<typename T>
		struct StackManager
		{
			StackManager(std::stack<T>& counter, const T& defa=T()) : counter_(counter) { counter_.emplace(defa); }
			~StackManager() { counter_.pop(); }
			std::stack<T>& counter_;
		};

		BoxPtr formatNode(NodePtr node, BoxPtr parent, const Dimensions& container, std::function<void(BoxPtr)> pre_layout_fn=nullptr) {
			if(node->id() == NodeId::ELEMENT) {
				RenderContext::Manager ctx_manager(node->getProperties());

				std::unique_ptr<StackManager<int>> li_manager;
				if(node->hasTag(ElementId::UL) || node->hasTag(ElementId::OL)) {
					li_manager.reset(new StackManager<int>(list_item_counter_, 1));
				}
				if(node->hasTag(ElementId::LI) ) {
					auto &top = list_item_counter_.top();
					++top;
				}

				const CssDisplay display = ctx_.getComputedValue(Property::DISPLAY).getValue<CssDisplay>();
				const CssFloat cfloat = ctx_.getComputedValue(Property::FLOAT).getValue<CssFloat>();
				const CssPosition position = ctx_.getComputedValue(Property::POSITION).getValue<CssPosition>();

				if(display == CssDisplay::NONE) {
					// Do not create a box for this or it's children
					// early return
					return nullptr;
				}

				if(position == CssPosition::ABSOLUTE) {
					// absolute positioned elements are taken out of the normal document flow
					parent->addAbsoluteElement(node);
					return nullptr;
				} else if(position == CssPosition::FIXED) {
					// fixed positioned elements are taken out of the normal document flow
					root_->addFixedElement(node);
					return nullptr;
				} else {
					if(cfloat != CssFloat::NONE) {
						bool saved_open = false;
						FixedPoint y = 0;
						if(isOpenBox()) {
							y = open_.top().cursor_.y;
							saved_open = true;
							pushOpenBox();
						}
						// XXX need to add an offset to position for the float box based on body margin.
						root_->addFloatBox(*this, std::make_shared<BlockBox>(root_, node), cfloat, y, offset_.top());
						if(saved_open) {
							popOpenBox();
							open_.top().open_box_->setContentX(open_.top().open_box_->getDimensions().content_.x + getXAtCursor());
						}
						return nullptr;
					}
					switch(display) {
						case CssDisplay::NONE:
							// Do not create a box for this or it's children
							return nullptr;
						case CssDisplay::INLINE: {
							layoutInlineElement(node, parent, pre_layout_fn);
							return nullptr;
						}
						case CssDisplay::BLOCK: {
							closeOpenBox(parent);
							auto box = parent->addChild(std::make_shared<BlockBox>(parent, node));
							if(pre_layout_fn) {
								pre_layout_fn(box);
							}
							box->layout(*this, container);
							if(!open_.empty()) {
								open_.top().cursor_.x = 0;
								open_.top().cursor_.y = 0;
							}
							//open_.top().cursor_.y = box->getDimensions().content_.y + box->getDimensions().content_.height + box->getMBPBottom();
							return box;
						}
						case CssDisplay::INLINE_BLOCK: {							
							auto box = parent->addChild(std::make_shared<BlockBox>(parent, node));
							if(pre_layout_fn) {
								pre_layout_fn(box);
							}
							box->layout(*this, container);
							return box;
						}
						case CssDisplay::LIST_ITEM: {
							auto box = parent->addChild(std::make_shared<ListItemBox>(parent, node, list_item_counter_.top()));
							if(pre_layout_fn) {
								pre_layout_fn(box);
							}
							box->layout(*this, container);
							return nullptr;
						}
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
				layoutInlineText(node, parent, pre_layout_fn);
				return nullptr;
			} else {
				ASSERT_LOG(false, "Unhandled node id, only elements and text can be used in layout: " << static_cast<int>(node->id()));
			}
			return nullptr;
		}

		void layoutInlineElement(NodePtr node, BoxPtr parent, std::function<void(BoxPtr)> pre_layout_fn) {
			if(node->isReplaced()) {
				BoxPtr open = getOpenBox(parent);
				// This should be adding to the currently open box.
				auto inline_element_box = open->addInlineElement(node);
				pushOpenBox();
				if(pre_layout_fn) {
					pre_layout_fn(inline_element_box);
				}
				inline_element_box->layout(*this, open->getDimensions());
				popOpenBox();
			} else {
				// XXX we should apply border styles to inline element here (and backgrounds)
				std::vector<FixedPoint> padding = generatePadding();
				std::vector<FixedPoint> border_width = generateBorderWidth();
				std::vector<CssBorderStyle> border_style = generateBorderStyle();
				std::vector<KRE::Color> border_color = generateBorderColor();

				auto first = node->getChildren().cbegin();
				auto last = node->getChildren().cend()-1;

				bool is_first = true;
				for(auto it = first; it != node->getChildren().cend(); ++it) {
					auto& child = *it;
					bool is_last = it == last;
					formatNode(child, parent, parent->getDimensions(), [&border_color, &border_style, &padding, &border_width, &is_first, is_last](BoxPtr box) {
						if(is_first) {
							is_first = false;
							box->setPaddingLeft(padding[1]);
							box->setBorderLeft(border_width[1]);
							box->setBorderStyleLeft(border_style[1]);
							box->setBorderColorLeft(border_color[1]);
						}
						if(is_last) {
							box->setBorderRight(border_width[3]);
							box->setPaddingRight(padding[3]);
							box->setBorderStyleRight(border_style[3]);
							box->setBorderColorRight(border_color[3]);
						}
						box->setBorderTop(border_width[0]);
						box->setBorderBottom(border_width[2]);
						box->setBorderStyleTop(border_style[0]);
						box->setBorderStyleBottom(border_style[2]);
						box->setBorderColorTop(border_color[0]);
						box->setBorderColorBottom(border_color[2]);
					});
				}
			
			}
		}

		std::vector<CssBorderStyle> generateBorderStyle() {
			std::vector<CssBorderStyle> res;
			res.resize(4);
			res[0] = ctx_.getComputedValue(Property::BORDER_TOP_STYLE).getValue<CssBorderStyle>();
			res[1] = ctx_.getComputedValue(Property::BORDER_LEFT_STYLE).getValue<CssBorderStyle>();
			res[2] = ctx_.getComputedValue(Property::BORDER_BOTTOM_STYLE).getValue<CssBorderStyle>();
			res[3] = ctx_.getComputedValue(Property::BORDER_RIGHT_STYLE).getValue<CssBorderStyle>();
			return res;
		}

		std::vector<KRE::Color> generateBorderColor() {
			std::vector<KRE::Color> res;
			res.resize(4);
			res[0] = ctx_.getComputedValue(Property::BORDER_TOP_COLOR).getValue<CssColor>().compute();
			res[1] = ctx_.getComputedValue(Property::BORDER_LEFT_COLOR).getValue<CssColor>().compute();
			res[2] = ctx_.getComputedValue(Property::BORDER_BOTTOM_COLOR).getValue<CssColor>().compute();
			res[3] = ctx_.getComputedValue(Property::BORDER_RIGHT_COLOR).getValue<CssColor>().compute();
			return res;
		}

		std::vector<FixedPoint> generateBorderWidth() {
			std::vector<FixedPoint> res;
			res.resize(4);
			res[0] = ctx_.getComputedValue(Property::BORDER_TOP_WIDTH).getValue<Length>().compute();
			res[1] = ctx_.getComputedValue(Property::BORDER_LEFT_WIDTH).getValue<Length>().compute();
			res[2] = ctx_.getComputedValue(Property::BORDER_BOTTOM_WIDTH).getValue<Length>().compute();
			res[3] = ctx_.getComputedValue(Property::BORDER_RIGHT_WIDTH).getValue<Length>().compute();
			return res;
		}

		std::vector<FixedPoint> generatePadding() {
			std::vector<FixedPoint> res;
			res.resize(4);
			res[0] = ctx_.getComputedValue(Property::PADDING_TOP).getValue<Length>().compute();
			res[1] = ctx_.getComputedValue(Property::PADDING_LEFT).getValue<Length>().compute();
			res[2] = ctx_.getComputedValue(Property::PADDING_BOTTOM).getValue<Length>().compute();
			res[3] = ctx_.getComputedValue(Property::PADDING_RIGHT).getValue<Length>().compute();
			return res;
		}

		void layoutInlineText(NodePtr node, BoxPtr parent, std::function<void(BoxPtr)> pre_layout_fn) {
			TextPtr tnode = std::dynamic_pointer_cast<Text>(node);
			ASSERT_LOG(tnode != nullptr, "Logic error, couldn't up-cast node to Text.");
			ASSERT_LOG(parent->getDimensions().content_.width != 0, "Parent content width is 0.");

			const FixedPoint lh = getLineHeight();
			BoxPtr open = getOpenBox(parent);
			FixedPoint width = getWidthAtCursor(parent->getDimensions().content_.width) - open_.top().cursor_.x;
			while(width == 0) {
				open_.top().cursor_.y += lh;
				width = getWidthAtCursor(parent->getDimensions().content_.width);
				open_.top().open_box_->setContentX(getXAtCursor());
				open_.top().open_box_->setContentY(open_.top().cursor_.y);
			}

			tnode->transformText(width >= 0);
			auto it = tnode->begin();
			while(it != tnode->end()) {
				LinePtr line = tnode->reflowText(it, width);
				if(line != nullptr && !line->line.empty()) {
					auto txt = std::make_shared<TextBox>(parent, line);
					txt->setDescent(getDescent());
					open->addChild(txt);
					if(pre_layout_fn) {
						pre_layout_fn(txt);
					}
					txt->setContentX(open_.top().cursor_.x + txt->getMPBLeft());
					txt->setContentY(0);
					txt->setContentHeight(lh);
					FixedPoint x_inc = txt->getDimensions().content_.width + txt->getMBPWidth();
					open_.top().cursor_.x += x_inc;
					width -= x_inc;
				}
				if(line != nullptr && line->is_end_line) {
					closeOpenBox(parent);
					open = getOpenBox(parent);
					width = getWidthAtCursor(parent->getDimensions().content_.width);
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
				open_.top().open_box_->setContentX(getXAtCursor());
				open_.top().open_box_->setContentY(open_.top().cursor_.y);
				open_.top().open_box_->setContentWidth(getWidthAtCursor(parent->getDimensions().content_.width));
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

		FixedPoint getDescent() const {
			return ctx_.getFontHandle()->getDescender() * (fixed_point_scale / 64);
		}

		bool isOpenBox() const { return !open_.empty() && open_.top().open_box_ != nullptr; }

		BoxPtr getRoot() const { return root_; }
		
		const point& getCursor() const { 
			if(open_.empty()) {
				static point default_point;
				return default_point;
			}
			return open_.top().cursor_; 
		}
		void setCursor(const point& cursor) { open_.top().cursor_ = cursor; }
		void incrCursor(FixedPoint x) { open_.top().cursor_.x += x; }

		FixedPoint getWidthAtCursor(FixedPoint width) const {
			return getWidthAtPosition(getCursor().y, width);
		}

		FixedPoint getXAtCursor() const {
			return getXAtPosition(getCursor().y);
		}

		FixedPoint getXAtPosition(FixedPoint y) const {
			FixedPoint x = 0;
			// since we expect only a small number of floats per element
			// a linear search through them seems fine at this point.
			for(auto& lf : root_->getLeftFloats()) {
				auto& dims = lf->getDimensions();
				if(y >= (lf->getMPBTop() + dims.content_.y) && y <= (lf->getMPBTop() + lf->getMBPHeight() + dims.content_.height + dims.content_.y)) {
					x = std::max(x, lf->getMBPWidth() + lf->getDimensions().content_.x + dims.content_.width);
				}
			}
			return x;
		}

		FixedPoint getX2AtPosition(FixedPoint y) const {
			FixedPoint x2 = dims_.content_.width;
			// since we expect only a small number of floats per element
			// a linear search through them seems fine at this point.
			for(auto& rf : root_->getRightFloats()) {
				auto& dims = rf->getDimensions();
				if(y >= (rf->getMPBTop() + dims.content_.y) && y <= (rf->getMPBTop() + rf->getMBPHeight() + dims.content_.height + dims.content_.y)) {
					x2 = std::min(x2, rf->getMBPWidth() + dims.content_.width);
				}
			}
			return x2;
		}

		FixedPoint getWidthAtPosition(FixedPoint y, FixedPoint width) const {
			// since we expect only a small number of floats per element
			// a linear search through them seems fine at this point.
			for(auto& lf : root_->getLeftFloats()) {
				auto& dims = lf->getDimensions();
				if(y >= (lf->getMPBTop() + dims.content_.y) && y <= (lf->getMPBTop() + lf->getMBPHeight() + dims.content_.height + dims.content_.y)) {
					width -= lf->getMBPWidth() + lf->getDimensions().content_.x + dims.content_.width;
				}
			}
			for(auto& rf : root_->getRightFloats()) {
				auto& dims = rf->getDimensions();
				if(y >= (rf->getMPBTop() + dims.content_.y) && y <= (rf->getMPBTop() + rf->getMBPHeight() + dims.content_.height + dims.content_.y)) {
					width -= rf->getMBPWidth() + rf->getDimensions().content_.x + dims.content_.width;
				}
			}
			return width < 0 ? 0 : width;
		}

		const Dimensions& getDimensions() const { return dims_; }
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
		std::stack<point> offset_;

		std::stack<int> list_item_counter_;
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
		  left_floats_(),
		  right_floats_(),
		  cfloat_(CssFloat::NONE),
		  font_handle_(xhtml::RenderContext::get().getFontHandle()),
		  binfo_(),
		  css_position_(CssPosition::STATIC),
		  border_{}
	{
	}

	void Box::init()
	{
		RenderContext& ctx = RenderContext::get();
		color_ = ctx.getComputedValue(Property::COLOR).getValue<CssColor>().compute();

		binfo_.setColor(ctx.getComputedValue(Property::BACKGROUND_COLOR).getValue<CssColor>().compute());
		// We set repeat before the filename so we can correctly set the background texture wrap mode.
		binfo_.setRepeat(ctx.getComputedValue(Property::BACKGROUND_REPEAT).getValue<CssBackgroundRepeat>());
		binfo_.setPosition(ctx.getComputedValue(Property::BACKGROUND_POSITION).getValue<BackgroundPosition>());
		auto uri = ctx.getComputedValue(Property::BACKGROUND_IMAGE).getValue<UriStyle>();
		if(!uri.isNone()) {
			binfo_.setFile(uri.getUri());
		}
		css_position_ = ctx.getComputedValue(Property::POSITION).getValue<CssPosition>();

		const Property b[4]  = { Property::BORDER_TOP_WIDTH, Property::BORDER_LEFT_WIDTH, Property::BORDER_BOTTOM_WIDTH, Property::BORDER_RIGHT_WIDTH };
		const Property bs[4]  = { Property::BORDER_TOP_STYLE, Property::BORDER_LEFT_STYLE, Property::BORDER_BOTTOM_STYLE, Property::BORDER_RIGHT_STYLE };
		const Property p[4]  = { Property::PADDING_TOP, Property::PADDING_LEFT, Property::PADDING_BOTTOM, Property::PADDING_RIGHT };
		const Property m[4]  = { Property::MARGIN_TOP, Property::MARGIN_LEFT, Property::MARGIN_BOTTOM, Property::MARGIN_RIGHT };
		const Property bc[4] = { Property::BORDER_TOP_COLOR, Property::BORDER_LEFT_COLOR, Property::BORDER_BOTTOM_COLOR, Property::BORDER_RIGHT_COLOR };

		for(int n = 0; n != 4; ++n) {
			border_color_[n] = ctx.getComputedValue(bc[n]).getValue<CssColor>().compute();
			border_style_[n] = ctx.getComputedValue(bs[n]).getValue<CssBorderStyle>();
			border_[n] = ctx.getComputedValue(b[n]).getValue<Length>();
			padding_[n] = ctx.getComputedValue(p[n]).getValue<Length>();
			margin_[n] = ctx.getComputedValue(m[n]).getValue<Width>();
		}

		css_left_ = ctx.getComputedValue(Property::LEFT).getValue<Width>();
		css_top_ = ctx.getComputedValue(Property::TOP).getValue<Width>();
		css_width_ = ctx.getComputedValue(Property::WIDTH).getValue<Width>();
		css_height_ = ctx.getComputedValue(Property::HEIGHT).getValue<Width>();
	}

	BoxPtr Box::createLayout(NodePtr node, int containing_width, int containing_height)
	{
		LayoutEngine e;
		// search for the body element then render that content.
		node->preOrderTraversal([&e, containing_width, containing_height](NodePtr node){
			if(node->id() == NodeId::ELEMENT && node->hasTag(ElementId::BODY)) {
				e.formatNode(node, nullptr, point(containing_width * fixed_point_scale, containing_height * fixed_point_scale));
				return false;
			}
			return true;
		});
		node->layoutComplete();
		return e.getRoot();
	}

	void Box::addFloatBox(LayoutEngine& eng, BoxPtr box, CssFloat cfloat, FixedPoint y, const point& offset)
	{
		box->layout(eng, getDimensions());

		const FixedPoint lh = eng.getLineHeight();
		const FixedPoint box_w = box->getDimensions().content_.width;

		FixedPoint x = cfloat == CssFloat::LEFT ? eng.getXAtPosition(y) : eng.getX2AtPosition(y);
		FixedPoint w = eng.getWidthAtPosition(y, dimensions_.content_.width);
		bool placed = false;
		while(!placed) {
			if(w > box_w) {
				box->setContentX(x - (cfloat == CssFloat::LEFT ? 0 : box_w) + offset.x);
				box->setContentY(y + offset.y);
				placed = true;
			} else {
				y += lh;
				x = cfloat == CssFloat::LEFT ? eng.getXAtPosition(y) : eng.getX2AtPosition(y);
				w = eng.getWidthAtPosition(y, dimensions_.content_.width);
			}
		}

		if(cfloat == CssFloat::LEFT) {
			left_floats_.emplace_back(box);
		} else {
			right_floats_.emplace_back(box);
		}
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
		for(auto& lf : left_floats_) {
			lf->preOrderTraversal(fn, nesting+1);
		}
		for(auto& rf : left_floats_) {
			rf->preOrderTraversal(fn, nesting+1);
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

	void Box::layoutFixed(LayoutEngine& eng, const Dimensions& containing)
	{
		for(auto& fix : fixed_boxes_) {
			fix->layout(eng, eng.getDimensions());
		}
	}

	void Box::layout(LayoutEngine& eng, const Dimensions& containing)
	{
		RenderContext::Manager ctx_manager(getNode()->getProperties());
		handleLayout(eng, containing);
		layoutAbsolute(eng, containing);
		layoutFixed(eng, containing);

		border_info_.init(dimensions_);
	}

	BoxPtr Box::addFixedElement(NodePtr node)
	{
		fixed_boxes_.emplace_back(std::make_shared<BlockBox>(shared_from_this(), node));
		fixed_boxes_.back()->init();
		return fixed_boxes_.back();
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

	BlockBox::BlockBox(BoxPtr parent, NodePtr node)
		: Box(BoxId::BLOCK, parent, node)
	{
	}

	void BlockBox::handleLayout(LayoutEngine& eng, const Dimensions& containing)
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
		calculateHorzMPB(containing_width);
	}

	void BlockBox::layoutWidth(const Dimensions& containing)
	{
		RenderContext& ctx = RenderContext::get();
		const FixedPoint containing_width = containing.content_.width;

		auto css_width = getCssWidth();
		FixedPoint width = 0;
		if(!css_width.isAuto()) {
			width = css_width.getLength().compute(containing_width);
			setContentWidth(width);
		}

		setMBP(containing_width);
		auto css_margin_left = getCssMargin(Side::LEFT);
		auto css_margin_right = getCssMargin(Side::RIGHT);

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
				const auto rmargin = css_margin_right.getLength().compute(containing_width);
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

	void Box::calculateVertMPB(FixedPoint containing_height)
	{
		if(getCssBorderStyle(Side::TOP) != CssBorderStyle::NONE 
			&& getCssBorderStyle(Side::TOP) != CssBorderStyle::HIDDEN) {
			setBorderTop(getCssBorder(Side::TOP).compute());
		}
		if(getCssBorderStyle(Side::BOTTOM) != CssBorderStyle::NONE 
			&& getCssBorderStyle(Side::BOTTOM) != CssBorderStyle::HIDDEN) {
			setBorderBottom(getCssBorder(Side::BOTTOM).compute());
		}

		setPaddingTop(getCssPadding(Side::TOP).compute(containing_height));
		setPaddingBottom(getCssPadding(Side::BOTTOM).compute(containing_height));

		setMarginTop(getCssMargin(Side::TOP).getLength().compute(containing_height));
		setMarginBottom(getCssMargin(Side::BOTTOM).getLength().compute(containing_height));
	}

	void Box::calculateHorzMPB(FixedPoint containing_width)
	{		
		if(getCssBorderStyle(Side::LEFT) != CssBorderStyle::NONE) {
			setBorderLeft(getCssBorder(Side::LEFT).compute());
		}
		if(getCssBorderStyle(Side::RIGHT) != CssBorderStyle::NONE) {
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

	void BlockBox::layoutPosition(const Dimensions& containing)
	{
		const FixedPoint containing_height = containing.content_.height;

		calculateVertMPB(containing_height);

		setContentX(getMPBLeft());
		setContentY(containing.content_.height + getMPBTop());
		//LOG_DEBUG("  XXX: y: " << getDimensions().content_.y);

		if(getPosition() == CssPosition::FIXED) {
			if(!getCssLeft().isAuto()) {
				setContentX(getCssLeft().getLength().compute(containing.content_.width) + getMPBLeft());
			}
			if(!getCssTop().isAuto()) {
				setContentY(getCssTop().getLength().compute(containing.content_.height) + getMPBTop());
			}
		} 
	}

	void BlockBox::layoutChildren(LayoutEngine& eng)
	{
		NodePtr node = getNode();
		if(node != nullptr) {
			for(auto& child : node->getChildren()) {
				if(getPosition() == CssPosition::FIXED) {
					eng.pushOpenBox();
				}
				BoxPtr box_child = eng.formatNode(child, shared_from_this(), getDimensions());
				if(box_child != nullptr) {
					setContentHeight(box_child->getDimensions().content_.y + box_child->getDimensions().content_.height + box_child->getMBPBottom());
				}
				if(getPosition() == CssPosition::FIXED) {
					eng.closeOpenBox(shared_from_this());
					eng.popOpenBox();
				}
			}
		}
		// close any open boxes.
		eng.closeOpenBox(shared_from_this());

		for(auto& child : getChildren()) {
			setContentHeight(child->getDimensions().content_.y + child->getDimensions().content_.height + child->getMBPHeight());
		}
	}

	void BlockBox::layoutHeight(const Dimensions& containing)
	{
		RenderContext& ctx = RenderContext::get();
		// a set height value overrides the calculated value.
		if(!getCssHeight().isAuto()) {
			setContentHeight(getCssHeight().getLength().compute(containing.content_.height));
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

	void LineBox::handleLayout(LayoutEngine& eng, const Dimensions& containing)
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

	void TextBox::handleLayout(LayoutEngine& eng, const Dimensions& containing)
	{
	}

	AbsoluteBox::AbsoluteBox(BoxPtr parent, NodePtr node)
		: Box(BoxId::ABSOLUTE, parent, node)
	{
		RenderContext& ctx = RenderContext::get();
		
		css_rect_[0] = ctx.getComputedValue(Property::LEFT).getValue<Width>();
		css_rect_[1] = ctx.getComputedValue(Property::TOP).getValue<Width>();
		css_rect_[2] = ctx.getComputedValue(Property::RIGHT).getValue<Width>();
		css_rect_[3] = ctx.getComputedValue(Property::BOTTOM).getValue<Width>();
	}

	void AbsoluteBox::handleLayout(LayoutEngine& eng, const Dimensions& containing)
	{
		Rect container = containing.content_;

		// Find the first ancestor with non-static position
		auto parent = getParent();
		if(parent != nullptr && !parent->ancestralTraverse([&container](const ConstBoxPtr& box) {
			if(box->getPosition() != CssPosition::STATIC) {
				container = box->getDimensions().content_;
				return false;
			}
			return true;
		})) {
			// couldn't find anything use the layout engine dimensions
			container = eng.getDimensions().content_;
		}
		// we expect top/left and either bottom/right or width/height
		// if the appropriate thing isn't set then we use the parent container dimensions.
		RenderContext& ctx = RenderContext::get();
		const FixedPoint containing_width = container.width;
		const FixedPoint containing_height = container.height;
		
		FixedPoint left = container.x;
		if(!css_rect_[0].isAuto()) {
			left = css_rect_[0].getLength().compute(containing_width);
		}
		FixedPoint top = container.y;
		if(!css_rect_[1].isAuto()) {
			top = css_rect_[1].getLength().compute(containing_height);
		}
		FixedPoint width = container.width;
		if(!css_rect_[2].isAuto()) {
			width = css_rect_[2].getLength().compute(containing_width) - left + container.width;
		}
		FixedPoint height = container.height;
		if(!css_rect_[3].isAuto()) {
			height = css_rect_[3].getLength().compute(containing_height) - top + container.height;
		}

		// if width/height properties are set they override right/bottom.
		if(!getCssWidth().isAuto()) {
			width = getCssWidth().getLength().compute(containing_width);
		}

		if(!getCssHeight().isAuto()) {
			height = getCssHeight().getLength().compute(containing_height);
		}

		calculateHorzMPB(containing_width);
		calculateVertMPB(containing_height);

		setContentX(left + getMPBLeft());
		setContentY(top + getMPBTop());
		setContentWidth(width - getMBPWidth());
		setContentHeight(height - getMBPHeight());

		NodePtr node = getNode();
		if(node != nullptr) {
			eng.pushOpenBox();
			for(auto& child : node->getChildren()) {
				eng.formatNode(child, shared_from_this(), getDimensions());
			}
			eng.closeOpenBox(shared_from_this());
			eng.popOpenBox();
		}
	}

	InlineElementBox::InlineElementBox(BoxPtr parent, NodePtr node)
		: Box(BoxId::INLINE_ELEMENT, parent, node)
	{
	}

	void InlineElementBox::handleLayout(LayoutEngine& eng, const Dimensions& containing)
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

	ListItemBox::ListItemBox(BoxPtr parent, NodePtr node, int count)
		: Box(BoxId::LIST_ITEM, parent, node),
		  content_(std::make_shared<BlockBox>(parent, node)),
		  count_(count),
		  marker_(utils::codepoint_to_utf8(marker_disc))
	{		
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

	std::string BlockBox::toString() const
	{
		std::ostringstream ss;
		ss << "BlockBox: " << getDimensions().content_;
		for(auto& lf : getLeftFloats()) {
			ss << " LeftFloatBox: " << lf->toString();
		}
		for(auto& rf : getRightFloats()) {
			ss << " RightFloatBox: " << rf->toString();
		}
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

	std::string ListItemBox::toString() const 
	{
		std::ostringstream ss;
		ss << "ListItemBox: " << getDimensions().content_ << ", content: " << content_->toString();
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
		for(auto& lf : left_floats_) {
			lf->render(display_list, offs);
		}
		for(auto& rf : right_floats_) {
			rf->render(display_list, offs);
		}
		for(auto& ab : absolute_boxes_) {
			ab->render(display_list, point(0, 0));
		}
		// render fixed boxes.
		for(auto& fix : fixed_boxes_) {
			fix->render(display_list, point(0, 0));
		}

		// set the active rect on any parent node.
		if(node != nullptr) {
			auto& dims = getDimensions();
			const int x = (offs.x - dims.padding_.left - dims.border_.left) / fixed_point_scale;
			const int y = (offs.y - dims.padding_.top - dims.border_.top) / fixed_point_scale;
			const int w = (dims.content_.width + dims.padding_.left + dims.padding_.right + dims.border_.left + dims.border_.right) / fixed_point_scale;
			const int h = (dims.content_.height + dims.padding_.top + dims.padding_.bottom + dims.border_.top + dims.border_.bottom) / fixed_point_scale;
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
		binfo_.render(display_list, offset, dims);
	}

	void Box::handleRenderBorder(DisplayListPtr display_list, const point& offset) const
	{
		// we try and render border image first, if we can't then we default to the any given
		// style.
		if(!border_info_.render(display_list, offset, getDimensions())) {
			auto border = std::make_shared<SolidRenderable>();
			std::vector<KRE::vertex_color> vc;

			auto& ctx = RenderContext::get();

			FixedPoint bw[4];
			bw[0] = dimensions_.border_.left; 
			bw[1] = dimensions_.border_.top; 
			bw[2] = dimensions_.border_.right; 
			bw[3] = dimensions_.border_.bottom; 

			// this is the left/top edges of the appropriate side
			const float side_left    = static_cast<float>(offset.x - dimensions_.padding_.left   - dimensions_.border_.left) / fixed_point_scale_float;
			const float side_top     = static_cast<float>(offset.y - dimensions_.padding_.top    - dimensions_.border_.top) / fixed_point_scale_float;
			const float side_right   = static_cast<float>(offset.x + dimensions_.content_.width  + dimensions_.padding_.right) / fixed_point_scale_float;
			const float side_bottom  = static_cast<float>(offset.y + dimensions_.content_.height + dimensions_.padding_.bottom) / fixed_point_scale_float;
			const float left_width   = static_cast<float>(dimensions_.border_.left) / fixed_point_scale_float;
			const float top_width    = static_cast<float>(dimensions_.border_.top) / fixed_point_scale_float;
			const float right_width  = static_cast<float>(dimensions_.border_.right) / fixed_point_scale_float;
			const float bottom_width = static_cast<float>(dimensions_.border_.bottom) / fixed_point_scale_float;

			vc.reserve(24*4);
			for(int side = 0; side != 4; ++side) {
				// only drawing border if width > 0, alpha != 0
				if(bw[side] > 0 && border_color_[side].ai() != 0) {
					switch(border_style_[side]) {
						case CssBorderStyle::SOLID:
							switch(side) {
								case 0: generate_solid_left_side(&vc, side_left, left_width, side_top, top_width, side_bottom, bottom_width, border_color_[side].as_u8vec4()); break;
								case 1: generate_solid_top_side(&vc, side_left, left_width, side_right, right_width, side_top, top_width, border_color_[side].as_u8vec4()); break;
								case 2: generate_solid_right_side(&vc, side_right, right_width, side_top, top_width, side_bottom, bottom_width, border_color_[side].as_u8vec4()); break;
								case 3: generate_solid_bottom_side(&vc, side_left, left_width, side_right, right_width, side_bottom, bottom_width, border_color_[side].as_u8vec4()); break;
							}
							break;
						case CssBorderStyle::INSET: {
							glm::u8vec4 inset(border_color_[side].as_u8vec4().r/2, border_color_[side].as_u8vec4().g/2, border_color_[side].as_u8vec4().b/2, border_color_[side].as_u8vec4().a);
							switch(side) {
								case 0: generate_solid_left_side(&vc, side_left, left_width, side_top, top_width, side_bottom, bottom_width, inset); break;
								case 1: generate_solid_top_side(&vc, side_left, left_width, side_right, right_width, side_top, top_width, inset); break;
								case 2: generate_solid_right_side(&vc, side_right, right_width, side_top, top_width, side_bottom, bottom_width, border_color_[side].as_u8vec4()); break;
								case 3: generate_solid_bottom_side(&vc, side_left, left_width, side_right, right_width, side_bottom, bottom_width, border_color_[side].as_u8vec4()); break;
							}
							break;
						}
						case CssBorderStyle::OUTSET: {
							glm::u8vec4 outset(border_color_[side].as_u8vec4().r/2, border_color_[side].as_u8vec4().g/2, border_color_[side].as_u8vec4().b/2, border_color_[side].as_u8vec4().a);
							switch(side) {
								case 0: generate_solid_left_side(&vc, side_left, left_width, side_top, top_width, side_bottom, bottom_width, border_color_[side].as_u8vec4()); break;
								case 1: generate_solid_top_side(&vc, side_left, left_width, side_right, right_width, side_top, top_width, border_color_[side].as_u8vec4()); break;
								case 2: generate_solid_right_side(&vc, side_right, right_width, side_top, top_width, side_bottom, bottom_width, outset); break;
								case 3: generate_solid_bottom_side(&vc, side_left, left_width, side_right, right_width, side_bottom, bottom_width, outset); break;
							}
							break;
						}
						case CssBorderStyle::DOUBLE:
							switch(side) {
								case 0: 
									generate_solid_left_side(&vc, side_left, left_width/3.0f, side_top, top_width/3.0f, side_bottom+2.0f*bottom_width/3.0f, bottom_width/3.0f, border_color_[side].as_u8vec4()); 
									generate_solid_left_side(&vc, side_left+2.0f*left_width/3.0f, left_width/3.0f, side_top+2.0f*top_width/3.0f, top_width/3.0f, side_bottom, bottom_width/3.0f, border_color_[side].as_u8vec4()); 
									break;
								case 1: 
									generate_solid_top_side(&vc, side_left, left_width/3.0f, side_right+2.0f*right_width/3.0f, right_width/3.0f, side_top, top_width/3.0f, border_color_[side].as_u8vec4()); 
									generate_solid_top_side(&vc, side_left+2.0f*left_width/3.0f, left_width/3.0f, side_right, right_width/3.0f, side_top+2.0f*top_width/3.0f, top_width/3.0f, border_color_[side].as_u8vec4()); 
									break;
								case 2: 
									generate_solid_right_side(&vc, side_right, right_width/3.0f, side_top+2.0f*top_width/3.0f, top_width/3.0f, side_bottom, bottom_width/3.0f, border_color_[side].as_u8vec4()); 
									generate_solid_right_side(&vc, side_right+2.0f*right_width/3.0f, right_width/3.0f, side_top, top_width/3.0f, side_bottom+2.0f*bottom_width/3.0f, bottom_width/3.0f, border_color_[side].as_u8vec4()); 
									break;
								case 3: 
									generate_solid_bottom_side(&vc, side_left+2.0f*left_width/3.0f, left_width/3.0f, side_right, right_width/3.0f, side_bottom, bottom_width/3.0f, border_color_[side].as_u8vec4()); 
									generate_solid_bottom_side(&vc, side_left, left_width/3.0f, side_right+2.0f*right_width/3.0f, right_width/3.0f, side_bottom+2.0f*bottom_width/3.0f, bottom_width/3.0f, border_color_[side].as_u8vec4()); 
									break;
							}
							break;
						case CssBorderStyle::GROOVE: {
							glm::u8vec4 inset(border_color_[side].as_u8vec4().r/2, border_color_[side].as_u8vec4().g/2, border_color_[side].as_u8vec4().b/2, border_color_[side].as_u8vec4().a);
							glm::u8vec4 outset(border_color_[side].as_u8vec4());
							switch(side) {
								case 0: 
									generate_solid_left_side(&vc, side_left, left_width/2.0f, side_top, top_width/2.0f, side_bottom+bottom_width/2.0f, bottom_width/2.0f, inset); 
									generate_solid_left_side(&vc, side_left+left_width/2.0f, left_width/2.0f, side_top+top_width/2.0f, top_width/2.0f, side_bottom, bottom_width/2.0f, outset);
									break;
								case 1: 
									generate_solid_top_side(&vc, side_left, left_width/2.0f, side_right+right_width/2.0f, right_width/2.0f, side_top, top_width/2.0f, inset); 
									generate_solid_top_side(&vc, side_left+left_width/2.0f, left_width/2.0f, side_right+right_width/2.0f, right_width/2.0f, side_top+top_width/2.0f, top_width/2.0f, outset); 
									break;
								case 2: 
									generate_solid_right_side(&vc, side_right, right_width/2.0f, side_top+top_width/2.0f, top_width/2.0f, side_bottom, bottom_width/2.0f, inset); 
									generate_solid_right_side(&vc, side_right+right_width/2.0f, right_width/2.0f, side_top, top_width/2.0f, side_bottom+bottom_width/2.0f, bottom_width/2.0f, outset); 
									break;
								case 3: 
									generate_solid_bottom_side(&vc, side_left+left_width/2.0f, left_width/2.0f, side_right, right_width/2.0f, side_bottom, bottom_width/2.0f, inset); 
									generate_solid_bottom_side(&vc, side_left, left_width/2.0f, side_right+right_width/2.0f, right_width/2.0f, side_bottom+bottom_width/2.0f, bottom_width/2.0f, outset);
									break;
							}
							break;
						}
						case CssBorderStyle::RIDGE: {
							glm::u8vec4 outset(border_color_[side].as_u8vec4().r/2, border_color_[side].as_u8vec4().g/2, border_color_[side].as_u8vec4().b/2, border_color_[side].as_u8vec4().a);
							glm::u8vec4 inset(border_color_[side].as_u8vec4());
							switch(side) {
								case 0: 
									generate_solid_left_side(&vc, side_left, left_width/2.0f, side_top, top_width/2.0f, side_bottom+bottom_width/2.0f, bottom_width/2.0f, inset); 
									generate_solid_left_side(&vc, side_left+left_width/2.0f, left_width/2.0f, side_top+top_width/2.0f, top_width/2.0f, side_bottom, bottom_width/2.0f, outset);
									break;
								case 1: 
									generate_solid_top_side(&vc, side_left, left_width/2.0f, side_right+right_width/2.0f, right_width/2.0f, side_top, top_width/2.0f, inset); 
									generate_solid_top_side(&vc, side_left+left_width/2.0f, left_width/2.0f, side_right+right_width/2.0f, right_width/2.0f, side_top+top_width/2.0f, top_width/2.0f, outset); 
									break;
								case 2: 
									generate_solid_right_side(&vc, side_right, right_width/2.0f, side_top+top_width/2.0f, top_width/2.0f, side_bottom, bottom_width/2.0f, inset); 
									generate_solid_right_side(&vc, side_right+right_width/2.0f, right_width/2.0f, side_top, top_width/2.0f, side_bottom+bottom_width/2.0f, bottom_width/2.0f, outset); 
									break;
								case 3: 
									generate_solid_bottom_side(&vc, side_left+left_width/2.0f, left_width/2.0f, side_right, right_width/2.0f, side_bottom, bottom_width/2.0f, inset); 
									generate_solid_bottom_side(&vc, side_left, left_width/2.0f, side_right+right_width/2.0f, right_width/2.0f, side_bottom+bottom_width/2.0f, bottom_width/2.0f, outset);
									break;
							}
							break;
						}
						case CssBorderStyle::DOTTED:
						case CssBorderStyle::DASHED:
							ASSERT_LOG(false, "No support for border style of: " << static_cast<int>(border_style_[side]));
							break;
						case CssBorderStyle::HIDDEN:
						case CssBorderStyle::NONE:
						default:
							// these skip drawing.
							break;
					}
				}
			}

			border->update(&vc);

			display_list->addRenderable(border);
		}
	}

	void BlockBox::handleRender(DisplayListPtr display_list, const point& offset) const
	{
		NodePtr node = getNode();
		if(node != nullptr && node->isReplaced()) {
			auto r = node->getRenderable();
			r->setPosition(glm::vec3(static_cast<float>(offset.x)/65536.0f,
				static_cast<float>(offset.y)/65536.0f,
				0.0f));
			display_list->addRenderable(r);
		}
	}

	void ListItemBox::handleRender(DisplayListPtr display_list, const point& offset) const 
	{
		auto& ctx = RenderContext::get();

		auto path = getFont()->getGlyphPath(marker_);
		auto fontr = getFont()->createRenderableFromPath(nullptr, marker_, path);
		fontr->setColor(getColor());
		FixedPoint path_width = path.back().x - path.front().x + getFont()->calculateCharAdvance(' ');
		fontr->setPosition(static_cast<float>(offset.x - path_width - 5) / fixed_point_scale_float, static_cast<float>(offset.y) / fixed_point_scale_float);
		display_list->addRenderable(fontr);

		// XXX
		content_->render(display_list, offset);
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
		auto fontr = getFont()->createRenderableFromPath(nullptr, text, path);
		fontr->setColor(getColor());
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

	BackgroundInfo::BackgroundInfo()
		: color_(0, 0, 0, 0),
		  texture_(),
		  repeat_(CssBackgroundRepeat::REPEAT),
		  position_()
	{
	}

	void BackgroundInfo::setFile(const std::string& filename)
	{
		texture_ = KRE::Texture::createTexture(filename);
		switch(repeat_) {
			case CssBackgroundRepeat::REPEAT:
				texture_->setAddressModes(0, KRE::Texture::AddressMode::WRAP, KRE::Texture::AddressMode::WRAP, KRE::Texture::AddressMode::WRAP);
				break;
			case CssBackgroundRepeat::REPEAT_X:
				texture_->setAddressModes(0, KRE::Texture::AddressMode::WRAP, KRE::Texture::AddressMode::BORDER, KRE::Texture::AddressMode::BORDER, KRE::Color(0,0,0,0));
				break;
			case CssBackgroundRepeat::REPEAT_Y:
				texture_->setAddressModes(0, KRE::Texture::AddressMode::BORDER, KRE::Texture::AddressMode::WRAP, KRE::Texture::AddressMode::BORDER, KRE::Color(0,0,0,0));
				break;
			case CssBackgroundRepeat::NO_REPEAT:
				texture_->setAddressModes(0, KRE::Texture::AddressMode::BORDER, KRE::Texture::AddressMode::BORDER, KRE::Texture::AddressMode::BORDER, KRE::Color(0,0,0,0));
				break;
		}
	}

	void BackgroundInfo::render(DisplayListPtr display_list, const point& offset, const Dimensions& dims) const
	{
		// XXX if we're rendering the body element then it takes the entire canvas :-/
		// technically the rule is that if no background styles are applied to the html element then
		// we apply the body styles.
		const int rx = offset.x - dims.padding_.left - dims.border_.left;
		const int ry = offset.y - dims.padding_.top - dims.border_.top;
		const int rw = dims.content_.width + dims.padding_.left + dims.padding_.right + dims.border_.left + dims.border_.right;
		const int rh = dims.content_.height + dims.padding_.top + dims.padding_.bottom + dims.border_.top + dims.border_.bottom;
		rect r(rx, ry, rw, rh);

		if(color_.ai() != 0) {
			display_list->addRenderable(std::make_shared<SolidRenderable>(r, color_));
		}
		// XXX if texture is set then use background position and repeat as appropriate.
		if(texture_) {
			// XXX this needs fixing.
			// we porbably should clone the texture we pass to blittable.
			// With a value pair of '14% 84%', the point 14% across and 84% down the image is to be placed at the point 14% across and 84% down the padding box.
			const int sw = texture_->surfaceWidth();
			const int sh = texture_->surfaceHeight();

			int sw_offs = 0;
			int sh_offs = 0;

			if(position_.getLeft().isPercent()) {
				sw_offs = position_.getLeft().compute(sw * fixed_point_scale);
			}
			if(position_.getTop().isPercent()) {
				sh_offs = position_.getTop().compute(sh * fixed_point_scale);
			}

			const int rw_offs = position_.getLeft().compute(rw);
			const int rh_offs = position_.getTop().compute(rh);
			
			const float rxf = static_cast<float>(rx) / fixed_point_scale_float;
			const float ryf = static_cast<float>(ry) / fixed_point_scale_float;

			const float left = static_cast<float>(rw_offs - sw_offs + rx) / fixed_point_scale_float;
			const float top = static_cast<float>(rh_offs - sh_offs + ry) / fixed_point_scale_float;
			const float width = static_cast<float>(rw) / fixed_point_scale_float;
			const float height = static_cast<float>(rh) / fixed_point_scale_float;

			auto tex = texture_->clone();
			auto ptr = std::make_shared<KRE::Blittable>(tex);
			ptr->setCentre(KRE::Blittable::Centre::TOP_LEFT);
			switch(repeat_) {
				case CssBackgroundRepeat::REPEAT:
					tex->setSourceRect(0, rect(-left, -top, width, height));
					ptr->setDrawRect(rectf(rxf, ryf, width, height));
					break;
				case CssBackgroundRepeat::REPEAT_X:
					tex->setSourceRect(0, rect(-left, 0, width, sh));
					ptr->setDrawRect(rectf(rxf, top, width, static_cast<float>(sh)));
					break;
				case CssBackgroundRepeat::REPEAT_Y:
					tex->setSourceRect(0, rect(0, -top, sw, height));
					ptr->setDrawRect(rectf(left, ryf, static_cast<float>(sw), height));
					break;
				case CssBackgroundRepeat::NO_REPEAT:
					tex->setSourceRect(0, rect(0, 0, sw, sh));
					ptr->setDrawRect(rectf(left, top, static_cast<float>(sw), static_cast<float>(sh)));
					break;
			}

			display_list->addRenderable(ptr);
		}
	}

	void TextBox::handleRenderBackground(DisplayListPtr display_list, const point& offset) const
	{
		point offs = offset - point(0, getDimensions().content_.height + getDescent());
		
		Box::handleRenderBackground(display_list, offs);
	}

	void TextBox::handleRenderBorder(DisplayListPtr display_list, const point& offset) const
	{
		point offs = offset - point(0, getDimensions().content_.height + getDescent());
		Box::handleRenderBorder(display_list, offs);
	}

	BorderInfo::BorderInfo()
		: image_(),
		  fill_(false),
		  slice_{},
		  outset_{},
		  widths_{},
		  repeat_horiz_(CssBorderImageRepeat::STRETCH),
		  repeat_vert_(CssBorderImageRepeat::STRETCH)
	{
	}

	void BorderInfo::setFile(const std::string& filename)
	{
		image_ = KRE::Texture::createTexture(filename);
		image_->setAddressModes(0, KRE::Texture::AddressMode::WRAP, KRE::Texture::AddressMode::WRAP, KRE::Texture::AddressMode::WRAP);
	}

	void BorderInfo::init(const Dimensions& dims)
	{
		std::array<FixedPoint, 4> border = { dims.border_.top, dims.border_.left, dims.border_.bottom, dims.border_.right };

		auto& ctx = RenderContext::get();
		auto uri = ctx.getComputedValue(Property::BORDER_IMAGE_SOURCE).getValue<UriStyle>();
		if(uri.isNone()) {
			return;
		}
		setFile(uri.getUri());

		auto& outset = ctx.getComputedValue(Property::BORDER_IMAGE_OUTSET).getValue<WidthList>().getWidths();
		for(auto side = 0; side != 4; ++side) {
			if(outset[side].getLength().isNumber()) {
				// If the outset is a plain number, it is take as the multiple of border_widths
				outset_[side] = static_cast<float>(outset[side].getLength().compute()) / fixed_point_scale_float 
					* static_cast<float>(border[side]) / fixed_point_scale_float;
			} else {
				// is a dimensioned length.
				outset_[side] = static_cast<float>(outset[side].getLength().compute()) / fixed_point_scale_float;
			}
		}

		// We adjust the border image area by the outset values.
		const FixedPoint border_image_width = dims.content_.width 
			+ dims.padding_.left 
			+ dims.padding_.right 
			+ dims.border_.left 
			+ dims.border_.right
			+ static_cast<FixedPoint>((outset_[1] + outset_[3]) * fixed_point_scale);
		const FixedPoint border_image_height = dims.content_.height 
			+ dims.padding_.top 
			+ dims.padding_.bottom 
			+ dims.border_.left 
			+ dims.border_.right 
			+ static_cast<FixedPoint>((outset_[0] + outset_[2]) * fixed_point_scale);

		auto border_slice = ctx.getComputedValue(Property::BORDER_IMAGE_SLICE).getValue<BorderImageSlice>();
		auto& slices = border_slice.getWidths();
		for(auto side = 0; side != 4; ++side) {
			const Length& slice_length = slices[side].getLength();
			if(slice_length.isNumber()) {
				// is number in pixels
				slice_[side] = static_cast<float>(slice_length.compute()) / fixed_point_scale_float;
			} else if(slice_length.isPercent()) {
				// is percentage, referring to size of border image
				FixedPoint image_wh = ((side & 1) ? image_->surfaceWidth() : image_->surfaceHeight()) * fixed_point_scale;
				slice_[side] = static_cast<float>(slice_length.compute(image_wh)) / fixed_point_scale_float;
				// need to cap at 100%
			} else {
				ASSERT_LOG(false, "Received a length that wasn't a number or percent for slice value: ");
			}
			ASSERT_LOG(slice_[side] >= 0, "Negative values for slices are illegal");
		}

		auto& widths = ctx.getComputedValue(Property::BORDER_IMAGE_WIDTH).getValue<WidthList>().getWidths();
		for(auto side = 0; side != 4; ++side) {
			if(widths[side].isAuto()) {
				// intrinsic width of corrsponding slice.
				widths_[side] = slice_[side];
			} else if(widths[side].getLength().isNumber()) {
				// is multiple of border width
				widths_[side] = static_cast<float>(widths[side].getLength().compute()) / fixed_point_scale_float 
					* static_cast<float>(border[side]) / fixed_point_scale_float;
			} else if(widths[side].getLength().isPercent()) {
				// is percentage
				const FixedPoint bia = (side & 1) ? border_image_width : border_image_height;
				widths_[side] = static_cast<float>(widths[side].getLength().compute(bia)) / fixed_point_scale_float;
			} else {
				// is dimensioned value
				widths_[side] = static_cast<float>(widths[side].getLength().compute()) / fixed_point_scale_float;
			}
			ASSERT_LOG(widths_[side] >= 0, "Negative values for width are illegal");
		}

		// Proportionally reduce width values if there are pairs that would overlap.
		const float l_width = static_cast<float>(border_image_width) / fixed_point_scale_float;
		const float l_height = static_cast<float>(border_image_height) / fixed_point_scale_float;
		const float f = std::min(l_width / (widths_[1] + widths_[3]), l_height / (widths_[0] + widths_[2]));
		if(f < 1.0f) {
			for(int side = 0; side != 4; ++side) {
				widths_[side] *= f;
			}
		}

		auto repeat = ctx.getComputedValue(Property::BORDER_IMAGE_REPEAT).getValue<BorderImageRepeat>();
		setRepeat(repeat.image_repeat_horiz_, repeat.image_repeat_vert_);

		setFill(border_slice.isFilled());
	}

	bool BorderInfo::render(DisplayListPtr display_list, const point& offset, const Dimensions& dims) const
	{
		if(image_ == nullptr) {
			// no image, indicate we should try fallback.
			return false;
		}
		bool no_fill = false;

		// Create a renderable object to store co-ordinates we will use.
		auto ptr = std::make_shared<TextureRenderable>(image_);
		std::vector<KRE::vertex_texcoord> coords;

		// These are the outside edges.
		const float y1 = static_cast<float>(offset.y - dims.padding_.top - dims.border_.top) / fixed_point_scale_float - outset_[0];
		const float x1 = static_cast<float>(offset.x - dims.padding_.left - dims.border_.left) / fixed_point_scale_float - outset_[1];
		const float y2 = static_cast<float>(offset.y + dims.content_.height + dims.padding_.bottom + dims.border_.bottom) / fixed_point_scale_float + outset_[2];
		const float x2 = static_cast<float>(offset.x + dims.content_.width + dims.padding_.right + dims.border_.right) / fixed_point_scale_float + outset_[3];

		auto uw1 = image_->getTextureCoordW(0, slice_[1]);
		auto vw1 = image_->getTextureCoordH(0, slice_[0]);
		auto uw2 = image_->getTextureCoordW(0, slice_[3]);
		auto vw2 = image_->getTextureCoordH(0, slice_[2]);

		// top-left corner
		coords.emplace_back(glm::vec2(x1, y1), glm::vec2(0.0f, 0.0f));
		coords.emplace_back(glm::vec2(x1, y1+widths_[0]), glm::vec2(0.0f, vw1));
		coords.emplace_back(glm::vec2(x1+widths_[1], y1+widths_[0]), glm::vec2(uw1, vw1));
		coords.emplace_back(glm::vec2(x1+widths_[1], y1+widths_[0]), glm::vec2(uw1, vw1));
		coords.emplace_back(glm::vec2(x1, y1), glm::vec2(0.0f, 0.0f));
		coords.emplace_back(glm::vec2(x1+widths_[1], y1), glm::vec2(uw1, 0.0f));

		// top-right corner
		coords.emplace_back(glm::vec2(x2-widths_[3], y1), glm::vec2(1.0f-uw2, 0.0f));
		coords.emplace_back(glm::vec2(x2-widths_[3], y1+widths_[0]), glm::vec2(1.0f-uw2, vw1));
		coords.emplace_back(glm::vec2(x2, y1+widths_[0]), glm::vec2(1.0f, vw1));
		coords.emplace_back(glm::vec2(x2, y1+widths_[0]), glm::vec2(1.0f, vw1));
		coords.emplace_back(glm::vec2(x2-widths_[3], y1), glm::vec2(1.0f-uw2, 0.0f));
		coords.emplace_back(glm::vec2(x2, y1), glm::vec2(1.0f, 0.0f));

		// bottom-left corner
		coords.emplace_back(glm::vec2(x1, y2-widths_[2]), glm::vec2(0.0f, 1.0f-vw2));
		coords.emplace_back(glm::vec2(x1, y2), glm::vec2(0.0f, 1.0f));
		coords.emplace_back(glm::vec2(x1+widths_[1], y2), glm::vec2(uw1, 1.0f));
		coords.emplace_back(glm::vec2(x1+widths_[1], y2), glm::vec2(uw1, 1.0f));
		coords.emplace_back(glm::vec2(x1, y2-widths_[2]), glm::vec2(0.0f, 1.0f-vw2));
		coords.emplace_back(glm::vec2(x1+widths_[1], y2-widths_[2]), glm::vec2(uw1, 1.0f-vw2));

		// bottom-right corner
		coords.emplace_back(glm::vec2(x2-widths_[3], y2-widths_[2]), glm::vec2(1.0f-uw2, 1.0f-vw2));
		coords.emplace_back(glm::vec2(x2-widths_[3], y2), glm::vec2(1.0f-uw2, 1.0f));
		coords.emplace_back(glm::vec2(x2, y2), glm::vec2(1.0f, 1.0f));
		coords.emplace_back(glm::vec2(x2, y2), glm::vec2(1.0f, 1.0f));
		coords.emplace_back(glm::vec2(x2-widths_[3], y2-widths_[2]), glm::vec2(1.0f-uw2, 1.0f-vw2));
		coords.emplace_back(glm::vec2(x2, y2-widths_[2]), glm::vec2(1.0f, 1.0f-vw2));

		const float side_height = widths_[2] - widths_[0];
		const float side_width = widths_[3] - widths_[1];

		const float rounded_tiles_h = std::round(side_height / (slice_[2] - slice_[0]));
		const float rounded_tiles_w = std::round(side_width  / (slice_[3] - slice_[1]));

		// left and right sides being shown are contingent on the top/bottom slices
		// being less than the height of the image
		if(slice_[0] + slice_[2] < image_->surfaceHeight()) {
			const float l_u1 = 0.0f;
			const float l_v1 = vw1;
			const float l_u2 = uw2;
			const float l_v2 = 1.0f - vw2;

			const float r_u1 = 1.0f - uw2;
			const float r_v1 = vw1;
			const float r_u2 = 1.0f;
			const float r_v2 = 1.0f - vw2;
			switch(repeat_vert_) {
				case CssBorderImageRepeat::STRETCH: 
					// left-side
					coords.emplace_back(glm::vec2(x1, y1+widths_[0]), glm::vec2(l_u1, l_v1));
					coords.emplace_back(glm::vec2(x1, y2-widths_[2]), glm::vec2(l_u1, l_v2));
					coords.emplace_back(glm::vec2(x1+widths_[1], y2-widths_[2]), glm::vec2(l_u2, l_v2));
					coords.emplace_back(glm::vec2(x1+widths_[1], y2-widths_[2]), glm::vec2(l_u2, l_v2));
					coords.emplace_back(glm::vec2(x1, y1+widths_[0]), glm::vec2(l_u1, l_v1));
					coords.emplace_back(glm::vec2(x1+widths_[1], y1+widths_[0]), glm::vec2(l_u2, l_v1));

					// right-side
					coords.emplace_back(glm::vec2(x2-widths_[3], y1+widths_[0]), glm::vec2(r_u1, r_v1));
					coords.emplace_back(glm::vec2(x2-widths_[3], y2-widths_[2]), glm::vec2(r_u1, r_v2));
					coords.emplace_back(glm::vec2(x2, y2-widths_[2]), glm::vec2(r_u2, r_v2));
					coords.emplace_back(glm::vec2(x2, y2-widths_[2]), glm::vec2(r_u2, r_v2));
					coords.emplace_back(glm::vec2(x2-widths_[3], y1+widths_[0]), glm::vec2(r_u1, r_v1));
					coords.emplace_back(glm::vec2(x2, y1+widths_[0]), glm::vec2(r_u2, r_v1));
					break;
				case CssBorderImageRepeat::REPEAT:
					// XXX
					break;
				case CssBorderImageRepeat::ROUND:
					// XXX
					break;
				case CssBorderImageRepeat::SPACE:
					// ugly special case
					break;
			}
		} else {
			no_fill = true;
		}

		// top and bottom sides being shown are contingent on the left/right slices
		// being less than the width of the image
		if(slice_[1] + slice_[3] < image_->surfaceWidth()) {
			const float t_u1 = uw1;
			const float t_v1 = 0.0f;
			const float t_u2 = 1.0f - uw2;
			const float t_v2 = vw1;

			const float b_u1 = uw1;
			const float b_v1 = 1.0f - vw2;
			const float b_u2 = 1.0f - uw2;
			const float b_v2 = 1.0f;

			switch(repeat_horiz_) {
				case CssBorderImageRepeat::STRETCH:
					// top-side
					coords.emplace_back(glm::vec2(x1+widths_[1], y1), glm::vec2(t_u1, t_v1));
					coords.emplace_back(glm::vec2(x1+widths_[1], y1+widths_[0]), glm::vec2(t_u1, t_v2));
					coords.emplace_back(glm::vec2(x2-widths_[3], y1+widths_[0]), glm::vec2(t_u2, t_v2));
					coords.emplace_back(glm::vec2(x2-widths_[3], y1+widths_[0]), glm::vec2(t_u2, t_v2));
					coords.emplace_back(glm::vec2(x1+widths_[1], y1), glm::vec2(t_u1, t_v1));
					coords.emplace_back(glm::vec2(x2-widths_[3], y1), glm::vec2(t_u2, t_v1));

					// bottom-side
					coords.emplace_back(glm::vec2(x1+widths_[1], y2-widths_[2]), glm::vec2(b_u1, b_v1));
					coords.emplace_back(glm::vec2(x1+widths_[1], y2), glm::vec2(b_u1, b_v2));
					coords.emplace_back(glm::vec2(x2-widths_[3], y2), glm::vec2(b_u2, b_v2));
					coords.emplace_back(glm::vec2(x2-widths_[3], y2), glm::vec2(b_u2, b_v2));
					coords.emplace_back(glm::vec2(x1+widths_[1], y2-widths_[2]), glm::vec2(b_u1, b_v1));
					coords.emplace_back(glm::vec2(x2-widths_[3], y2-widths_[2]), glm::vec2(b_u2, b_v1));
					break;
				case CssBorderImageRepeat::REPEAT:
					// XXX
					break;
				case CssBorderImageRepeat::ROUND:
					// XXX
					break;
				case CssBorderImageRepeat::SPACE:
					// ugly special case
					break;
			}
		} else {
			no_fill = true;
		}

		// fill
		if(fill_ && !no_fill) {
		}

		// pass co-ordinates to renderable object and add to display list ready for rendering.
		ptr->update(&coords);
		display_list->addRenderable(ptr);
		// returning true indicates we handled drawing the border
		return true;
	}
}
