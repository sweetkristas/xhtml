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

#pragma once

#include "xhtml_box.hpp"
#include "xhtml_text_box.hpp"

namespace xhtml
{
	struct LineBoxParseInfo
	{
		explicit LineBoxParseInfo(const BoxPtr& parent, const StyleNodePtr& node, const RootBoxPtr& root, const TextPtr& txt) 
			: parent_(parent), 
			  node_(node), 
			  root_(root),
			  txt_(txt)
		{
		}
		const BoxPtr& parent_;
		const StyleNodePtr& node_;
		const RootBoxPtr& root_;
		const TextPtr& txt_;
	};

	// This class acts as a container for LineBox's and TextBox's so that we can generate them during layout, but
	// allocate during the LayoutEngine pass.
	class LineBoxContainer : public Box
	{
	public:
		LineBoxContainer(const BoxPtr& parent, const StyleNodePtr& node, const RootBoxPtr& root);
		std::string toString() const override;
	private:
		void handlePreChildLayout(LayoutEngine& eng, const Dimensions& containing) override;
		void handleLayout(LayoutEngine& eng, const Dimensions& containing) override;
		void postParentLayout(LayoutEngine& eng, const Dimensions& containing) override;
		void handleRender(const KRE::SceneTreePtr& scene_tree, const point& offset) const override;
		void handleRenderBackground(const KRE::SceneTreePtr& scene_tree, const point& offset) const override;
		void handleRenderBorder(const KRE::SceneTreePtr& scene_tree, const point& offset) const override;
		
		TextPtr txt_;
	};

	class LineBox : public Box
	{
	public:
		LineBox(const BoxPtr& parent, const StyleNodePtr& node, const RootBoxPtr& root);
		std::string toString() const override;
		static std::vector<LineBoxPtr> reflowText(LineBoxParseInfo* pi, LayoutEngine& eng, const Dimensions& containing);
	private:
		void handleLayout(LayoutEngine& eng, const Dimensions& containing) override;
		void postParentLayout(LayoutEngine& eng, const Dimensions& containing) override;
		void handleRender(const KRE::SceneTreePtr& scene_tree, const point& offset) const override;
		void handleRenderBackground(const KRE::SceneTreePtr& scene_tree, const point& offset) const override;
		void handleRenderBorder(const KRE::SceneTreePtr& scene_tree, const point& offset) const override;
	};
}
