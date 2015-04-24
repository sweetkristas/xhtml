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

namespace css
{
	namespace 
	{
		const double pixels_per_inch = 96.0;

		std::vector<int>& get_font_size_table(int ppi)
		{
			static std::vector<int> res;
			if(res.empty()) {
				// First guess implementation.
				double min_size = 9.0 / 72.0 * ppi;
				res.emplace_back(static_cast<int>(min_size));
				res.emplace_back(static_cast<int>(std::ceil(min_size * 1.1)));
				res.emplace_back(static_cast<int>(std::ceil(min_size * 1.3)));
				res.emplace_back(static_cast<int>(std::ceil(min_size * 1.45)));
				res.emplace_back(static_cast<int>(std::ceil(min_size * 1.6)));
				res.emplace_back(static_cast<int>(std::ceil(min_size * 1.8)));
				res.emplace_back(static_cast<int>(std::ceil(min_size * 2.0)));
				res.emplace_back(static_cast<int>(std::ceil(min_size * 2.3)));
			}
			return res;
		}
	}

	CssColor::CssColor()
		: param_(ColorParam::VALUE),
		  color_(KRE::Color::colorWhite())
	{
	}

	CssColor::CssColor(ColorParam param, const KRE::Color& color)
		: param_(param),
		  color_(color)
	{
	}

	void CssColor::setParam(ColorParam param)
	{
		param_ = param;
	}

	void CssColor::setColor(const KRE::Color& color)
	{
		color_ = color;
		setParam(ColorParam::VALUE);
	}

	CssLength::CssLength(double value, const std::string& units) 
		: value_(value), 
		units_(CssLengthUnits::NUMBER) 
	{
		if(units == "em") {
			units_ = CssLengthUnits::EM;
		} else if(units == "ex") {
			units_ = CssLengthUnits::EX;
		} else if(units == "in") {
			units_ = CssLengthUnits::IN;
		} else if(units == "cm") {
			units_ = CssLengthUnits::CM;
		} else if(units == "mm") {
			units_ = CssLengthUnits::MM;
		} else if(units == "pt") {
			units_ = CssLengthUnits::PT;
		} else if(units == "pc") {
			units_ = CssLengthUnits::PC;
		} else if(units == "px") {
			units_ = CssLengthUnits::PX;
		} else if(units == "%") {
			units_ = CssLengthUnits::PERCENT;
			// normalize to range 0.0 -> 1.0
			value_ /= 100.0;
		} else {
			LOG_ERROR("unrecognised units value: '" << units << "'");
		}
	}

	CssLength::CssLength(CssLengthParam param)
		: param_(param), 
		  value_(0), 
		  units_(CssLengthUnits::NUMBER)
	{
	}

	double CssLength::evaluate(double length, const xhtml::RenderContext* ctx) const
	{
		// auto values evaluate as 0
		if(isAuto()) {
			return 0;
		}
		switch(units_) {
			case CssLengthUnits::NUMBER:
			case CssLengthUnits::PX:
				return value_ * pixels_per_inch / 72.0 * 0.75;
			case CssLengthUnits::EM:
				return ctx->getFontSize() * value_ * pixels_per_inch / 72.0;
			case CssLengthUnits::EX:
				// fudge
				return ctx->getFontXHeight() * value_ * pixels_per_inch / 72.0;
			case CssLengthUnits::IN:
				return value_ * pixels_per_inch;
			case CssLengthUnits::CM:
				return value_ * pixels_per_inch * 2.54;
			case CssLengthUnits::MM:
				return value_ * pixels_per_inch * 25.4;
			case CssLengthUnits::PT:
				return value_ * pixels_per_inch / 72.0;
			case CssLengthUnits::PC:
				return 12.0 * value_ * pixels_per_inch / 72.0;
			case CssLengthUnits::PERCENT:
				return value_* length;
			default: break;
		}
		ASSERT_LOG(false, "Unrecognised units value: " << static_cast<int>(units_));
		return value_;
	}

	Border::Border() 
		: style_(BorderStyle::NONE), 
		  color_(), 
		  width_(4.0) 
	{
	}

	void Border::setWidth(const CssLength& len)
	{
		width_ = len;
	}

	void Border::setColor(const CssColor& color)
	{
		color_ = color;
	}

	void Border::setStyle(BorderStyle style)
	{
		style_ = style;	
	}

	FontFamily::FontFamily() 
		: inherit_(true), 
		  fonts_() 
	{ 
		fonts_.emplace_back("sans-serif");
	}
}
