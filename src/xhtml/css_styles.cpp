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

	CssStyles::CssStyles() 
		: margin_left_(0.0),
		  margin_top_(0.0),
		  margin_right_(0.0),
		  margin_bottom_(0.0),
		  padding_left_(0.0),
		  padding_top_(0.0),
		  padding_right_(0.0),
		  padding_bottom_(0.0),
		  border_left_(),
		  border_top_(),
		  border_right_(),
		  border_bottom_(),
		  background_color_(ColorParam::TRANSPARENT),
		  color_(),
		  font_family_(),
		  font_size_(),
		  float_(CssFloat::INHERIT),
		  display_(CssDisplay::INLINE)
	{
	}

}
