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
#include "variant_object.hpp"

#define MAKE_FACTORY(classname)																\
	template<typename... T>																	\
	static std::shared_ptr<classname> create(T&& ... all) {									\
		return std::make_shared<classname>(std::forward<T>(all)...);						\
	}


namespace xhtml
{
	class RenderContext;
}

namespace css
{
	enum class Property {
		BACKGROUND_ATTACHMENT,
		BACKGROUND_COLOR,
		BACKGROUND_IMAGE,
		BACKGROUND_POSITION,
		BACKGROUND_REPEAT,
		BORDER_COLLAPSE,
		BORDER_TOP_COLOR,
		BORDER_LEFT_COLOR,
		BORDER_BOTTOM_COLOR,
		BORDER_RIGHT_COLOR,
		BORDER_TOP_STYLE,
		BORDER_LEFT_STYLE,
		BORDER_BOTTOM_STYLE,
		BORDER_RIGHT_STYLE,
		BORDER_TOP_WIDTH,
		BORDER_LEFT_WIDTH,
		BORDER_BOTTOM_WIDTH,
		BORDER_RIGHT_WIDTH,
		BOTTOM,
		CAPTION_SIDE,
		CLEAR,
		CLIP,
		COLOR,
		CONTENT,
		COUNTER_INCREMENT,
		COUNTER_RESET,
		CURSOR,
		DIRECTION,
		DISPLAY,
		EMPTY_CELLS,
		FLOAT,
		FONT_FAMILY,
		FONT_SIZE,
		FONT_STYLE,
		FONT_VARIANT,
		FONT_WEIGHT,
		HEIGHT,
		LEFT,
		LETTER_SPACING,
		LINE_HEIGHT,
		LIST_STYLE_IMAGE,
		LIST_STYLE_POSITION,
		LIST_STYLE_TYPE,
		MARGIN_TOP,
		MARGIN_LEFT,
		MARGIN_BOTTOM,
		MARGIN_RIGHT,
		MAX_HEIGHT,
		MAX_WIDTH,
		MIN_HEIGHT,
		MIN_WIDTH,
		ORPHANS,
		OUTLINE_COLOR,
		OUTLINE_STYLE,
		OUTLINE_WIDTH,
		CSS_OVERFLOW,		// had to decorate the name because it clashes with a #define
		PADDING_TOP,
		PADDING_LEFT,
		PADDING_RIGHT,
		PADDING_BOTTOM,
		POSITION,
		QUOTES,
		RIGHT,
		TABLE_LAYOUT,
		TEXT_ALIGN,
		TEXT_DECORATION,
		TEXT_INDENT,
		TEXT_TRANSFORM,
		TOP,
		UNICODE_BIDI,
		VERTICAL_ALIGN,
		VISIBILITY,
		WHITE_SPACE,
		WIDOWS,
		WIDTH,
		WORD_SPACING,
		Z_INDEX,
		MAX_PROPERTIES,
	};

	enum class CssColorParam {
		NONE,
		TRANSPARENT,
		VALUE,
	};

	// This is the basee class for all other styles.
	class Style
	{
	public:
		Style() : is_important_(false), is_inherited_(false) {}
		explicit Style(bool inh) : is_important_(false), is_inherited_(inh) {}
		virtual ~Style() {}
		virtual Object evaluate(Property p, const xhtml::RenderContext& rc) const { return Object(); }
		void setImportant(bool imp=true) { is_important_ = imp; }
		void setInherited(bool inh=true) { is_inherited_ = inh; }
		bool isImportant() const { return is_important_; }
		bool isInherited() const { return is_inherited_; }
	private:
		bool is_important_;
		bool is_inherited_;
	};
	typedef std::shared_ptr<Style> StylePtr;

	class CssColor : public Style
	{
	public:
		MAKE_FACTORY(CssColor);
		CssColor();
		explicit CssColor(CssColorParam param, const KRE::Color& color=KRE::Color::colorWhite());
		void setParam(CssColorParam param);
		void setColor(const KRE::Color& color);
		CssColorParam getParam() const { return param_; }
		const KRE::Color& getColor() const { return color_; }	
		bool isTransparent() const { return param_ == CssColorParam::TRANSPARENT; }
		bool isNone() const { return param_ == CssColorParam::NONE; }
		bool isValue() const { return param_ == CssColorParam::VALUE; }
		Object evaluate(Property p, const xhtml::RenderContext& rc) const override;
	private:
		CssColorParam param_;
		KRE::Color color_;
	};

	enum class LengthUnits {
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
	
	class Length : public Style
	{
	public:
		MAKE_FACTORY(Length);
		Length() : value_(0), units_(LengthUnits::NUMBER) {}
		explicit Length(double value, bool is_percent=false) : value_(value), units_(is_percent ? LengthUnits::PERCENT : LengthUnits::NUMBER) {}
		explicit Length(double value, LengthUnits units) : value_(value), units_(units) {}
		explicit Length(double value, const std::string& units);
		bool isNumber() const { return units_ == LengthUnits::NUMBER; }
		bool isPercent() const { return units_ == LengthUnits::PERCENT; }
		bool isLength() const {  return units_ != LengthUnits::NUMBER && units_ != LengthUnits::PERCENT; }
		Object evaluate(Property p, const xhtml::RenderContext& rc) const override;
	private:
		double value_;
		LengthUnits units_;
	};

	class Width : public Style
	{
	public:
		explicit Width(Length len) : is_auto_(false), width_(len) {}
		explicit Width(bool a) : is_auto_(a), width_() {}
		Object evaluate(Property p, const xhtml::RenderContext& rc) const override;	
		bool isAuto() const { return is_auto_; }
	private:
		bool is_auto_;
		Length width_;
	};

	enum class CssBorderStyle {
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

	struct BorderStyle : public Style
	{
		MAKE_FACTORY(BorderStyle);
		explicit BorderStyle(CssBorderStyle bs) : border_style_(bs) {}
		Object evaluate(Property p, const xhtml::RenderContext& rc) const override;
		CssBorderStyle border_style_;
	};

	class FontFamily : public Style
	{
	public:
		MAKE_FACTORY(FontFamily);
		FontFamily();
		explicit FontFamily(const std::vector<std::string>& fonts) : fonts_(fonts) {}
		Object evaluate(Property p, const xhtml::RenderContext& rc) const override;
	private:
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

	class FontSize : public Style
	{
	public:
		MAKE_FACTORY(FontSize);
		FontSize() 
			: is_absolute_(false), 
			  absolute_(FontSizeAbsolute::NONE), 
			  is_relative_(false), 
			  relative_(FontSizeRelative::NONE), 
			  is_length_(false), 
			  length_()
		{
		}
		explicit FontSize(FontSizeAbsolute absvalue) 
			: is_absolute_(true), 
			  absolute_(absvalue), 
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
		void setFontSize(const Length& len) { 
			disableAll();
			length_ = len;
			is_length_ = true;
		}
		Object evaluate(Property p, const xhtml::RenderContext& rc) const override;
	private:
		bool is_absolute_;
		FontSizeAbsolute absolute_;
		bool is_relative_;
		FontSizeRelative relative_;
		bool is_length_;
		Length length_;
		void disableAll() { is_absolute_= false; is_relative_ = false; is_length_ = false; }
	};

	enum class CssFloat {
		NONE,
		LEFT,
		RIGHT,
	};
	
	struct Float : public Style
	{
		MAKE_FACTORY(Float);
		explicit Float(CssFloat f) : float_(f) {}
		Object evaluate(Property p, const xhtml::RenderContext& rc) const override;
		CssFloat float_;
	};

	enum class CssDisplay {
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

	struct Display : public Style
	{
		MAKE_FACTORY(Display);
		explicit Display(CssDisplay d) : display_(d) {}
		Object evaluate(Property p, const xhtml::RenderContext& rc) const override;
		CssDisplay display_;
	};

	enum class CssWhitespace {
		NORMAL,
		PRE,
		NOWRAP,
		PRE_WRAP,
		PRE_LINE,
	};

	struct Whitespace : public Style
	{
		MAKE_FACTORY(Whitespace);
		explicit Whitespace(CssWhitespace ws) : whitespace_(ws) {}
		Object evaluate(Property p, const xhtml::RenderContext& rc) const override;
		CssWhitespace whitespace_;
	};

	enum class CssFontStyle {
		NORMAL,
		ITALIC,
		OBLIQUE,
	};

	struct FontStyle : public Style
	{
		MAKE_FACTORY(FontStyle);
		explicit FontStyle(CssFontStyle fs) : fs_(fs) {}
		Object evaluate(Property p, const xhtml::RenderContext& rc) const override;
		CssFontStyle fs_;
	};

	enum class CssFontVariant {
		NORMAL,
		SMALL_CAPS,
	};

	struct FontVariant : public Style
	{
		MAKE_FACTORY(FontVariant);
		explicit FontVariant(CssFontVariant fv) : fv_(fv) {}
		Object evaluate(Property p, const xhtml::RenderContext& rc) const override;
		CssFontVariant fv_;
	};

	enum class FontWeightRelative {
		LIGHTER,
		BOLDER,
	};

	class FontWeight : public Style
	{
	public:
		MAKE_FACTORY(FontWeight);
		FontWeight() : is_relative_(false), weight_(400), relative_(FontWeightRelative::LIGHTER) {}
		explicit FontWeight(FontWeightRelative r) { is_relative_ = true; relative_ = r; }
		explicit FontWeight(double fw) { is_relative_ = false; weight_ = fw; }
		void setRelative(FontWeightRelative r) { is_relative_ = true; relative_ = r; }
		void setWeight(double fw) { is_relative_ = false; weight_ = fw; }
		Object evaluate(Property p, const xhtml::RenderContext& rc) const override;
	private:
		bool is_relative_;
		double weight_;
		FontWeightRelative relative_;
	};

	enum class CssTextAlign {
		// normal is the default value that acts as 'left' if direction=ltr and
		// 'right' if direction='rtl'.
		NORMAL,
		LEFT,
		RIGHT,
		CENTER,
		JUSTIFY,
	};

	struct TextAlign : public Style
	{
		MAKE_FACTORY(TextAlign);
		explicit TextAlign(CssTextAlign ta) : ta_(ta) {}
		Object evaluate(Property p, const xhtml::RenderContext& rc) const override;
		CssTextAlign ta_;
	};

	enum class CssDirection {
		LTR,
		RTL,
	};

	struct Direction : public Style
	{
		MAKE_FACTORY(Direction);
		explicit Direction(CssDirection dir) : dir_(dir) {}
		Object evaluate(Property p, const xhtml::RenderContext& rc) const override;
		CssDirection dir_;
	};

	enum class CssTextTransform {
		NONE,
		CAPITALIZE,
		UPPERCASE,
		LOWERCASE,
	};

	struct TextTransform : public Style
	{
		MAKE_FACTORY(TextTransform);
		explicit TextTransform(CssTextTransform tt) : tt_(tt) {}
		Object evaluate(Property p, const xhtml::RenderContext& rc) const override;
		CssTextTransform tt_;
	};

}
