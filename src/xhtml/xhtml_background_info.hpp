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
	struct BgBoxShadow
	{
		BgBoxShadow() : x_offset(0), y_offset(0), blur_radius(0), spread_radius(0), inset(false), color(KRE::Color::colorBlack()) {}
		BgBoxShadow(FixedPoint x, FixedPoint y, FixedPoint blur, FixedPoint spread, bool ins, const KRE::Color& col) 
			: x_offset(x), y_offset(y), blur_radius(blur), spread_radius(spread), inset(ins), color(col) {}
		FixedPoint x_offset;
		FixedPoint y_offset;
		FixedPoint blur_radius;
		FixedPoint spread_radius;
		bool inset;
		KRE::Color color;
	};

	class BackgroundInfo
	{
	public:
		BackgroundInfo();
		void setColor(const KRE::Color& color) { color_ = color; }
		void setFile(const std::string& filename);
		void setPosition(const std::shared_ptr<css::BackgroundPosition>& pos) { position_ = pos; }
		void setRepeat(css::BackgroundRepeat repeat) { repeat_ = repeat; }
		void render(DisplayListPtr display_list, const point& offset, const Dimensions& dims) const;
		void init(const Dimensions& dims);
	private:
		void renderBoxShadow(DisplayListPtr display_list, const point& offset, const Dimensions& dims) const;
		KRE::Color color_;
		KRE::TexturePtr texture_;
		css::BackgroundRepeat repeat_;
		std::shared_ptr<css::BackgroundPosition> position_;

		std::array<FixedPoint, 4> border_radius_horiz_;
		std::array<FixedPoint, 4> border_radius_vert_;

		// box-shadow properties.
		std::vector<BgBoxShadow> box_shadows_;

		// background-clip
		css::BackgroundClip background_clip_; 

		bool has_border_radius_;
	};
}
