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

#include "Color.hpp"

namespace css
{
	enum class ColorParam {
		NONE,
		INHERIT,
		TRANSPARENT,
		VALUE,
	};

	class CssColor
	{
	public:
		CssColor();
		explicit CssColor(ColorParam param, const KRE::Color& color=KRE::Color::colorWhite());
		void setParam(ColorParam param);
		void setColor(const KRE::Color& color);
	private:
		ColorParam param_;
		KRE::Color color_;
	};

	enum class CssLengthUnits {
		NUMBER,		// Plain old number
		EM,			// Computed value of the font-size property
		EX,			// Computed height of lowercase 'x'
		IN,			// Inches
		CM,			// Centimeters
		MM,			// Millimeters
		PT,			// Point size, equal to 1/72 of an inch
		PC,			// Picas. 1 pica = 12pt
		PX,			// Pixels. 1px = 0.75pt
		PERCENT		// percent value
	};

	enum class CssLengthParam {
		VALUE,
		AUTO,
		INHERIT,
	};

	class CssLength
	{
	public:
		CssLength() : param_(CssLengthParam::VALUE), value_(0), units_(CssLengthUnits::NUMBER) {}
		explicit CssLength(double value, bool is_percent=false) : param_(CssLengthParam::VALUE), value_(value), units_(is_percent ? CssLengthUnits::PERCENT : CssLengthUnits::NUMBER) {}
		explicit CssLength(double value, CssLengthUnits units) : param_(CssLengthParam::VALUE), value_(value), units_(units) {}
		explicit CssLength(double value, const std::string& units);
		explicit CssLength(CssLengthParam param);
	private:
		CssLengthParam param_;
		double value_;
		CssLengthUnits units_;
	};

	enum class BorderStyle {
		INHERIT,
		NONE,
		HIDDEN,
		DOTTED,
		DASHED,
		SOLID,
		DOUBLE,
		GROOVE,
		RIDGE,
		INSET,
		OUTSET,
	};

	class Border
	{
	public:
		Border();
		void setWidth(const CssLength& len);
		void setColor(const CssColor& color);
		void setStyle(BorderStyle style);
	private:
		BorderStyle style_;
		CssColor color_;
		CssLength width_;
	};

	class FontFamily
	{
	public:
		FontFamily();
		explicit FontFamily(const std::vector<std::string>& fonts) : inherit_(false), fonts_(fonts) {}
	private:
		bool inherit_;
		std::vector<std::string> fonts_;
	};

	enum class FontSizeAbsolute {
		NONE,
		XX_SMALL,
		X_SMALL,
		SMALL,
		MEDIUM,
		LARGE,
		X_LARGE,
		XX_LARGE,
		XXX_LARGE,
	};

	enum class FontSizeRelative {
		NONE,
		LARGER,
		SMALLER,
	};

	class FontSize
	{
	public:
		FontSize() 
			: inherit_(true), 
			  is_absolute_(false), 
			  absolute_(FontSizeAbsolute::NONE), 
			  is_relative_(false), 
			  relative_(FontSizeRelative::NONE), 
			  is_length_(false), 
			  length_()
		{
		}
		void setFontSize(FontSizeAbsolute absvalue) { 
			disableAll();
			absolute_ = absvalue;
			is_absolute_ = true;
		}
		void setFontSize(FontSizeRelative rel) { 
			disableAll();
			relative_ = rel;
			is_relative_ = true;
		}
		void setFontSize(const CssLength& len) { 
			disableAll();
			length_ = len;
			is_length_ = true;
		}
	private:
		bool inherit_;
		bool is_absolute_;
		FontSizeAbsolute absolute_;
		bool is_relative_;
		FontSizeRelative relative_;
		bool is_length_;
		CssLength length_;
		void disableAll() { inherit_ = false; is_absolute_= false; is_relative_ = false; is_length_ = false; }
	};

	enum class CssFloat {
		INHERIT,
		NONE,
		LEFT,
		RIGHT,
	};

	enum class CssDisplay {
		INHERIT,
		NONE,
		INLINE,
		BLOCK,
		LIST_ITEM,
		INLINE_BLOCK,
		TABLE,
		INLINE_TABLE,
		TABLE_ROW_GROUP,
		TABLE_HEADER_GROUP,
		TABLE_FOOTER_GROUP,
		TABLE_ROW,
		TABLE_COLUMN_GROUP,
		TABLE_COLUMN,
		TABLE_CELL,
		TABLE_CAPTION,
	};

	struct CssAttributes
	{
		CssAttributes();
		CssLength margin_left_;
		CssLength margin_top_;
		CssLength margin_right_;
		CssLength margin_bottom_;
		CssLength padding_left_;
		CssLength padding_top_;
		CssLength padding_right_;
		CssLength padding_bottom_;
		Border border_left_;
		Border border_top_;
		Border border_right_;
		Border border_bottom_;
		CssColor background_color_;
		CssColor color_;
		FontFamily font_family_;
		FontSize font_size_;
		CssFloat float_;
		CssDisplay display_;
	};
}
