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

#include "AttributeSet.hpp"
#include "DisplayDevice.hpp"
#include "SceneObject.hpp"
#include "Shaders.hpp"
#include "StencilSettings.hpp"

#include "asserts.hpp"
#include "gradients.hpp"

namespace KRE
{
	namespace
	{
		const static glm::vec3 z_axis(0.0f, 0.0f, 1.0f);

		// XXX these need to go in stencil.
		StencilSettings& get_stencil_mask_settings() 
		{
			static KRE::StencilSettings ss(true, 
				KRE::StencilFace::FRONT_AND_BACK, 
				KRE::StencilFunc::NOT_EQUAL, 
				0xff, 0x00, 0xff, 
				KRE::StencilOperation::INCREMENT, 
				KRE::StencilOperation::KEEP, 
				KRE::StencilOperation::KEEP);
			return ss;
		}

		class SimpleClipShape : public SceneObject
		{
		public:
			SimpleClipShape()
				: SceneObject("SimpleClipShape")
			{
				setShader(ShaderProgram::getProgram("simple"));

				auto as = DisplayDevice::createAttributeSet();
				attribs_.reset(new Attribute<glm::vec2>(AccessFreqHint::DYNAMIC, AccessTypeHint::DRAW));
				attribs_->addAttributeDesc(AttributeDesc(AttrType::POSITION, 2, AttrFormat::FLOAT, false));
				as->addAttribute(AttributeBasePtr(attribs_));
				as->setDrawMode(DrawMode::TRIANGLE_STRIP);
				addAttributeSet(as);

				std::vector<glm::vec2> vc;
				vc.emplace_back(0.0f, 0.0f);
				vc.emplace_back(0.0f, 1.0f);
				vc.emplace_back(1.0f, 0.0f);
				vc.emplace_back(1.0f, 1.0f);
				attribs_->update(&vc);
			}
		private:
			std::shared_ptr<Attribute<glm::vec2>> attribs_;
		};

		class GradientRenderable : public SceneObject
		{
		public:
			GradientRenderable()
				: SceneObject("GradientRenderable")
			{
				setShader(ShaderProgram::getProgram("attr_color_shader"));

				auto as = DisplayDevice::createAttributeSet();
				attribs_.reset(new KRE::Attribute<KRE::vertex_color>(AccessFreqHint::DYNAMIC, AccessTypeHint::DRAW));
				attribs_->addAttributeDesc(AttributeDesc(AttrType::POSITION, 2, AttrFormat::FLOAT, false, sizeof(vertex_color), offsetof(vertex_color, vertex)));
				attribs_->addAttributeDesc(AttributeDesc(AttrType::COLOR,  4, AttrFormat::UNSIGNED_BYTE, true, sizeof(vertex_color), offsetof(vertex_color, color)));
				as->addAttribute(AttributeBasePtr(attribs_));
				as->setDrawMode(DrawMode::TRIANGLES);
		
				addAttributeSet(as);
			}
			void update(std::vector<KRE::vertex_color>* coords)
			{
				attribs_->update(coords);
			}
		private:
			std::shared_ptr<KRE::Attribute<KRE::vertex_color>> attribs_;
		};
		typedef std::shared_ptr<GradientRenderable> GradientRenderablePtr;
	}

	SceneObjectPtr LinearGradient::createRenderable()
	{
		ASSERT_LOG(color_stops_.size() >= 2, "Must be at least two color stops.");
		ASSERT_LOG(std::abs(color_stops_.front().length) < FLT_EPSILON, "First stop must be at 0");
		ASSERT_LOG(std::abs(color_stops_.back().length - 1.0f) < FLT_EPSILON, "Last stop must be at 1");
		// attempt one.
		int number_strips = color_stops_.size() - 1;
		const float y_incr = 1.0f / static_cast<float>(number_strips);

		auto gr = std::make_shared<GradientRenderable>();
		gr->setRotation(-angle_, z_axis);
		//auto clip_mask = std::make_shared<SimpleClipShape>();
		//clip_mask->setRotation(-angle_, z_axis);
		//gr->setClipSettings(get_stencil_mask_settings(), clip_mask);
		
		std::vector<KRE::vertex_color> vc;
		vc.reserve(6 * number_strips);

		// assume a box size from 0 -> 1, 0 -> 1
		for(int strip = 0; strip != number_strips; ++strip) {
			
			const float vx1 = 0.0f;
			const float vy1 = color_stops_[strip].length;
			const float vx2 = 1.0f;
			const float vy2 = color_stops_[strip + 1].length;

			vc.emplace_back(glm::vec2(vx1, vy1), color_stops_[strip].color->as_u8vec4());
			vc.emplace_back(glm::vec2(vx1, vy2), color_stops_[strip + 1].color->as_u8vec4());
			vc.emplace_back(glm::vec2(vx2, vy2), color_stops_[strip + 1].color->as_u8vec4());

			vc.emplace_back(glm::vec2(vx1, vy1), color_stops_[strip].color->as_u8vec4());
			vc.emplace_back(glm::vec2(vx2, vy2), color_stops_[strip + 1].color->as_u8vec4());
			vc.emplace_back(glm::vec2(vx2, vy1), color_stops_[strip].color->as_u8vec4());
		}
		
		gr->update(&vc);

		return gr;
	}
}
