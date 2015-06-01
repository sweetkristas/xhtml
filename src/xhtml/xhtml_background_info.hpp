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
	class BackgroundInfo
	{
	public:
		BackgroundInfo();
		void setColor(const KRE::Color& color) { color_ = color; }
		void setFile(const std::string& filename);
		void setPosition(const css::BackgroundPosition& pos) { position_ = pos; }
		void setRepeat(css::CssBackgroundRepeat repeat) { repeat_ = repeat; }
		void render(DisplayListPtr display_list, const point& offset, const Dimensions& dims) const;
	private:
		KRE::Color color_;
		KRE::TexturePtr texture_;
		css::CssBackgroundRepeat repeat_;
		css::BackgroundPosition position_;
	};
}
