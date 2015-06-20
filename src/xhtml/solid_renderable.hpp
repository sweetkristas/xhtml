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

#include "AttributeSet.hpp"
#include "SceneObject.hpp"

namespace xhtml
{
	class SimpleRenderable : public KRE::SceneObject
	{
	public:
		SimpleRenderable();
		explicit SimpleRenderable(KRE::DrawMode draw_mode);
		void init(KRE::DrawMode draw_mode = KRE::DrawMode::TRIANGLES);
		void update(std::vector<glm::vec2>* coords);
		void setDrawMode(KRE::DrawMode draw_mode);
	private:
		std::shared_ptr<KRE::Attribute<glm::vec2>> attribs_;
	};

	class SolidRenderable : public KRE::SceneObject
	{
	public:
		SolidRenderable();
		explicit SolidRenderable(const rect& r, const KRE::Color& color);
		void init();
		void update(std::vector<KRE::vertex_color>* coords);
		void setDrawMode(KRE::DrawMode draw_mode);
	private:
		std::shared_ptr<KRE::Attribute<KRE::vertex_color>> attribs_;
	};

	struct blur_vertex_color
	{
		blur_vertex_color(const glm::vec2& v, const glm::u8vec4& c, const glm::vec2& norm)
			: vertex(v), color(c), normal(norm) {}
		glm::vec2 vertex;
		glm::u8vec4 color;
		glm::vec2 normal;
	};

	struct vertex_normal
	{
		vertex_normal(const glm::vec2& v, const glm::vec2& norm)
			: vertex(v), normal(norm) {}
		glm::vec2 vertex;
		glm::vec2 normal;
	};

	class BlurredSolidRenderable : public KRE::SceneObject
	{
	public:
		BlurredSolidRenderable();
		explicit BlurredSolidRenderable(const rect& r, const KRE::Color& color, float blur_radius);
		void init();
		void update(std::vector<vertex_normal>* coords);
	private:
		std::shared_ptr<KRE::Attribute<vertex_normal>> attribs_;
	};
}