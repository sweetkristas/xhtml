/*
	Copyright (C) 2015-2016 by Kristina Simpson <sweet.kristas@gmail.com>
	
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

#include "xhtml_layout_engine.hpp"
#include "xhtml_line_box.hpp"

namespace xhtml
{
	LineBox::LineBox(const BoxPtr& parent, const StyleNodePtr& node, const RootBoxPtr& root)
		: Box(BoxId::LINE, parent, node, root)
	{
	}

	std::string LineBox::toString() const 
	{
		std::ostringstream ss;
		ss << "LineBox: " << getDimensions().content_;
		return ss.str();
	}

	void LineBox::handleLayout(LayoutEngine& eng, const Dimensions& containing)
	{
		calculateHorzMPB(containing.content_.width);
		calculateVertMPB(containing.content_.height);

        int child_height = 0;
        FixedPoint width = 0;
        for(auto& child : getChildren()) {
			if(!child->isFloat()) {
				//child_height = std::max(child_height, child->getHeight() + child->getTop() + child->getMBPBottom());
				child_height += child->getTop() + child->getHeight() + child->getMBPBottom();
				width = std::max(width, child->getLeft() + child->getWidth() + child->getMBPWidth());
			}
		}
		setContentHeight(child_height);
		setContentWidth(width);
	}

	void LineBox::postParentLayout(LayoutEngine& eng, const Dimensions& containing)
	{
	}

	void LineBox::handleRender(const KRE::SceneTreePtr& scene_tree, const point& offset) const 
	{
	}

	void LineBox::handleRenderBackground(const KRE::SceneTreePtr& scene_tree, const point& offset) const 
	{
	}

	void LineBox::handleRenderBorder(const KRE::SceneTreePtr& scene_tree, const point& offset) const 
	{
	}

	std::vector<LineBoxPtr> LineBox::reflowText(LineBoxParseInfo* pi, LayoutEngine& eng, const Dimensions& containing)
	{
		// TextBox's have no children to deal with, by definition.	
		// XXX fix the point() to be the actual last point, say from LayoutEngine
		//point cursor = TextBox::reflowText(eng, containing, eng.getCursor());
		//eng.setCursor(cursor);

		std::vector<LineBoxPtr> line_boxes;

		//bool done = false;
		//while(!done) {
			auto line_box = std::make_shared<LineBox>(pi->parent_, pi->node_, pi->root_);
			auto tboxes = TextBox::reflowText(pi, eng, line_box, containing);
			for(auto& tbox : tboxes) {
				line_box->addChild(tbox);
			}

			//if(tboxes.empty()) {
			//	done = true;
			//}

			line_boxes.emplace_back(line_box);
		//}
		return line_boxes;
	}



	LineBoxContainer::LineBoxContainer(const BoxPtr& parent, const StyleNodePtr& node, const RootBoxPtr& root)
		: Box(BoxId::LINE_CONTAINER, parent, node, root),
		  txt_(std::dynamic_pointer_cast<Text>(node->getNode()))
	{
		txt_->transformText(node, true);
	}

	std::string LineBoxContainer::toString() const
	{
		std::ostringstream ss;
		ss << "LineBoxContainer: " << getDimensions().content_;
		return ss.str();
	}

	void LineBoxContainer::handlePreChildLayout(LayoutEngine& eng, const Dimensions& containing)
	{
		LineBoxParseInfo pi(getParent(), getStyleNode(), getRoot(), txt_);
		std::vector<LineBoxPtr> line_boxes = LineBox::reflowText(&pi, eng, containing);

		for(const auto& line_box : line_boxes) {
			addChild(line_box);
		}
	}

	void LineBoxContainer::handleLayout(LayoutEngine& eng, const Dimensions& containing)
	{
		calculateHorzMPB(containing.content_.width);
		calculateVertMPB(containing.content_.height);

        int child_height = 0;
        FixedPoint width = 0;
        for(auto& child : getChildren()) {
			if(!child->isFloat()) {
				child_height += child->getHeight() + child->getTop() + child->getMBPBottom();
				width = std::max(width, child->getLeft() + child->getWidth() + child->getMBPWidth());
			}
		}
		setContentHeight(child_height);
		setContentWidth(width);
	}

	void LineBoxContainer::postParentLayout(LayoutEngine& eng, const Dimensions& containing)
	{
	}

	void LineBoxContainer::handleRender(const KRE::SceneTreePtr& scene_tree, const point& offset) const
	{
	}

	void LineBoxContainer::handleRenderBackground(const KRE::SceneTreePtr& scene_tree, const point& offset) const
	{
	}

	void LineBoxContainer::handleRenderBorder(const KRE::SceneTreePtr& scene_tree, const point& offset) const
	{
	}

}
