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
#include FT_BBOX_H

#include "formatter.hpp"
#include "font_freetype.hpp"
#include "utf8_to_codepoint.hpp"

namespace KRE
{
	namespace
	{
		const int default_dpi = 96;

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

		FT_Library& get_ft_library()
		{
			static FT_Library library = nullptr;
			if(library == nullptr) {
				FT_Error error = FT_Init_FreeType(&library);
				ASSERT_LOG(error == 0, "Unable to initialise freetype library: " << error);				
			}
			return library;
		}
	}

	FontDriver::FontDriver()
	{
	}

	void FontDriver::setAvailableFonts(const font_path_cache& font_map)
	{
		get_font_path_cache() = font_map;
	}

	FontHandlePtr FontDriver::getFontHandle(const std::vector<std::string>& font_list, float size, const Color& color)
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
							it = get_font_path_cache().find(it->second);
							if(it != get_font_path_cache().end()) {
								selected_font = it->second;
								break;
							}
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
			throw FontError2(ss.str());
		}

		return std::make_shared<FontHandle>(selected_font, size, color);
	}

	class FontHandle::Impl
	{
	public:
		Impl(const std::string& fnt_name, float size, const Color& color)
			: fnt_(fnt_name),
			  size_(size),
			  color_(color),
			  face_(nullptr),
			  has_kerning_(false),
			  x_height_(0)
		{
			// XXX starting off with a basic way of rendering glyphs.
			// It'd be better to render all the glyphs to a texture,
			// then use a VBO to store texture co-ords for indivdual glyphs and 
			// vertex co-ords where to place them. Thus we could always use the 
			// same texture for a particular font.
			// More advanded ideas involve decomposing glyph outlines, triangulating 
			// and storing those in a VBO for rendering.
			auto lib = get_ft_library();
			FT_Error error = FT_New_Face(lib, fnt_name.c_str(), 0, &face_);
			ASSERT_LOG(error == 0, "Error reading font file: " << fnt_name << ", error was: " << error);
			error = FT_Set_Char_Size(face_, static_cast<int>(size * 64), 0, default_dpi, 0);
			ASSERT_LOG(error == 0, "Error setting character size, file: " << fnt_name << ", error was: " << error);
			has_kerning_ = FT_HAS_KERNING(face_) ? true : false;
			std::ostringstream debug_ss;
			debug_ss << "Loaded font '" << fnt_name << "'\n\tfamily name: '" << face_->family_name 
				<< "'\n\tnumber of glyphs: " << face_->num_glyphs
				<< "\n\tunits per EM: " << face_->units_per_EM 
				<< "\n\thas_kerning: " 
				<< (has_kerning_ ? "true" : "false");
			LOG_DEBUG(debug_ss.str());

			FT_UInt glyph_index = FT_Get_Char_Index(face_, 'x');
			FT_Load_Glyph(face_, glyph_index, FT_LOAD_DEFAULT);
			x_height_ = face_->glyph->metrics.height / 64.0f;
		}
		~Impl() 
		{
			if(face_) {
				FT_Done_Face(face_);
				face_ = nullptr;
			}
		}
		void getBoundingBox(const std::string& str, double* w, double* h) 
		{
			FT_GlyphSlot slot = face_->glyph;
			FT_UInt previous_glyph = 0;
			ASSERT_LOG(w != nullptr && h != nullptr, "w or h is nullptr");
			FT_Vector pen = { 0, 0 };
			for(char32_t cp : utils::utf8_to_codepoint(str)) {
				FT_UInt glyph_index = FT_Get_Char_Index(face_, cp);
				if(has_kerning_ && previous_glyph && glyph_index) {
					FT_Vector  delta;
					FT_Get_Kerning(face_, previous_glyph, glyph_index, FT_KERNING_DEFAULT, &delta);
					pen.x += delta.x;
				}
				FT_Error error;
				if((error = FT_Load_Glyph(face_, glyph_index, FT_LOAD_DEFAULT)) != 0) {
					continue;
				}
				pen.x += slot->advance.x;
				pen.y += slot->advance.y;
				previous_glyph = glyph_index;
			}
			// This is to ensure that the returned dimensions are tight, i.e. the final advance is replaced by the width of the character.
			*w = (pen.x - slot->advance.x + slot->metrics.width) / 64.0;
			*h = (pen.y - slot->advance.y + slot->metrics.height) / 64.0;
		}
		SurfacePtr renderTextAsSurface(const std::string& utf8_str)
		{
			FT_Vector pen = { 0, 0 };
			FT_GlyphSlot slot = face_->glyph;
			FT_Error error;
			double w, h;			
			getBoundingBox(utf8_str, &w, &h);
			SurfacePtr dest = Surface::create(static_cast<int>(w), static_cast<int>(h), PixelFormat::PF::PIXELFORMAT_RGBA8888);
			FT_UInt previous_glyph = 0;
			for(char32_t cp : utils::utf8_to_codepoint(utf8_str)) {
				// FT_Set_Transform(face, &matrix, &pen);

				FT_UInt glyph_index = FT_Get_Char_Index(face_, cp);
				if(has_kerning_ && previous_glyph && glyph_index) {
					FT_Vector  delta;
					FT_Get_Kerning(face_, previous_glyph, glyph_index, FT_KERNING_DEFAULT, &delta);
					pen.x += delta.x;
				}

				if((error = FT_Load_Glyph(face_, glyph_index, FT_LOAD_RENDER)) != 0) {
					continue;
				}
				SurfacePtr src = Surface::create(slot->bitmap.width, slot->bitmap.rows, PixelFormat::PF::PIXELFORMAT_RGBA8888);
				dest->blitTo(src, rect(pen.x >> 6, pen.y >> 6));
				pen.x += slot->advance.x;
				pen.y += slot->advance.y;

				previous_glyph = glyph_index;
			}
		}

		void getGlyphPath(const std::string& text, std::vector<geometry::Point<double>>* path)
		{
			FT_Vector pen = { 0, 0 };
			FT_GlyphSlot slot = face_->glyph;
			FT_Error error;
			FT_UInt previous_glyph = 0;
			for(char32_t cp : utils::utf8_to_codepoint(text)) {
				path->emplace_back(pen.x / 64.0, pen.y / 64.0);
				FT_UInt glyph_index = FT_Get_Char_Index(face_, cp);
				if(has_kerning_ && previous_glyph && glyph_index) {
					FT_Vector  delta;
					FT_Get_Kerning(face_, previous_glyph, glyph_index, FT_KERNING_DEFAULT, &delta);
					pen.x += delta.x;
				}
				if((error = FT_Load_Glyph(face_, glyph_index, FT_LOAD_RENDER)) != 0) {
					continue;
				}
				pen.x += slot->advance.x;
				pen.y += slot->advance.y;

				previous_glyph = glyph_index;
			}
			// pushing back the end point so we know where the next letter starts.
			path->emplace_back(pen.x / 64.0, pen.y / 64.0);
		}
		
		double calculateCharAdvance(char32_t cp)
		{
			FT_Error error;
			FT_GlyphSlot slot = face_->glyph;
			if((error = FT_Load_Char(face_, cp, FT_LOAD_RENDER)) != 0) {
				return 0;
			}
			// XXX we should return int's and use the *64 version, stuff processing power.
			return slot->advance.x / 64.0;
		}
	private:
		std::string fnt_;
		float size_;
		Color color_;
		FT_Face face_;
		bool has_kerning_;
		float x_height_;
		friend class FontHandle;
	};
	
	FontHandle::FontHandle(const std::string& fnt_name, float size, const Color& color)
		: impl_(new Impl(fnt_name, size, color))
	{
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
		return impl_->x_height_;
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

	void FontHandle::getGlyphPath(const std::string& text, std::vector<geometry::Point<double>>* path)
	{
		impl_->getGlyphPath(text, path);
	}

	rectf FontHandle::getBoundingBox(const std::string& text)
	{
		return rectf();
	}

	double FontHandle::calculateCharAdvance(char32_t cp)
	{
		return impl_->calculateCharAdvance(cp);
	}
}
