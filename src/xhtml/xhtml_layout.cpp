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
#include "xhtml_render_ctx.hpp"

#include "AttributeSet.hpp"
#include "DisplayDevice.hpp"
#include "SceneObject.hpp"
#include "Shaders.hpp"

namespace xhtml
{
	using namespace css;

	namespace 
	{
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
			SolidRenderable(const rectf& r, const KRE::Color& color)
				: KRE::SceneObject("SolidRenderable")
			{
				init();
				std::vector<KRE::vertex_color> vc;
				vc.emplace_back(glm::vec2(r.x1(), r.y2()), color.as_u8vec4());
				vc.emplace_back(glm::vec2(r.x1(), r.y1()), color.as_u8vec4());
				vc.emplace_back(glm::vec2(r.x2(), r.y1()), color.as_u8vec4());

				vc.emplace_back(glm::vec2(r.x2(), r.y1()), color.as_u8vec4());
				vc.emplace_back(glm::vec2(r.x2(), r.y2()), color.as_u8vec4());
				vc.emplace_back(glm::vec2(r.x1(), r.y2()), color.as_u8vec4());
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
	}

	LayoutBox::LayoutBox(LayoutBoxPtr parent, NodePtr node)
		: node_(node),
		  children_(),
		  dimensions_()
	{
	}

	LayoutBoxPtr LayoutBox::factory(NodePtr node, CssDisplay display, LayoutBoxPtr parent)
	{
		if(node == nullptr) {
			// return anonymous box if no attached node.
			return std::make_shared<AnonymousLayoutBox>(parent);
		}
		switch(display) {
			case CssDisplay::NONE:
				// Do not create a box for this or it's children
				return nullptr;
			case CssDisplay::INLINE:
				return std::make_shared<InlineLayoutBox>(parent, node);
			case CssDisplay::BLOCK:
				return std::make_shared<BlockLayoutBox>(parent, node);
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

	LayoutBoxPtr LayoutBox::create(NodePtr node, int containing_width)
	{
		auto lb = handleCreate(node, nullptr);
		Dimensions root;
		root.content_ = geometry::Rect<double>(0, 0, containing_width, 0);
		lb->layout(root, inline_offset());
		return lb;
	}

	LayoutBoxPtr LayoutBox::handleCreate(NodePtr node, LayoutBoxPtr parent)
	{
		std::unique_ptr<RenderContext::Manager> ctx_manager;
		if(node->id() == NodeId::ELEMENT) {
			ctx_manager.reset(new RenderContext::Manager(node->getProperties()));
		}
				// text nodes are always inline
		CssDisplay display = node->id() == NodeId::TEXT 
			? display = CssDisplay::INLINE
			: RenderContext::get().getComputedValue(Property::DISPLAY).getValue<CssDisplay>();
		if(display != CssDisplay::NONE) {
			auto root = factory(node, display, parent);

			AnonymousLayoutBoxPtr inline_container;
			for(auto& c : node->getChildren()) {
				std::unique_ptr<RenderContext::Manager> child_ctx_manager;
				if(c->id() == NodeId::ELEMENT) {
					child_ctx_manager.reset(new RenderContext::Manager(c->getProperties()));
				}
				// text nodes are always inline
				CssDisplay child_display = c->id() == NodeId::TEXT 
					? CssDisplay::INLINE 
					: RenderContext::get().getComputedValue(Property::DISPLAY).getValue<CssDisplay>();
				if(child_display == CssDisplay::NONE) {
					// ignore child nodes with display none.
				} else if(child_display == CssDisplay::INLINE && display == CssDisplay::BLOCK) {
					if(inline_container == nullptr) {
						inline_container = std::make_shared<AnonymousLayoutBox>(root);
						root->children_.emplace_back(inline_container);
					}
					inline_container->children_.emplace_back(handleCreate(c, root));
				} else {
					inline_container.reset();
					root->children_.emplace_back(handleCreate(c, root));
				}
			}

			return root;
		}
		return nullptr;
	}

	void LayoutBox::insertChildren(const_child_iterator pos, const std::vector<LayoutBoxPtr>& children)
	{
		children_.insert(pos, children.begin(), children.end());
	}

	void LayoutBox::setBorder(Side n, double value)
	{
		switch(n) {
			case Side::TOP:		dimensions_.border_.top = value; break;
			case Side::LEFT:	dimensions_.border_.left = value; break;
			case Side::BOTTOM:	dimensions_.border_.bottom = value; break;
			case Side::RIGHT:	dimensions_.border_.right = value; break;
			default:
				ASSERT_LOG(false, "Unrecognised index for setBorder(): " << static_cast<int>(n));
		}
	}

	void LayoutBox::setPadding(Side n, double value)
	{
		switch(n) {
			case Side::TOP:		dimensions_.padding_.top = value; break;
			case Side::LEFT:	dimensions_.padding_.left = value; break;
			case Side::BOTTOM:	dimensions_.padding_.bottom = value; break;
			case Side::RIGHT:	dimensions_.padding_.right = value; break;
			default:
				ASSERT_LOG(false, "Unrecognised index for setPadding(): " << static_cast<int>(n));
		}
	}

	void LayoutBox::setMargins(Side n, double value)
	{
		switch(n) {
			case Side::TOP:		dimensions_.margin_.top = value; break;
			case Side::LEFT:	dimensions_.margin_.left = value; break;
			case Side::BOTTOM:	dimensions_.margin_.bottom = value; break;
			case Side::RIGHT:	dimensions_.margin_.right = value; break;
			default:
				ASSERT_LOG(false, "Unrecognised index for setMargins(): " << static_cast<int>(n));
		}
	}

	void LayoutBox::layout(const Dimensions& containing, inline_offset& offset)
	{
		auto node = node_.lock();
		std::unique_ptr<RenderContext::Manager> ctx_manager;
		if(node != nullptr && node->id() == NodeId::ELEMENT) {
			// only instantiate on element nodes.
			ctx_manager.reset(new RenderContext::Manager(node->getProperties()));
		}
		RenderContext& ctx = RenderContext::get();
		border_style_[0] = ctx.getComputedValue(Property::BORDER_TOP_STYLE).getValue<CssBorderStyle>();
		border_style_[1] = ctx.getComputedValue(Property::BORDER_LEFT_STYLE).getValue<CssBorderStyle>();
		border_style_[2] = ctx.getComputedValue(Property::BORDER_BOTTOM_STYLE).getValue<CssBorderStyle>();
		border_style_[3] = ctx.getComputedValue(Property::BORDER_RIGHT_STYLE).getValue<CssBorderStyle>();
		border_color_[0] = ctx.getComputedValue(Property::BORDER_TOP_COLOR).getValue<CssColor>().compute();
		border_color_[1] = ctx.getComputedValue(Property::BORDER_LEFT_COLOR).getValue<CssColor>().compute();
		border_color_[2] = ctx.getComputedValue(Property::BORDER_BOTTOM_COLOR).getValue<CssColor>().compute();
		border_color_[3] = ctx.getComputedValue(Property::BORDER_RIGHT_COLOR).getValue<CssColor>().compute();

		background_color_ = ctx.getComputedValue(Property::BACKGROUND_COLOR).getValue<CssColor>().compute();

		handleLayout(containing, offset);

		LOG_DEBUG(toString() << " content: " << dimensions_.content_);
	}

	void LayoutBox::render(DisplayListPtr display_list) const
	{
		renderBackground(display_list);
		renderBorder(display_list);
		handleRender(display_list);
		for(auto& child : children_) {
			child->render(display_list);
		}
	}

	void LayoutBox::renderBackground(DisplayListPtr display_list) const
	{
		if(background_color_.ai() != 0) {
			rectf r(dimensions_.content_.x() - dimensions_.padding_.left,
				dimensions_.content_.y() - dimensions_.padding_.top,
				dimensions_.content_.w() + dimensions_.padding_.left + dimensions_.padding_.right,
				dimensions_.content_.h() + dimensions_.padding_.top + dimensions_.padding_.bottom);
			display_list->addRenderable(std::make_shared<SolidRenderable>(r, background_color_));
		}
	}

	void LayoutBox::renderBorder(DisplayListPtr display_list) const
	{
		// XXX
		
	}

	void BlockLayoutBox::handleLayout(const Dimensions& containing, inline_offset& offset)
	{
		layoutBlockWidth(containing);
		layoutBlockPosition(containing);
		layoutBlockChildren();
		layoutBlockHeight(containing);
	}

	BlockLayoutBox::BlockLayoutBox(LayoutBoxPtr parent, NodePtr node)
		: LayoutBox(parent, node)
	{
	}

	void BlockLayoutBox::layoutBlockWidth(const Dimensions& containing)
	{
		RenderContext& ctx = RenderContext::get();
		double containing_width = containing.content_.w();

		auto css_width = ctx.getComputedValue(Property::WIDTH).getValue<Width>();
		double width = css_width.evaluate(ctx).getValue<Length>().compute(containing_width);

		getDims().border_.left = ctx.getComputedValue(Property::BORDER_LEFT_WIDTH).getValue<Length>().compute();
		getDims().border_.right = ctx.getComputedValue(Property::BORDER_RIGHT_WIDTH).getValue<Length>().compute();

		getDims().padding_.left = ctx.getComputedValue(Property::PADDING_LEFT).getValue<Length>().compute(containing_width);
		getDims().padding_.right = ctx.getComputedValue(Property::PADDING_RIGHT).getValue<Length>().compute(containing_width);

		auto css_margin_left = ctx.getComputedValue(Property::MARGIN_LEFT).getValue<Width>();
		auto css_margin_right = ctx.getComputedValue(Property::MARGIN_RIGHT).getValue<Width>();
		getDims().margin_.left = css_margin_left.evaluate(ctx).getValue<Length>().compute(containing_width);
		getDims().margin_.right = css_margin_right.evaluate(ctx).getValue<Length>().compute(containing_width);

		double total = getDims().border_.left + getDims().border_.right
			+ getDims().padding_.left + getDims().padding_.right
			+ getDims().margin_.left + getDims().margin_.right
			+ width;
			
		if(!css_width.isAuto() && total > containing.content_.w()) {
			if(css_margin_left.isAuto()) {
				getDims().margin_.left = 0;
			}
			if(css_margin_right.isAuto()) {
				getDims().margin_.right = 0;
			}
		}

		// If negative is overflow.
		double underflow = containing.content_.w() - total;

		if(css_width.isAuto()) {
			if(css_margin_left.isAuto()) {
				getDims().margin_.left = 0;
			}
			if(css_margin_right.isAuto()) {
				getDims().margin_.right = 0;
			}
			if(underflow >= 0) {
				width = underflow;
			} else {
				width = 0;
				getDims().margin_.right = css_margin_right.evaluate(ctx).getValue<Length>().compute(containing_width) + underflow;
			}
			getDims().content_.set_w(width);
		} else if(!css_margin_left.isAuto() && !css_margin_right.isAuto()) {
			getDims().margin_.right = getDims().margin_.right + underflow;
		} else if(!css_margin_left.isAuto() && css_margin_right.isAuto()) {
			getDims().margin_.right = underflow;
		} else if(css_margin_left.isAuto() && !css_margin_right.isAuto()) {
			getDims().margin_.left = underflow;
		} else if(css_margin_left.isAuto() && css_margin_right.isAuto()) {
			getDims().margin_.left = underflow/2.0;
			getDims().margin_.right = underflow/2.0;
		} 
	}

	void BlockLayoutBox::layoutBlockPosition(const Dimensions& containing)
	{
		RenderContext& ctx = RenderContext::get();
		double containing_height = containing.content_.h();
			
		getDims().border_.top = ctx.getComputedValue(Property::BORDER_TOP_WIDTH).getValue<Length>().compute();
		getDims().border_.bottom = ctx.getComputedValue(Property::BORDER_BOTTOM_WIDTH).getValue<Length>().compute();

		getDims().padding_.top = ctx.getComputedValue(Property::PADDING_TOP).getValue<Length>().compute(containing_height);
		getDims().padding_.bottom = ctx.getComputedValue(Property::PADDING_BOTTOM).getValue<Length>().compute(containing_height);

		getDims().margin_.top = ctx.getComputedValue(Property::MARGIN_TOP).getValue<Width>().evaluate(ctx).getValue<Length>().compute(containing_height);
		getDims().margin_.bottom = ctx.getComputedValue(Property::MARGIN_BOTTOM).getValue<Width>().evaluate(ctx).getValue<Length>().compute(containing_height);

		getDims().content_.set_x(containing.content_.x() + getDims().margin_.left + getDims().padding_.left + getDims().border_.left);
		getDims().content_.set_y(containing.content_.y2() + getDims().margin_.top + getDims().padding_.top + getDims().border_.top);
	}

	void BlockLayoutBox::layoutBlockChildren()
	{
		auto& ctx = RenderContext::get();
		long font_coord_factor = ctx.getFontHandle()->getScaleFactor();

		inline_offset offs;
		for(auto& child : getChildren()) {
			offs.offset.x = 0;
			offs.offset.y = static_cast<long>(getDims().content_.h() * font_coord_factor);
			child->layout(getDims(), offs);
			getDims().content_.set_h(getDims().content_.h()
				+ child->getDimensions().content_.h() 
				+ child->getDimensions().margin_.top + child->getDimensions().margin_.bottom
				+ child->getDimensions().padding_.top + child->getDimensions().padding_.bottom
				+ child->getDimensions().border_.top + child->getDimensions().border_.bottom);
		}
	}

	void BlockLayoutBox::layoutBlockHeight(const Dimensions& containing)
	{
		RenderContext& ctx = RenderContext::get();
		// a set height value overrides the calculated value.
		auto css_h = ctx.getComputedValue(Property::HEIGHT).getValue<Width>();
		if(!css_h.isAuto()) {
			getDims().content_.set_h(css_h.evaluate(ctx).getValue<Length>().compute(containing.content_.h()));
		}
	}

	AnonymousLayoutBox::AnonymousLayoutBox(LayoutBoxPtr parent)
		: LayoutBox(parent, nullptr)
	{
	}

	void AnonymousLayoutBox::handleLayout(const Dimensions& containing, inline_offset& offset)
	{
		for(auto& child : getChildren()) {
			child->layout(containing, offset);
			getDims().content_.set_w(std::max(getDims().content_.w(), child->getDimensions().content_.w()));			
			getDims().content_.set_h(std::max(getDims().content_.h(), child->getDimensions().content_.h()));
		}
	}

	void InlineLayoutBox::handleLayout(const Dimensions& containing, inline_offset& offset)
	{
		auto& ctx = RenderContext::get();
		long font_coord_factor = ctx.getFontHandle()->getScaleFactor();

		auto children = layoutInlineWidth(containing, offset);
		if(!children.empty()) {
			insertChildren(getChildren().begin(), children);
		}
		double line_h = 0;
		double line_w = 0;
		double max_w = 0;
		setContentX(static_cast<double>(offset.offset.x) / font_coord_factor); 
		setContentY(containing.content_.y());
		for(auto& child : getChildren()) {
			child->layout(containing, offset);
			double child_w = child->getDimensions().content_.w()
				+ child->getDimensions().padding_.left + child->getDimensions().padding_.right
				+ child->getDimensions().border_.left + child->getDimensions().border_.right;
			double w = getDims().content_.w();
				
			if(line_w + child_w > containing.content_.w()) {
				getDims().content_.set_h(getDims().content_.h() + line_h);
				child->setContentY(getDims().content_.y2());
				offset.offset.x = static_cast<long>(child_w * font_coord_factor);
				offset.offset.y += static_cast<long>(line_h * font_coord_factor);

				line_w = child_w;
				line_h = 0;
				max_w = containing.content_.w();
			} else {
				line_w += child_w;
				max_w = std::max(max_w, line_w);
				offset.offset.x += static_cast<long>(child_w * font_coord_factor);
				child->setContentY(containing.content_.y());
			}
			line_h = std::max(line_h, child->getDimensions().content_.h());
		}
		getDims().content_.set_w(max_w);
	}

	InlineLayoutBox::InlineLayoutBox(LayoutBoxPtr parent, NodePtr node)
		: LayoutBox(parent, node)
	{
	}

	std::vector<LayoutBoxPtr> InlineLayoutBox::layoutInlineWidth(const Dimensions& containing, inline_offset& offset)
	{
		std::vector<LayoutBoxPtr> new_children;
		KRE::FontRenderablePtr fontr = nullptr;
		auto node = getNode();
		if(node != nullptr && node->id() == NodeId::TEXT) {
			auto& ctx = RenderContext::get();
			long font_coord_factor = ctx.getFontHandle()->getScaleFactor();

			auto left_border = ctx.getComputedValue(Property::BORDER_LEFT_WIDTH).getValue<Length>().compute();
			auto right_border = ctx.getComputedValue(Property::BORDER_RIGHT_WIDTH).getValue<Length>().compute();
			auto top_border = ctx.getComputedValue(Property::BORDER_TOP_WIDTH).getValue<Length>().compute();
			auto bottom_border = ctx.getComputedValue(Property::BORDER_BOTTOM_WIDTH).getValue<Length>().compute();

			auto left_padding = ctx.getComputedValue(Property::PADDING_LEFT).getValue<Length>().compute(containing.content_.w());
			auto right_padding = ctx.getComputedValue(Property::PADDING_RIGHT).getValue<Length>().compute(containing.content_.w());
			auto top_padding = ctx.getComputedValue(Property::PADDING_TOP).getValue<Length>().compute();
			auto bottom_padding = ctx.getComputedValue(Property::PADDING_BOTTOM).getValue<Length>().compute();

			if((getDims().border_.left + getDims().padding_.left) + offset.offset.x/font_coord_factor > containing.content_.w()) {
				offset.offset.x = static_cast<int>(getDims().border_.left + getDims().padding_.left * font_coord_factor);
				offset.offset.y += static_cast<int>(offset.line_height * font_coord_factor);
			}

			auto lines = node->generateLines(offset.offset.x/font_coord_factor, static_cast<int>(containing.content_.w()));

			auto left_border_style = ctx.getComputedValue(Property::BORDER_LEFT_STYLE).getValue<CssBorderStyle>();
			//auto top_border_style = ctx.getComputedValue(Property::BORDER_TOP_STYLE).getValue<CssBorderStyle>();
			auto right_border_style = ctx.getComputedValue(Property::BORDER_RIGHT_STYLE).getValue<CssBorderStyle>();
			//auto bottom_border_style = ctx.getComputedValue(Property::BORDER_BOTTOM_STYLE).getValue<CssBorderStyle>();

			// XXX we need to store in RenderContext a variable specifying the max-line-height of the last line
			// to account for the case that we have mixed fonts on one line.
			// XXX also need to handle the case that we have an element with a different line height on the same line
			// oh, I suppose that is the same case I just mentioned. mea-culpa.
			// XXX I think this still needs to be passed up to a higher layer and concatenate more text flows together
			// which would allow better handling of justified text when you have elements or breaks on a line near the
			// end of a run. 
			// e.g. <span>Some text to render <em>NeedThisRenderedToday!</em> more text.</span>
			// If the screen was like this |----------------------------|
			// then the text might render  |Some text to render         |
			//                             |NeedThisRenderedToday! more |
			//                             |text.                       |
			//                             |----------------------------|
			// Where it breaks the em element on to another line, but doesn't justify the first line correctly to account 
			// for it. Yes it's a contrived example, but I saw it happen in practice. The other example is if it did
			// |----------------------------|
			// | This is a msg. me this  one| 
			// |----------------------------|
			// i.e. <span>This is a msg.<em>me</em> This one</span>
			// What happens is that the justify algoritm only applies to the text after the em tag finishes,
			// which can look kind of silly if there are only a few words being justified, when the entire 
			// rest of the line could be used.

			// computer the content width based on the generated lines.

			//int min_x = offset.offset.x/font_coord_factor;
			//int max_x = -1;

			// since the Renderable returned from createRenderableFromPath is relative to the font baseline
			// we offset by the line-height to start.
			/*const auto lh = ctx.getComputedValue(Property::LINE_HEIGHT).getValue<Length>();
			double line_height = lh.compute();
			if(lh.isPercent() || lh.isNumber()) {
				line_height *= ctx.getComputedValue(Property::FONT_SIZE).getValue<double>();
			}
			lines->line_height = line_height;
			line_height = std::max(offset.line_height, line_height);
			LOG_DEBUG("line-height: " << line_height);
			if(offset.offset.y == 0) {
				offset.offset.y = static_cast<long>(line_height * font_coord_factor);
			}*/
			// XXX Break this into children added to new_children and set there
			// border_style_/border_color_/dimensions_.margin properties appropriatly
			// Remembering to clear them from this layout node.
			const auto last_line = lines->lines.end()-1;
			for(auto line_it = lines->lines.begin(); line_it != lines->lines.end(); ++line_it) {
				new_children.emplace_back(std::make_shared<InlineTextBox>(shared_from_this(), *line_it, lines->space_advance));

				/*for(auto& word : line) {
					for(auto it = word.advance.begin(); it != word.advance.end()-1; ++it) {
						path.emplace_back(it->x + offset.offset.x, it->y + offset.offset.y);
						max_x = std::max(max_x, static_cast<int>((it->x + offset.offset.x)/font_coord_factor));
					}
					offset.offset.x += word.advance.back().x + lines->space_advance;
					text += word.word;
				}*/
				// We exclude the last line from generating a newline.
				/*if(line_it != last_line) {
					offset.offset.y += static_cast<long>(line_height * font_coord_factor);
					offset.offset.x = 0;
					min_x = std::min(min_x, 0);
					max_x = std::max(max_x, static_cast<int>(containing.content_.w()));
					line_height = lines->line_height;
				}*/
			}

			if(!new_children.empty()) {
				// Apply the left/right side padding/borders.
				new_children.front()->setPadding(Side::LEFT, left_padding);
				new_children.front()->setBorder(Side::LEFT, left_border);

				new_children.back()->setPadding(Side::RIGHT, right_padding);
				new_children.back()->setBorder(Side::RIGHT, right_border);

				// Apply the border styles/colors
				new_children.front()->setBorderStyle(Side::LEFT, left_border_style);
				new_children.front()->setBorderStyle(Side::RIGHT, CssBorderStyle::NONE);
				new_children.back()->setBorderStyle(Side::RIGHT, right_border_style);
				new_children.back()->setBorderStyle(Side::LEFT, CssBorderStyle::NONE);

				for(auto& child : new_children) {
					//child->setBorderStyle(Side::TOP, top_border_style);
					//child->setBorderStyle(Side::BOTTOM, bottom_border_style);
					//child->setBorderColor(Side::TOP, top_border_color);
					//child->setBorderColor(Side::BOTTOM, bottom_border_color);

					// N.B. though we set values for these they are *not* used for the parents 
					// calculations of the height needed to contain the node. They are however
					// used for border thickness and for drawing the background image/color.
					child->setBorder(Side::TOP, top_border);
					child->setBorder(Side::BOTTOM, bottom_border);
					child->setPadding(Side::TOP, top_padding);
					child->setPadding(Side::BOTTOM, bottom_padding);
				}
				// Set all non-terminal children left/right styles to none.
				for(auto it = new_children.begin()+1; it != new_children.end()-1 && it != new_children.end(); ++it) {
					auto& child = *it;
					child->setBorderStyle(Side::LEFT, CssBorderStyle::NONE);
					child->setBorderStyle(Side::RIGHT, CssBorderStyle::NONE);
				}
			}

			// XXX add the right padding/border here to the advance of the last word of the last line

			//if(lines->lines.size() > 0) {
			//	last_line_height = line_height;
			//}
			//getDims().content_.set_h(offset.offset.y);
			//getDims().content_.set_w(max_x - min_x);

			//lines_ = lines;

			// XXX hack for the moment
			//auto fh = ctx.getFontHandle();
			//fontr = fh->createRenderableFromPath(fontr, text, path);
			//fontr->setColor(ctx.getComputedValue(Property::COLOR).getValue<KRE::Color>());
			//get_display_list()->addRenderable(fontr);
		} else if(node != nullptr && node->id() == NodeId::ELEMENT) {
			// Need to call a function to get element dimensions.
		}

		return new_children;
	}

	InlineTextBox::InlineTextBox(LayoutBoxPtr parent, const Line& line, long space_advance)
		: LayoutBox(parent, nullptr),
		  line_(line),
		  space_advance_(space_advance)
	{
	}

	double LayoutBox::getLineHeight() const
	{
		auto& ctx = RenderContext::get();
		const auto lh = ctx.getComputedValue(Property::LINE_HEIGHT).getValue<Length>();
		double line_height = lh.compute();
		if(lh.isPercent() || lh.isNumber()) {
			line_height *= ctx.getComputedValue(Property::FONT_SIZE).getValue<double>();
		}
		return line_height;
	}

	void InlineTextBox::handleLayout(const Dimensions& containing, inline_offset& offset)
	{
		auto& ctx = RenderContext::get();
		double line_height = getLineHeight();
		if(line_.empty()) {
			getDims().content_.set_w(0);
			getDims().content_.set_h(line_height);
			return;
		}
		long font_coord_factor = ctx.getFontHandle()->getScaleFactor();
		
		// margins/border/padding should already be set. We just need to calculate content size.
		long w = offset.offset.x != 0 ? space_advance_ : 0;
		for(auto it = line_.begin(); it != line_.end() && it != line_.end()-1; ++it) {
			auto& word = *it;
			w += word.advance.back().x + space_advance_;
		}
		// don't add a final space.
		w += line_.back().advance.back().x;

		getDims().content_.set_w(static_cast<double>(w) / static_cast<double>(font_coord_factor));
		getDims().content_.set_h(line_height);

		getDims().content_.set_x(containing.content_.x() + getDims().padding_.left + getDims().border_.left);
	}

	void LayoutBox::preOrderTraversal(std::function<void(LayoutBoxPtr, int)> fn, int nesting)
	{
		fn(shared_from_this(), nesting);
		for(auto& child : children_) {
			child->preOrderTraversal(fn, nesting+1);
		}
	}

	std::string BlockLayoutBox::toString() const
	{
		std::stringstream ss;
		ss << "BlockBox(" << getNode()->toString() << ")";
		return ss.str();
	}

	std::string InlineLayoutBox::toString() const
	{
		std::stringstream ss;
		ss << "InlineBox(" << getNode()->toString() << ")";
		return ss.str();
	}

	std::string AnonymousLayoutBox::toString() const
	{
		std::stringstream ss;
		ss << "AnonymousBox()";
		return ss.str();
	}

	std::string LayoutBox::toString() const
	{
		std::stringstream ss;
		auto node = node_.lock();
		//ss << "Box(" << (node ? display_string(display_) : "anonymous") << (node ? ", " + node->toString() : "") << ")";
		return ss.str();
	}

	std::string InlineTextBox::toString() const 
	{
		std::stringstream ss;
		ss << "InlineText(\"";
		for(auto& word : line_) {
			ss << " " << word.word;
		}
		ss << "\")";
		return ss.str();
	}

	void BlockLayoutBox::handleRender(DisplayListPtr display_list) const
	{
		// do nothing
	}

	void InlineLayoutBox::handleRender(DisplayListPtr display_list) const 
	{
		// do nothing
	}

	void InlineTextBox::handleRender(DisplayListPtr display_list) const
	{
		std::vector<geometry::Point<long>> path;
		std::string text;
		auto& ctx = RenderContext::get();
		long font_coord_factor = ctx.getFontHandle()->getScaleFactor();
		int dim_x = static_cast<long>(getDimensions().content_.x() * font_coord_factor);
		int dim_y = static_cast<long>(getDimensions().content_.y() * font_coord_factor);
		for(auto& word : line_) {
			for(auto it = word.advance.begin(); it != word.advance.end()-1; ++it) {
				path.emplace_back(it->x + dim_x, it->y + dim_y);
			}
			dim_x += word.advance.back().x + space_advance_;
			text += word.word;
		}

		auto fh = ctx.getFontHandle();
		auto fontr = fh->createRenderableFromPath(nullptr, text, path);
		fontr->setColor(ctx.getComputedValue(Property::COLOR).getValue<CssColor>().compute());
		display_list->addRenderable(fontr);
	}

	void InlineLayoutBox::renderBackground(DisplayListPtr display_list) const
	{
		// do nothing
	}

	void InlineLayoutBox::renderBorder(DisplayListPtr display_list) const
	{
		// do nothing
	}
}
