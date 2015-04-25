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

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
#include FT_LCD_FILTER_H

#include "formatter.hpp"
#include "font_freetype.hpp"


namespace xhtml
{
	namespace
	{
		font_path_cache& get_font_path_cache()
		{
			static font_path_cache res;
			return res;
		}

		font_path_cache& generic_font_lookup()
		{
			static font_path_cache res;
			if(res.empty()) {
				// XXX ideally we read this from a config file.
				res["serif"] = "FreeSerif.ttf";
				res["sans-serif"] = "FreeSans.ttf";
				res["cursive"] = "Allura-Regular.ttf";
				res["fantasy"] = "TradeWinds-Regular.ttf";
				res["monospace"] = "SourceCodePro-Regular.ttf";
			}
			return res;
		}
	}

	FontDriver::FontDriver()
	{
	}

	void FontDriver::setAvailableFonts(const font_path_cache& font_map)
	{
		get_font_path_cache() = font_map;
	}

	FontHandlePtr FontDriver::getFontHandle(const std::vector<std::string>& font_list, int size, const KRE::Color& color)
	{
		std::string selected_font;
		for(auto& fnt : font_list) {
			auto it = get_font_path_cache().find(fnt);
			if(it != get_font_path_cache().end()) {
				selected_font = it->second;
				break;
			} else {
				it = get_font_path_cache().find(fnt + ".ttf");
				if(it != get_font_path_cache().end()) {
					selected_font = it->second;
					break;
				} else {
					it = get_font_path_cache().find(fnt + ".otf");
					if(it != get_font_path_cache().end()) {
						selected_font = it->second;
						break;
					} else {
						it = generic_font_lookup().find(fnt);
						if(it != generic_font_lookup().end()) {
							selected_font = it->second;
							break;
						}
					}
				}
			}
		}

		if(selected_font.empty()) {
			std::ostringstream ss;
			ss << "Unable to find a font to match in the given list:";
			for(auto& fnt : font_list) {
				ss << " " << fnt;
			}
			throw FontError(ss.str());
		}

		return std::make_shared<FontHandle>(selected_font, size, color);
	}

	struct FontHandle::Impl
	{
		std::string fnt_;
		float size_;
		KRE::Color color_;
	};
	
	FontHandle::FontHandle(const std::string& fnt_name, int size, const KRE::Color& color)
		: impl_(new Impl())
	{
		impl_->fnt_ = fnt_name;
		impl_->size_ = size;
		impl_->color_ = color;
	}

	FontHandle::~FontHandle()
	{
	}

	float FontHandle::getFontSize()
	{
		return impl_->size_;
	}

	float FontHandle::getFontXHeight()
	{
		// XXX
		return impl_->size_ / 2.0f;
	}

	const std::string& FontHandle::getFontName()
	{
		return impl_->fnt_;
	}

	const std::string& FontHandle::getFontFamily()
	{
		// XXX
		return impl_->fnt_;
	}

	void FontHandle::renderText()
	{
	}

	void FontHandle::getFontMetrics()
	{
	}

	rectf FontHandle::getBoundingBox(const std::string& text)
	{
		return rectf();
	}

}
