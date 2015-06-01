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
#include "Blittable.hpp"
#include "DisplayDevice.hpp"
#include "SceneObject.hpp"
#include "Shaders.hpp"

#include "solid_renderable.hpp"
#include "xhtml_background_info.hpp"
#include "xhtml_layout_engine.hpp"

namespace xhtml
{
	using namespace css;

	BackgroundInfo::BackgroundInfo()
		: color_(0, 0, 0, 0),
		  texture_(),
		  repeat_(CssBackgroundRepeat::REPEAT),
		  position_()
	{
	}

	void BackgroundInfo::setFile(const std::string& filename)
	{
		texture_ = KRE::Texture::createTexture(filename);
		switch(repeat_) {
			case CssBackgroundRepeat::REPEAT:
				texture_->setAddressModes(0, KRE::Texture::AddressMode::WRAP, KRE::Texture::AddressMode::WRAP, KRE::Texture::AddressMode::WRAP);
				break;
			case CssBackgroundRepeat::REPEAT_X:
				texture_->setAddressModes(0, KRE::Texture::AddressMode::WRAP, KRE::Texture::AddressMode::BORDER, KRE::Texture::AddressMode::BORDER, KRE::Color(0,0,0,0));
				break;
			case CssBackgroundRepeat::REPEAT_Y:
				texture_->setAddressModes(0, KRE::Texture::AddressMode::BORDER, KRE::Texture::AddressMode::WRAP, KRE::Texture::AddressMode::BORDER, KRE::Color(0,0,0,0));
				break;
			case CssBackgroundRepeat::NO_REPEAT:
				texture_->setAddressModes(0, KRE::Texture::AddressMode::BORDER, KRE::Texture::AddressMode::BORDER, KRE::Texture::AddressMode::BORDER, KRE::Color(0,0,0,0));
				break;
		}
	}

	void BackgroundInfo::render(DisplayListPtr display_list, const point& offset, const Dimensions& dims) const
	{
		// XXX if we're rendering the body element then it takes the entire canvas :-/
		// technically the rule is that if no background styles are applied to the html element then
		// we apply the body styles.
		const int rx = offset.x - dims.padding_.left - dims.border_.left;
		const int ry = offset.y - dims.padding_.top - dims.border_.top;
		const int rw = dims.content_.width + dims.padding_.left + dims.padding_.right + dims.border_.left + dims.border_.right;
		const int rh = dims.content_.height + dims.padding_.top + dims.padding_.bottom + dims.border_.top + dims.border_.bottom;
		rect r(rx, ry, rw, rh);

		if(color_.ai() != 0) {
			display_list->addRenderable(std::make_shared<SolidRenderable>(r, color_));
		}
		// XXX if texture is set then use background position and repeat as appropriate.
		if(texture_) {
			// XXX this needs fixing.
			// we probably should clone the texture we pass to blittable.
			// With a value pair of '14% 84%', the point 14% across and 84% down the image is to be placed at the point 14% across and 84% down the padding box.
			const int sw = texture_->surfaceWidth();
			const int sh = texture_->surfaceHeight();

			int sw_offs = 0;
			int sh_offs = 0;

			if(position_.getLeft().isPercent()) {
				sw_offs = position_.getLeft().compute(sw * LayoutEngine::getFixedPointScale());
			}
			if(position_.getTop().isPercent()) {
				sh_offs = position_.getTop().compute(sh * LayoutEngine::getFixedPointScale());
			}

			const int rw_offs = position_.getLeft().compute(rw);
			const int rh_offs = position_.getTop().compute(rh);
			
			const float rxf = static_cast<float>(rx) / LayoutEngine::getFixedPointScaleFloat();
			const float ryf = static_cast<float>(ry) / LayoutEngine::getFixedPointScaleFloat();

			const float left = static_cast<float>(rw_offs - sw_offs + rx) / LayoutEngine::getFixedPointScaleFloat();
			const float top = static_cast<float>(rh_offs - sh_offs + ry) / LayoutEngine::getFixedPointScaleFloat();
			const float width = static_cast<float>(rw) / LayoutEngine::getFixedPointScaleFloat();
			const float height = static_cast<float>(rh) / LayoutEngine::getFixedPointScaleFloat();

			auto tex = texture_->clone();
			auto ptr = std::make_shared<KRE::Blittable>(tex);
			ptr->setCentre(KRE::Blittable::Centre::TOP_LEFT);
			switch(repeat_) {
				case CssBackgroundRepeat::REPEAT:
					tex->setSourceRect(0, rect(-left, -top, width, height));
					ptr->setDrawRect(rectf(rxf, ryf, width, height));
					break;
				case CssBackgroundRepeat::REPEAT_X:
					tex->setSourceRect(0, rect(-left, 0, width, sh));
					ptr->setDrawRect(rectf(rxf, top, width, static_cast<float>(sh)));
					break;
				case CssBackgroundRepeat::REPEAT_Y:
					tex->setSourceRect(0, rect(0, -top, sw, height));
					ptr->setDrawRect(rectf(left, ryf, static_cast<float>(sw), height));
					break;
				case CssBackgroundRepeat::NO_REPEAT:
					tex->setSourceRect(0, rect(0, 0, sw, sh));
					ptr->setDrawRect(rectf(left, top, static_cast<float>(sw), static_cast<float>(sh)));
					break;
			}

			display_list->addRenderable(ptr);
		}
	}
}
