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

#include <cairo.h>

#include "AttributeSet.hpp"
#include "Blittable.hpp"
#include "DisplayDevice.hpp"
#include "SceneObject.hpp"
#include "Shaders.hpp"

#include "profile_timer.hpp"
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
		auto& shadows = RenderContext::get().getComputedValue(Property::BOX_SHADOW).getValue<BoxShadowStyle>().getShadows();

		for(auto shadow : shadows) {
			box_shadows_.emplace_back(shadow.getX().compute(), 
				shadow.getY().compute(), 
				shadow.getBlur().compute(), 
				shadow.getSpread().compute(), 
				shadow.inset(), 
				shadow.getColor().compute());
		}
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

	void BackgroundInfo::renderBoxShadow(DisplayListPtr display_list, const point& offset, const Dimensions& dims) const
	{
		// XXX We should be using the shape generated via clipping.
		const FixedPoint left = dims.content_.x - dims.border_.left - dims.padding_.left - dims.margin_.left;
		const FixedPoint top = dims.content_.y - dims.border_.top - dims.padding_.top - dims.margin_.top;
		const FixedPoint right = dims.content_.x + dims.content_.width + dims.border_.right + dims.padding_.right + dims.margin_.right;
		const FixedPoint bottom = dims.content_.y + dims.content_.height + dims.border_.bottom + dims.padding_.bottom + dims.margin_.bottom;

		const int kernel_size = 17;
		const int half = kernel_size / 2;
		std::vector<uint8_t> kernel;
		kernel.resize(kernel_size);

		for(auto& shadow : box_shadows_) {
			if(shadow.inset) {
				// XXX
			} else {
				const FixedPoint spread_left = left - shadow.spread_radius;
				const FixedPoint spread_top = top - shadow.spread_radius;
				const FixedPoint spread_right = right + shadow.spread_radius;
				const FixedPoint spread_bottom = bottom + shadow.spread_radius;
				
				const int width = (spread_right - spread_left) / LayoutEngine::getFixedPointScale() + half * 2; 
				const int height = (spread_bottom - spread_top) / LayoutEngine::getFixedPointScale() + half * 2;

				cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
				cairo_t* cairo = cairo_create(surface);

				{
				profile::manager pman("cario_rect");
				cairo_set_source_rgba(cairo, 0, 0, 0, 0.05);
				cairo_rectangle(cairo, 0, 0, width, height);
				cairo_fill(cairo);

				cairo_set_source_rgba(cairo, shadow.color.r(), shadow.color.g(), shadow.color.b(), shadow.color.a());
				cairo_rectangle(cairo, 
					left / LayoutEngine::getFixedPointScaleFloat(), 
					top / LayoutEngine::getFixedPointScaleFloat(), 
					(right - left) / LayoutEngine::getFixedPointScaleFloat(), 
					(bottom - top) / LayoutEngine::getFixedPointScaleFloat());
				cairo_fill(cairo);
				}

				cairo_surface_t* tmp = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
				uint8_t *src = cairo_image_surface_get_data(surface);
				const int src_stride = cairo_image_surface_get_stride(surface);
				uint8_t *dst = cairo_image_surface_get_data(tmp);
				const int dst_stride = cairo_image_surface_get_stride(tmp);

				profile::manager pman("convolution");
				unsigned acc = 0;
				for(int n = 0; n != kernel_size; ++n) {
					double f = n - half;
					kernel[n] = static_cast<uint8_t>(std::exp(-f * f / 30.0) * 80);
					acc += kernel[n];
				}

				for(int y = 0; y != height; ++y) {
					uint32_t* s = reinterpret_cast<uint32_t *>(src + y * src_stride);
					uint32_t* d = reinterpret_cast<uint32_t *>(dst + y * dst_stride);
					for(int x = 0; x != width; ++x) {
						if(shadow.blur_radius < x && x < width - shadow.blur_radius) {
							d[x] = s[x];
							continue;
						}

						int r = 0, g = 0, b = 0, a = 0;
						for(int n = 0; n != kernel_size; ++n) {
							if (x - half + n < 0 || x - half + n >= width) {
								continue;
							}
							uint32_t pix = s[x - half + n];
							r += ((pix >> 24) & 0xff) * kernel[n];
							g += ((pix >> 16) & 0xff) * kernel[n];
							b += ((pix >>  8) & 0xff) * kernel[n];
							a += ((pix >>  0) & 0xff) * kernel[n];
						}
						d[x] = (r / acc << 24) | (g / acc << 16) | (b / acc << 8) | (a / acc);
					}
				}

				for(int y = 0; y != height; ++y) {
					uint32_t* s = reinterpret_cast<uint32_t *>(dst + y * dst_stride);
					uint32_t* d = reinterpret_cast<uint32_t *>(src + y * src_stride);
					for(int x = 0; x != width; ++x) {
						if(shadow.blur_radius <= y && y < height - shadow.blur_radius) {
							d[x] = s[x];
							continue;
						}

						int r = 0, g = 0, b = 0, a = 0;
						for(int n = 0; n != kernel_size; ++n) {
							if (y - half + n < 0 || y - half + n >= height) {
								continue;
							}

							s = reinterpret_cast<uint32_t *>(dst + (y - half + n) * dst_stride);
							uint32_t pix = s[x];
							r += ((pix >> 24) & 0xff) * kernel[n];
							g += ((pix >> 16) & 0xff) * kernel[n];
							b += ((pix >>  8) & 0xff) * kernel[n];
							a += ((pix >>  0) & 0xff) * kernel[n];
						}
						d[x] = (r / acc << 24) | (g / acc << 16) | (b / acc << 8) | (a / acc);
					}
				}

				auto surf = KRE::Surface::create(width, height, KRE::PixelFormat::PF::PIXELFORMAT_ARGB8888);
				surf->writePixels(src, src_stride * height);
				surf->createAlphaMap();

				auto ptr = std::make_shared<KRE::Blittable>(KRE::Texture::createTexture(surf));
				ptr->setCentre(KRE::Blittable::Centre::TOP_LEFT);
				ptr->setPosition(half + shadow.x_offset / LayoutEngine::getFixedPointScale(), half + shadow.y_offset / LayoutEngine::getFixedPointScale());
				display_list->addRenderable(ptr);

				cairo_destroy(cairo);
				cairo_surface_destroy(surface);
				cairo_surface_destroy(tmp);
			}
		}
	}

	void BackgroundInfo::render(DisplayListPtr display_list, const point& offset, const Dimensions& dims) const
	{
		renderBoxShadow(display_list, offset, dims);

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
