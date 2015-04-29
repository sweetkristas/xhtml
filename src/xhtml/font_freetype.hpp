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

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "geometry.hpp"
#include "AttributeSet.hpp"
#include "Color.hpp"
#include "RenderFwd.hpp"
#include "SceneObject.hpp"
#include "Texture.hpp"

namespace KRE
{
	typedef std::map<std::string, std::string> font_path_cache;	

	struct FontError2 : public std::runtime_error
	{
		FontError2(const std::string& str) : std::runtime_error(str) {}
	};

	struct GlyphInfo
	{
		// X co-ordinate of top-left corner of glyph in texture.
		unsigned short tex_x;
		// Y co-ordinate of top-left corner of glyph in texture.
		unsigned short tex_y;
		// Width of glyph in texture.
		unsigned short width;
		// Hidth of glyph in texture.
		unsigned short height;
		// X advance (i.e. distance to start of next glyph on X axis)
		long advance_x;
		// Y advance (i.e. distance to start of next glyph on Y axis)
		long advance_y;
		// X offset to top of glyph from origin
		long bearing_x;
		// Y offset to top of glyph from origin
		long bearing_y;
	};

	struct font_coord
	{
		font_coord(const glm::vec2& v, const glm::vec2& t) : vtx(v), tc(t) {}
		glm::vec2 vtx;
		glm::vec2 tc;
	};

	class FontRenderable : public SceneObject
	{
	public:
		FontRenderable();
		void clear();
		void update(std::vector<font_coord>* queue);
	private:
		std::shared_ptr<Attribute<font_coord>> attribs_;
	};
	typedef std::shared_ptr<FontRenderable> FontRenderablePtr;

	class FontHandle
	{
	public:
		FontHandle(const std::string& fnt_name, const std::string& fnt_path, float size, const Color& color);
		~FontHandle();
		float getFontSize();
		float getFontXHeight();
		const std::string& getFontName();
		const std::string& getFontPath();
		const std::string& getFontFamily();
		void renderText();
		void getFontMetrics();
		rect getBoundingBox(const std::string& text);
		FontRenderablePtr createRenderableFromPath(FontRenderablePtr r, const std::string& text, const std::vector<geometry::Point<long>>& path);
		void getGlyphPath(const std::string& text, std::vector<geometry::Point<long>>* path);
		long calculateCharAdvance(char32_t cp);
		long getScaleFactor() const { return 65536; }
		const GlyphInfo& getGlyphInfo(char32_t cp);
	private:
		class Impl;
		std::unique_ptr<Impl> impl_;
	};
	typedef std::shared_ptr<FontHandle> FontHandlePtr;

	class FontDriver
	{
	public:
		static FontHandlePtr getFontHandle(const std::vector<std::string>& font_list, float size, const Color& color=Color::colorWhite());
		static void setAvailableFonts(const font_path_cache& font_map);
		//static TexturePtr renderText(const std::string& text, ...);
	private:
		FontDriver();
	};
}
