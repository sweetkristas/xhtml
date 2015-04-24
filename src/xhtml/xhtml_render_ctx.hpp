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

#include <string>

namespace xhtml
{
	class RenderContext
	{
	public:
		explicit RenderContext(const std::string& font_name, double font_size);
		void setFont(const std::string& name) { font_name_ = name; }
		void setFontSize(double fs, double xheight=0) { font_size_ = fs; font_xheight_ = xheight==0 ? 0.5*fs : xheight; }
		const std::string& getFontName() const { return font_name_; }
		double getFontSize() const { return font_size_; }
		double getFontXHeight() const { return font_xheight_; }
	private:
		std::string font_name_;
		double font_size_;
		double font_xheight_;
	};
}
