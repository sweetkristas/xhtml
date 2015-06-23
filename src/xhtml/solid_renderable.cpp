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

#include "AttributeSet.hpp"
#include "Blittable.hpp"
#include "DisplayDevice.hpp"
#include "SceneObject.hpp"
#include "Shaders.hpp"

#include "solid_renderable.hpp"
#include "xhtml_layout_engine.hpp"

namespace xhtml
{
	SolidRenderable::SolidRenderable() 
		: KRE::SceneObject("SolidRenderable")
	{
		init();
	}

	SolidRenderable::SolidRenderable(const rect& r, const KRE::Color& color)
		: KRE::SceneObject("SolidRenderable")
	{
		init();

		const float vx1 = static_cast<float>(r.x1())/LayoutEngine::getFixedPointScaleFloat();
		const float vy1 = static_cast<float>(r.y1())/LayoutEngine::getFixedPointScaleFloat();
		const float vx2 = static_cast<float>(r.x2())/LayoutEngine::getFixedPointScaleFloat();
		const float vy2 = static_cast<float>(r.y2())/LayoutEngine::getFixedPointScaleFloat();

		std::vector<KRE::vertex_color> vc;
		vc.emplace_back(glm::vec2(vx1, vy2), color.as_u8vec4());
		vc.emplace_back(glm::vec2(vx1, vy1), color.as_u8vec4());
		vc.emplace_back(glm::vec2(vx2, vy1), color.as_u8vec4());

		vc.emplace_back(glm::vec2(vx2, vy1), color.as_u8vec4());
		vc.emplace_back(glm::vec2(vx2, vy2), color.as_u8vec4());
		vc.emplace_back(glm::vec2(vx1, vy2), color.as_u8vec4());
		attribs_->update(&vc);
	}

	void SolidRenderable::setDrawMode(KRE::DrawMode draw_mode)
	{
		getAttributeSet().back()->setDrawMode(draw_mode);
	}

	void SolidRenderable::init()
	{
		using namespace KRE;
		setShader(ShaderProgram::getProgram("attr_color_shader"));

		auto as = DisplayDevice::createAttributeSet();
		attribs_.reset(new KRE::Attribute<KRE::vertex_color>(AccessFreqHint::DYNAMIC, AccessTypeHint::DRAW));
		attribs_->addAttributeDesc(AttributeDesc(AttrType::POSITION, 2, AttrFormat::FLOAT, false, sizeof(vertex_color), offsetof(vertex_color, vertex)));
		attribs_->addAttributeDesc(AttributeDesc(AttrType::COLOR,  4, AttrFormat::UNSIGNED_BYTE, true, sizeof(vertex_color), offsetof(vertex_color, color)));
		as->addAttribute(AttributeBasePtr(attribs_));
		as->setDrawMode(DrawMode::TRIANGLES);
		
		addAttributeSet(as);
	}

	void SolidRenderable::update(std::vector<KRE::vertex_color>* coords)
	{
		attribs_->update(coords);
	}

	BlurredSolidRenderable::BlurredSolidRenderable() 
		: KRE::SceneObject("BlurredSolidRenderable")
	{
		init();
	}

	BlurredSolidRenderable::BlurredSolidRenderable(const rect& r, const KRE::Color& color, float blur_radius)
		: KRE::SceneObject("BlurredSolidRenderable")
	{
		init();

		/*const float vx1 = static_cast<float>(r.x1());
		const float vy1 = static_cast<float>(r.y1());
		const float vx2 = static_cast<float>(r.x2());
		const float vy2 = static_cast<float>(r.y2());

		const float blur_r2 = 0.70710678f;

		std::vector<blur_vertex_color> vc;
		vc.emplace_back(glm::vec2(vx1, vy2), color.as_u8vec4(), glm::vec2(-blur_r2, blur_r2));
		vc.emplace_back(glm::vec2(vx1, vy1), color.as_u8vec4(), glm::vec2(-blur_r2, -blur_r2));
		vc.emplace_back(glm::vec2(vx2, vy1), color.as_u8vec4(), glm::vec2(blur_r2, -blur_r2));

		vc.emplace_back(glm::vec2(vx2, vy1), color.as_u8vec4(), glm::vec2(blur_r2, -blur_r2));
		vc.emplace_back(glm::vec2(vx2, vy2), color.as_u8vec4(), glm::vec2(blur_r2, blur_r2));
		vc.emplace_back(glm::vec2(vx1, vy2), color.as_u8vec4(), glm::vec2(-blur_r2, blur_r2));
		attribs_->update(&vc);*/

		auto shader = getShader();
		const int u_blur = shader->getUniform("u_blur");
		const int u_line_width = shader->getUniform("u_line_width");
		const float line_width = static_cast<float>(r.h());
		const float blur = blur_radius;
		shader->setUniformDrawFunction([u_blur, u_line_width, line_width, blur](KRE::ShaderProgramPtr shader) {
			shader->setUniformValue(u_blur, blur);
			shader->setUniformValue(u_line_width, line_width);
		});
		setColor(color);

		std::vector<glm::vec2> varray;
		varray.emplace_back(r.x(), r.mid_y());
		varray.emplace_back(r.x2(), r.mid_y());

		std::vector<vertex_normal> vn;
		vn.reserve(varray.size() * 2);
		
		for(int n = 0; n != varray.size(); n += 2) {
			const float dx = varray[n+1].x - varray[n+0].x;
			const float dy = varray[n+1].y - varray[n+0].y;
			const glm::vec2 d1 = glm::normalize(glm::vec2(dy, -dx));
			const glm::vec2 d2 = glm::normalize(glm::vec2(-dy, dx));

			vn.emplace_back(varray[n+0], d1);
			vn.emplace_back(varray[n+0], d2);
			vn.emplace_back(varray[n+1], d1);
			vn.emplace_back(varray[n+1], d2);
		}
		attribs_->update(&vn);

	}

	void BlurredSolidRenderable::init()
	{
		using namespace KRE;
		/*setShader(ShaderProgram::getProgram("blur_attr_color_shader"));
		
		// XXX this should be instanced, but something is wrong with instanced rendering.
		// with blur as the instance variable.
		auto as = DisplayDevice::createAttributeSet();
		attribs_.reset(new KRE::Attribute<blur_vertex_color>(AccessFreqHint::DYNAMIC, AccessTypeHint::DRAW));
		attribs_->addAttributeDesc(AttributeDesc(AttrType::POSITION, 2, AttrFormat::FLOAT, false, sizeof(blur_vertex_color), offsetof(blur_vertex_color, vertex)));
		attribs_->addAttributeDesc(AttributeDesc(AttrType::COLOR, 4, AttrFormat::UNSIGNED_BYTE, true, sizeof(blur_vertex_color), offsetof(blur_vertex_color, color)));
		attribs_->addAttributeDesc(AttributeDesc(AttrType::NORMAL, 2, AttrFormat::FLOAT, false, sizeof(blur_vertex_color), offsetof(blur_vertex_color, normal)));
		as->addAttribute(AttributeBasePtr(attribs_));
		as->setDrawMode(DrawMode::TRIANGLES);

		addAttributeSet(as);*/

		setShader(ShaderProgram::getProgram("complex"));
		
		// XXX this should be instanced, but something is wrong with instanced rendering.
		// with blur as the instance variable.
		auto as = DisplayDevice::createAttributeSet();
		attribs_.reset(new KRE::Attribute<vertex_normal>(AccessFreqHint::DYNAMIC, AccessTypeHint::DRAW));
		attribs_->addAttributeDesc(AttributeDesc(AttrType::POSITION, 2, AttrFormat::FLOAT, false, sizeof(vertex_normal), offsetof(vertex_normal, vertex)));
		attribs_->addAttributeDesc(AttributeDesc(AttrType::NORMAL, 2, AttrFormat::FLOAT, false, sizeof(vertex_normal), offsetof(vertex_normal, normal)));
		as->addAttribute(AttributeBasePtr(attribs_));
		as->setDrawMode(DrawMode::TRIANGLE_STRIP);

		addAttributeSet(as);
	}

	void BlurredSolidRenderable::update(std::vector<vertex_normal>* coords)
	{
		attribs_->update(coords);
	}

	SimpleRenderable::SimpleRenderable()
		: KRE::SceneObject("SimpleRenderable")
	{
		init();
	}

	SimpleRenderable::SimpleRenderable(KRE::DrawMode draw_mode)
		: KRE::SceneObject("SimpleRenderable")
	{
		init(draw_mode);
	}

	void SimpleRenderable::init(KRE::DrawMode draw_mode)
	{
		using namespace KRE;
		setShader(ShaderProgram::getProgram("simple"));

		auto as = DisplayDevice::createAttributeSet();
		attribs_.reset(new KRE::Attribute<glm::vec2>(AccessFreqHint::DYNAMIC, AccessTypeHint::DRAW));
		attribs_->addAttributeDesc(AttributeDesc(AttrType::POSITION, 2, AttrFormat::FLOAT, false));
		as->addAttribute(AttributeBasePtr(attribs_));
		as->setDrawMode(draw_mode);
		
		addAttributeSet(as);
	}

	void SimpleRenderable::update(std::vector<glm::vec2>* coords)
	{
		attribs_->update(coords);
	}

	void SimpleRenderable::setDrawMode(KRE::DrawMode draw_mode)
	{
		getAttributeSet().back()->setDrawMode(draw_mode);
	}

}
