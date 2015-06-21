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

#include "geometry.hpp"
#include "Texture.hpp"

#include "css_styles.hpp"

namespace xhtml
{
	class BorderInfo
	{
	public:
		BorderInfo();
		void init(const Dimensions& dims);
		bool render(DisplayListPtr display_list, const point& offset, const Dimensions& dims) const;
		void renderNormal(DisplayListPtr display_list, const point& offset, const Dimensions& dims) const;
		void setFile(const std::string& filename);
		void setRepeat(css::CssBorderImageRepeat horiz, css::CssBorderImageRepeat vert) { repeat_horiz_ = horiz; repeat_vert_ = vert; }
		void setWidths(const std::array<float,4>& widths) { widths_ = widths; }
		void setOutset(const std::array<float,4>& outset) { outset_ = outset; }
		void setSlice(const std::array<float,4>& slice) { slice_ = slice; }
		void setFill(bool fill) { fill_ = fill; }
		bool isValid(css::Side side) const { return (border_style_[static_cast<int>(side)] != css::BorderStyle::HIDDEN 
			&& border_style_[static_cast<int>(side)] != css::BorderStyle::NONE) || !uri_.isNone(); }

		void setBorderStyleTop(css::BorderStyle bs) { border_style_[0] = bs; }
		void setBorderStyleLeft(css::BorderStyle bs) { border_style_[1] = bs; }
		void setBorderStyleBottom(css::BorderStyle bs) { border_style_[2] = bs; }
		void setBorderStyleRight(css::BorderStyle bs) { border_style_[3] = bs; }

		void setBorderColorTop(const KRE::Color& color) { border_color_[0] = color; }
		void setBorderColorLeft(const KRE::Color& color) { border_color_[1] = color; }
		void setBorderColorBottom(const KRE::Color& color) { border_color_[2] = color; }
		void setBorderColorRight(const KRE::Color& color) { border_color_[3] = color; }

	private:
		// pre-compute values.
		css::UriStyle uri_;
		std::shared_ptr<css::WidthList> border_width_;
		std::shared_ptr<css::WidthList> border_outset_;
		std::shared_ptr<css::BorderImageSlice> border_slice_;
		std::shared_ptr<css::BorderImageRepeat> border_repeat_;

		// CSS3 properties
		KRE::TexturePtr image_;
		bool fill_;
		std::array<float,4> slice_;
		std::array<float,4> outset_;
		std::array<float,4> widths_;
		css::CssBorderImageRepeat repeat_horiz_;
		css::CssBorderImageRepeat repeat_vert_;

		std::array<css::BorderStyle, 4> border_style_;
		std::array<KRE::Color, 4> border_color_;
	};

}
