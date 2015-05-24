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

#include "asserts.hpp"

#include "css_styles.hpp"
#include "xhtml_render_ctx.hpp"

namespace css
{
	namespace 
	{
		const int fixed_point_scale = 65536;

		std::vector<float>& get_font_size_table(float ppi)
		{
			static std::vector<float> res;
			if(res.empty()) {
				// First guess implementation.
				float min_size = 9.0f / 72.0f * ppi;
				res.emplace_back(min_size);
				res.emplace_back(std::ceil(min_size * 1.1f));
				res.emplace_back(std::ceil(min_size * 1.3f));
				res.emplace_back(std::ceil(min_size * 1.45f));
				res.emplace_back(std::ceil(min_size * 1.6f));
				res.emplace_back(std::ceil(min_size * 1.8f));
				res.emplace_back(std::ceil(min_size * 2.0f));
				res.emplace_back(std::ceil(min_size * 2.3f));
			}
			return res;
		}
	}

	CssColor::CssColor()
		: param_(CssColorParam::VALUE),
		  color_(KRE::Color::colorWhite())
	{
	}

	CssColor::CssColor(CssColorParam param, const KRE::Color& color)
		: param_(param),
		  color_(color)
	{
	}

	void CssColor::setParam(CssColorParam param)
	{
		param_ = param;
		if(param_ != CssColorParam::VALUE) {
			color_ = KRE::Color(0, 0, 0, 0);
		}
	}

	void CssColor::setColor(const KRE::Color& color)
	{
		color_ = color;
		setParam(CssColorParam::VALUE);
	}

	KRE::Color CssColor::compute() const
	{
		if(param_ == CssColorParam::VALUE) {
			return color_;
		} else if(param_ == CssColorParam::CURRENT) {
			auto& ctx = xhtml::RenderContext::get();
			auto current_color = ctx.getComputedValue(Property::COLOR).getValue<CssColor>();
			ASSERT_LOG(current_color.getParam() != CssColorParam::CURRENT, "Computing color of current color would cause infinite loop.");
			return current_color.compute();
		}
		return KRE::Color(0, 0, 0, 0);
	}

	Object CssColor::evaluate(const xhtml::RenderContext& ctx) const
	{
		return Object(*this);
	}

	Length::Length(xhtml::FixedPoint value, const std::string& units) 
		: value_(value), 
		  units_(LengthUnits::NUMBER) 
	{
		if(units == "em") {
			units_ = LengthUnits::EM;
		} else if(units == "ex") {
			units_ = LengthUnits::EX;
		} else if(units == "in") {
			units_ = LengthUnits::IN;
		} else if(units == "cm") {
			units_ = LengthUnits::CM;
		} else if(units == "mm") {
			units_ = LengthUnits::MM;
		} else if(units == "pt") {
			units_ = LengthUnits::PT;
		} else if(units == "pc") {
			units_ = LengthUnits::PC;
		} else if(units == "px") {
			units_ = LengthUnits::PX;
		} else if(units == "%") {
			units_ = LengthUnits::PERCENT;
			// normalize to range 0.0 -> 1.0
			value_ = fixed_point_scale;
		} else {
			LOG_ERROR("unrecognised units value: '" << units << "'");
		}
	}

	Object Width::evaluate(const xhtml::RenderContext& ctx) const
	{
		//if(is_auto_) {
		//	return Object(Length(0));
		//}
		//return Object(width_);
		return Object(*this);
	}

	xhtml::FixedPoint Length::compute(xhtml::FixedPoint scale) const
	{
		auto& ctx = xhtml::RenderContext::get();
		const int dpi = ctx.getDPI();
		xhtml::FixedPoint res = 0;
		switch(units_) {
			case LengthUnits::NUMBER:
				res = value_;
				break;
			case LengthUnits::PX:
				res = ((value_ * 3 * dpi) / (72 * 4));
				break;
			case LengthUnits::EM:
				res = static_cast<xhtml::FixedPoint>((ctx.getComputedValue(Property::FONT_SIZE).getValue<xhtml::FixedPoint>() / 72.0f) * (value_ * dpi));
				break;
			case LengthUnits::EX:
				res = static_cast<xhtml::FixedPoint>(ctx.getFontHandle()->getFontXHeight() / 72.0f * (value_ * dpi));
				break;
			case LengthUnits::IN:
				res = value_ * dpi;
				break;
			case LengthUnits::CM:
				res = (value_ * dpi * 254) / 100;
				break;
			case LengthUnits::MM:
				res = (value_ * dpi * 254) / 10;
				break;
			case LengthUnits::PT:
				res = (value_ * dpi) / 72;
				break;
			case LengthUnits::PC:
				res = (12 * value_ * dpi) / 72;
				break;
			case LengthUnits::PERCENT:
				res = (value_ / fixed_point_scale) * (scale / 100);
				break; 
			default: 
				ASSERT_LOG(false, "Unrecognised units value: " << static_cast<int>(units_));
				break;
		}
		return res;
	}

	Object Length::evaluate(const xhtml::RenderContext& ctx) const
	{
		return Object(*this);
	}

	Object FontSize::evaluate(const xhtml::RenderContext& ctx) const
	{
		float res = 0;
		xhtml::FixedPoint parent_fs = ctx.getComputedValue(Property::FONT_SIZE).getValue<xhtml::FixedPoint>();
		if(is_absolute_) {
			res = get_font_size_table(static_cast<float>(ctx.getDPI()))[static_cast<int>(absolute_)];
		} else if(is_relative_) {
			// XXX hack
			if(relative_ == FontSizeRelative::LARGER) {
				res = parent_fs * 1.15f;
			} else {
				res = parent_fs / 1.15f;
			}
		} else if(is_length_) {
			return Object(length_.compute(parent_fs));
		} else {
			ASSERT_LOG(false, "FontSize has no definite size defined!");
		}
		return Object(xhtml::FixedPoint(res * fixed_point_scale));
	}

	FontFamily::FontFamily() 
		: fonts_() 
	{ 
		fonts_.emplace_back("sans-serif");
	}

	Object BorderStyle::evaluate(const xhtml::RenderContext& rc) const
	{
		return Object(border_style_);
	}

	Object FontFamily::evaluate(const xhtml::RenderContext& rc) const
	{
		return Object(fonts_);
	}

	Object Float::evaluate(const xhtml::RenderContext& rc) const
	{
		return Object(float_);
	}

	Object Display::evaluate(const xhtml::RenderContext& rc) const
	{
		return Object(display_);
	}

	Object Whitespace::evaluate(const xhtml::RenderContext& rc) const
	{
		return Object(whitespace_);
	}

	Object FontStyle::evaluate(const xhtml::RenderContext& rc) const
	{
		return Object(fs_);
	}

	Object FontVariant::evaluate(const xhtml::RenderContext& rc) const
	{
		return Object(fv_);
	}

	Object FontWeight::evaluate(const xhtml::RenderContext& ctx) const
	{
		if(is_relative_) {
			int fw = ctx.getComputedValue(Property::FONT_WEIGHT).getValue<int>();
			if(relative_ == FontWeightRelative::BOLDER) {
				// bolder
				fw += 100;
			} else {
				// lighter
				fw -= 100;
			}
			if(fw > 900) {
				fw = 900;
			} else if(fw < 100) {
				fw = 100;
			}
			fw = (fw / 100) * 100;
			return Object(fw);
		}		
		return Object(weight_);
	}

	Object TextAlign::evaluate(const xhtml::RenderContext& rc) const
	{
		return Object(ta_);
	}

	Object Direction::evaluate(const xhtml::RenderContext& rc) const
	{
		return Object(dir_);
	}

	Object TextTransform::evaluate(const xhtml::RenderContext& rc) const
	{
		return Object(tt_);
	}

	Object Overflow::evaluate(const xhtml::RenderContext& ctx) const
	{
		return Object(overflow_);
	}

	Object Position::evaluate(const xhtml::RenderContext& rc) const
	{
		return Object(position_);
	}

	Object UriStyle::evaluate(const xhtml::RenderContext& rc) const
	{
		return Object(*this);
	}

	Object BackgroundRepeat::evaluate(const xhtml::RenderContext& rc) const
	{
		return Object(repeat_);
	}

	BackgroundPosition::BackgroundPosition() 
		: left_(0, true),
		  top_(0, true)
	{
	}

	void BackgroundPosition::setLeft(const Length& left) 
	{
		left_ = left;
	}

	void BackgroundPosition::setTop(const Length& top) 
	{
		top_ = top;
	}

	Object BackgroundPosition::evaluate(const xhtml::RenderContext& rc) const 
	{
		return Object(*this);
	}

	Object ListStyleType::evaluate(const xhtml::RenderContext& rc) const 
	{
		return Object(list_style_type_);
	}
}
