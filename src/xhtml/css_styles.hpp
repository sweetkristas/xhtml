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

#include <array>

#include "Color.hpp"
#include "xhtml_fwd.hpp"
#include "variant_object.hpp"

#define MAKE_FACTORY(classname)																\
	template<typename... T>																	\
	static std::shared_ptr<classname> create(T&& ... all) {									\
		return std::make_shared<classname>(std::forward<T>(all)...);						\
	}


namespace xhtml
{
	class RenderContext;
	typedef int FixedPoint;
}

namespace css
{
	typedef std::array<int,3> Specificity;

	inline bool operator==(const Specificity& lhs, const Specificity& rhs) {
		return lhs[0] == rhs[0] && lhs[1] == rhs[1] && lhs[2] == rhs[2];
	}
	inline bool operator<(const Specificity& lhs, const Specificity& rhs) {
		return lhs[0] == rhs[0] ? lhs[1] == rhs[1] ? lhs[2] < rhs[2] : lhs[1] < rhs[1] : lhs[0] < rhs[0];
	}
	inline bool operator<=(const Specificity& lhs, const Specificity& rhs) {
		return lhs == rhs || lhs < rhs;
	}

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

		// CSS3 provision properties
		BOX_SHADOW,
		TEXT_SHADOW,
		TRANSITION_PROPERTY,
		TRANSITION_DURATION,
		TRANSITION_TIMING_FUNCTION,
		TRANSITION_DELAY,
		BORDER_TOP_LEFT_RADIUS,
		BORDER_TOP_RIGHT_RADIUS,
		BORDER_BOTTOM_LEFT_RADIUS,
		BORDER_BOTTOM_RIGHT_RADIUS,
		BORDER_SPACING,
		OPACITY,
		BORDER_IMAGE_SOURCE,
		BORDER_IMAGE_SLICE,
		BORDER_IMAGE_WIDTH,
		BORDER_IMAGE_OUTSET,
		BORDER_IMAGE_REPEAT,

		MAX_PROPERTIES,
	};

	enum class Side {
		TOP,
		LEFT,
		BOTTOM, 
		RIGHT,
	};

	enum class CssColorParam {
		NONE,
		TRANSPARENT,
		VALUE,
		CURRENT,		// use current foreground color
	};

	// This is the basee class for all other styles.
	class Style
	{
	public:
		Style() : is_important_(false), is_inherited_(false) {}
		explicit Style(bool inh) : is_important_(false), is_inherited_(inh) {}
		virtual ~Style() {}
		virtual Object evaluate(const xhtml::RenderContext& rc) const { return Object(); }
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
		Object evaluate(const xhtml::RenderContext& rc) const override;
		KRE::Color compute() const;
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
		explicit Length(xhtml::FixedPoint value, bool is_percent=false) : value_(value), units_(is_percent ? LengthUnits::PERCENT : LengthUnits::NUMBER) {}
		explicit Length(xhtml::FixedPoint value, LengthUnits units) : value_(value), units_(units) {}
		explicit Length(xhtml::FixedPoint value, const std::string& units);
		bool isNumber() const { return units_ == LengthUnits::NUMBER; }
		bool isPercent() const { return units_ == LengthUnits::PERCENT; }
		bool isLength() const {  return units_ != LengthUnits::NUMBER && units_ != LengthUnits::PERCENT; }
		Object evaluate(const xhtml::RenderContext& rc) const override;
		xhtml::FixedPoint compute(xhtml::FixedPoint scale=65536) const;
	private:
		xhtml::FixedPoint value_;
		LengthUnits units_;
	};

	enum class AngleUnits {
		DEGREES,
		RADIANS,
		GRADIANS,
		TURNS,
	};

	class Angle
	{
	public:
		Angle() : value_(0), units_(AngleUnits::DEGREES) {}
		explicit Angle(float angle, AngleUnits units) : value_(angle), units_(units) {}
		explicit Angle(float angle, const std::string& units);		
		float getAngle(AngleUnits units=AngleUnits::DEGREES);
	private:
		float value_;
		AngleUnits units_;
	};

	class Width : public Style
	{
	public:
		Width() : is_auto_(false), width_() {}
		explicit Width(Length len) : is_auto_(false), width_(len) {}
		explicit Width(bool a) : is_auto_(a), width_() {}
		Object evaluate(const xhtml::RenderContext& rc) const override;	
		bool isAuto() const { return is_auto_; }
		const Length& getLength() const { return width_; }
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

	class UriStyle : public Style
	{
	public:
		MAKE_FACTORY(UriStyle);
		UriStyle() : is_none_(false), uri_() {}
		explicit UriStyle(bool none) : is_none_(none), uri_() {}
		explicit UriStyle(const std::string uri) : is_none_(false), uri_(uri) {}
		bool isNone() const { return is_none_; }
		const std::string& getUri() const { return uri_; }
		Object evaluate(const xhtml::RenderContext& rc) const override;
		void setURI(const std::string& uri) { uri_ = uri; is_none_ = false; }
	private:
		bool is_none_;
		std::string uri_;
	};

	struct BorderStyle : public Style
	{
		MAKE_FACTORY(BorderStyle);
		explicit BorderStyle(CssBorderStyle bs) : border_style_(bs) {}
		Object evaluate(const xhtml::RenderContext& rc) const override;
		CssBorderStyle border_style_;
	};

	class FontFamily : public Style
	{
	public:
		MAKE_FACTORY(FontFamily);
		FontFamily();
		explicit FontFamily(const std::vector<std::string>& fonts) : fonts_(fonts) {}
		Object evaluate(const xhtml::RenderContext& rc) const override;
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
		Object evaluate(const xhtml::RenderContext& rc) const override;
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
		Object evaluate(const xhtml::RenderContext& rc) const override;
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
		Object evaluate(const xhtml::RenderContext& rc) const override;
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
		Object evaluate(const xhtml::RenderContext& rc) const override;
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
		Object evaluate(const xhtml::RenderContext& rc) const override;
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
		Object evaluate(const xhtml::RenderContext& rc) const override;
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
		explicit FontWeight(int fw) { is_relative_ = false; weight_ = fw; }
		void setRelative(FontWeightRelative r) { is_relative_ = true; relative_ = r; }
		void setWeight(int fw) { is_relative_ = false; weight_ = fw; }
		Object evaluate(const xhtml::RenderContext& rc) const override;
	private:
		bool is_relative_;
		int weight_;
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
		Object evaluate(const xhtml::RenderContext& rc) const override;
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
		Object evaluate(const xhtml::RenderContext& rc) const override;
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
		Object evaluate(const xhtml::RenderContext& rc) const override;
		CssTextTransform tt_;
	};

	enum class CssOverflow {
		VISIBLE,
		HIDDEN,
		SCROLL,
		CLIP,
		AUTO,
	};

	struct Overflow : public Style
	{
		MAKE_FACTORY(Overflow);
		explicit Overflow(CssOverflow of) : overflow_(of) {}
		Object evaluate(const xhtml::RenderContext& rc) const override;
		CssOverflow overflow_;
	};

	enum class CssPosition {
		STATIC,
		RELATIVE,
		ABSOLUTE,
		FIXED,
	};

	struct Position : public Style
	{
		MAKE_FACTORY(Position);
		explicit Position(CssPosition p) : position_(p) {}
		Object evaluate(const xhtml::RenderContext& rc) const override;
		CssPosition position_;
	};

	enum class CssBackgroundRepeat {
		REPEAT,
		REPEAT_X,
		REPEAT_Y,
		NO_REPEAT,
	};

	struct BackgroundRepeat : public Style
	{
		MAKE_FACTORY(BackgroundRepeat);
		explicit BackgroundRepeat(CssBackgroundRepeat r) : repeat_(r) {}
		Object evaluate(const xhtml::RenderContext& rc) const override;
		CssBackgroundRepeat repeat_;
	};

	class BackgroundPosition : public Style
	{
	public:
		MAKE_FACTORY(BackgroundPosition);
		BackgroundPosition();
		void setLeft(const Length& left);
		void setTop(const Length& top);
		Object evaluate(const xhtml::RenderContext& rc) const override;
		const Length& getLeft() const { return left_; }
		const Length& getTop() const { return top_; }
	private:
		Length left_;
		Length top_;
	};

	enum class CssListStyleType {
		DISC,
		CIRCLE,
		SQUARE,
		DECIMAL,
		DECIMAL_LEADING_ZERO,
		LOWER_ROMAN,
		UPPER_ROMAN,
		LOWER_GREEK,
		LOWER_LATIN,
		UPPER_LATIN,
		ARMENIAN,
		GEORGIAN,
		LOWER_ALPHA,
		UPPER_ALPHA,
		NONE,
	};

	struct ListStyleType : public Style
	{
		MAKE_FACTORY(ListStyleType);
		ListStyleType() : list_style_type_(CssListStyleType::DISC) {}
		explicit ListStyleType(CssListStyleType lst) : list_style_type_(lst) {}
		Object evaluate(const xhtml::RenderContext& rc) const override;
		CssListStyleType list_style_type_;
	};

	enum class CssBackgroundAttachment {
		SCROLL,
		FIXED,
	};

	struct BackgroundAttachment : public Style
	{
		MAKE_FACTORY(BackgroundAttachment);
		BackgroundAttachment() : ba_(CssBackgroundAttachment::SCROLL) {}
		explicit BackgroundAttachment(CssBackgroundAttachment ba) : ba_(ba) {}
		Object evaluate(const xhtml::RenderContext& rc) const override { return Object(ba_); }
		CssBackgroundAttachment ba_;
	};

	enum class CssClear {
		NONE,
		LEFT,
		RIGHT,
		BOTH,
	};

	struct Clear : public Style
	{
		MAKE_FACTORY(Clear);
		Clear() : clr_(CssClear::NONE) {}
		explicit Clear(CssClear clr) : clr_(clr) {}
		Object evaluate(const xhtml::RenderContext& rc) const override { return Object(*this); }
		CssClear clr_;
	};

	class Clip : public Style
	{
	public:
		MAKE_FACTORY(Clip);
		Clip() : auto_(true), rect_() {}
		explicit Clip(xhtml::FixedPoint left, xhtml::FixedPoint top, xhtml::FixedPoint right, xhtml::FixedPoint bottom) 
			: auto_(false), 
			  rect_(left, top, right, bottom) 
		{
		}
		bool isAuto() const { return auto_; }
		const xhtml::Rect& getRect() const { return rect_; }
		void setRect(const xhtml::Rect& r) { rect_ = r; auto_ = false; }
		void setRect(xhtml::FixedPoint left, xhtml::FixedPoint top, xhtml::FixedPoint right, xhtml::FixedPoint bottom) { 
			rect_.x = left;
			rect_.y = top;
			rect_.width = right;
			rect_.height = bottom;
			auto_ = false; 
		}
		Object evaluate(const xhtml::RenderContext& rc) const override { return Object(*this); }
	private:
		bool auto_;
		xhtml::Rect rect_;
	};

	enum class CssContentType {
		STRING,
		URI,
		COUNTER,
		COUNTERS,
		OPEN_QUOTE,
		CLOSE_QUOTE,
		NO_OPEN_QUOTE,
		NO_CLOSE_QUOTE,
		ATTRIBUTE,
	};

	// encapsulates one kind of content.
	class ContentType
	{
	public:
		explicit ContentType(CssContentType type);
		explicit ContentType(CssContentType type, const std::string& name);
		explicit ContentType(CssListStyleType lst, const std::string& name);
		explicit ContentType(CssListStyleType lst, const std::string& name, const std::string& sep);
	private:
		CssContentType type_;
		std::string str_;
		std::string uri_;
		std::string counter_name_;
		std::string counter_seperator_;
		CssListStyleType counter_style_;
		std::string attr_;
	};

	class Content : public Style
	{
	public:
		MAKE_FACTORY(Content);
		Content() : content_() {}	// means none/normal
		explicit Content(const std::vector<ContentType>& content) : content_(content) {}
		void setContent(const std::vector<ContentType>& content) { content_ = content; }
	private:
		std::vector<ContentType> content_;
	};

	// used for incrementing and resetting.
	class Counter : public Style
	{
	public:
		MAKE_FACTORY(Counter);
		Counter() : counters_() {}
		explicit Counter(const std::vector<std::pair<std::string,int>>& counters) : counters_(counters) {}
		const std::vector<std::pair<std::string,int>>& getCounters() const { return counters_; }
		Object evaluate(const xhtml::RenderContext& rc) const override { return Object(*this); }
	private:
		std::vector<std::pair<std::string,int>> counters_;
	};

	enum class CssCursor {
		AUTO,
		CROSSHAIR,
		DEFAULT,
		POINTER,
		MOVE,
		E_RESIZE,
		NE_RESIZE,
		NW_RESIZE,
		N_RESIZE,
		SE_RESIZE,
		SW_RESIZE,
		S_RESIZE,
		W_RESIZE,
		TEXT,
		WAIT,
		PROGRESS,
		HELP,
	};

	class Cursor : public Style
	{
	public:
		MAKE_FACTORY(Cursor);
		Cursor() : uris_(), cursor_(CssCursor::AUTO) {}
		explicit Cursor(CssCursor c) : uris_(), cursor_(c) {}
		explicit Cursor(const std::vector<std::string>& uris, CssCursor c) : uris_(uris), cursor_(c) {}
		Object evaluate(const xhtml::RenderContext& rc) const override { return Object(*this); }
		void setURI(const std::vector<std::string>& uris) { uris_ = uris; }
		void setCursor(CssCursor c) { cursor_ = c; }
	private:
		std::vector<std::string> uris_;
		CssCursor cursor_;
	};

	enum class CssListStylePosition {
		INSIDE,
		OUTSIDE,
	};

	struct ListStylePosition : public Style
	{
		MAKE_FACTORY(ListStylePosition);
		ListStylePosition() : pos_(CssListStylePosition::OUTSIDE) {}
		explicit ListStylePosition(CssListStylePosition pos) : pos_(pos) {}
		Object evaluate(const xhtml::RenderContext& rc) const override { return Object(pos_); }
		CssListStylePosition pos_;
	};

	struct ListStyleImage : public Style
	{
		MAKE_FACTORY(ListStyleImage);
		ListStyleImage() : uri_() {}
		explicit ListStyleImage(const std::string& uri) : uri_(uri) {}
		Object evaluate(const xhtml::RenderContext& rc) const override { return Object(*this); }
		bool isNone() const { return uri_.empty(); }
		std::string uri_;
	};

	typedef std::pair<std::string, std::string> quote_pair;
	class Quotes : public Style
	{
	public:
		MAKE_FACTORY(Quotes);
		Quotes() : quotes_() {}
		explicit Quotes(const std::vector<quote_pair> quotes) : quotes_(quotes) {}
		bool isNone() const { return quotes_.empty(); }
		const std::vector<quote_pair>& getQuotes() const { return quotes_; }
		const quote_pair& getQuotesAtLevel(int n) { 
			if(n < 0) {
				static quote_pair no_quotes;
				return no_quotes;
			}
			if(n < static_cast<int>(quotes_.size())) {
				return quotes_[n];
			}
			return quotes_.back();
		};
		Object evaluate(const xhtml::RenderContext& rc) const override { return Object(*this); }
	private:
		std::vector<quote_pair> quotes_;
	};

	enum class CssTextDecoration {
		NONE,
		UNDERLINE,
		OVERLINE,
		LINE_THROUGH,
		BLINK, // N.B. We will not support blinking text.
	};

	struct TextDecoration : public Style
	{
		MAKE_FACTORY(TextDecoration);
		TextDecoration() : td_(CssTextDecoration::NONE) {}
		explicit TextDecoration(CssTextDecoration td) : td_(td) {}
		Object evaluate(const xhtml::RenderContext& rc) const override { return Object(td_); }
		CssTextDecoration td_;
	};

	enum class CssUnicodeBidi {
		NORMAL,
		EMBED,
		BIDI_OVERRIDE,
	};

	struct UnicodeBidi : public Style
	{
		MAKE_FACTORY(UnicodeBidi);
		UnicodeBidi() : bidi_(CssUnicodeBidi::NORMAL) {}
		explicit UnicodeBidi(CssUnicodeBidi bidi) : bidi_(bidi) {}
		Object evaluate(const xhtml::RenderContext& rc) const override { return Object(bidi_); }
		CssUnicodeBidi bidi_;
	};

	enum class CssVerticalAlign {
		BASELINE,
		SUB,
		SUPER,
		TOP,
		TEXT_TOP,
		MIDDLE,
		BOTTOM,
		TEXT_BOTTOM,
		
		LENGTH,
	};

	class VerticalAlign : public Style
	{
	public:
		MAKE_FACTORY(VerticalAlign);
		VerticalAlign() : va_(CssVerticalAlign::BASELINE), len_() {}
		explicit VerticalAlign(CssVerticalAlign va) : va_(va), len_() {}
		explicit VerticalAlign(Length len) : va_(CssVerticalAlign::LENGTH), len_(len) {}
		Object evaluate(const xhtml::RenderContext& rc) const override { return Object(*this); }
		void setAlign(CssVerticalAlign va) { va_ = va; }
		void setLength(const Length& len) { len_ = len; va_ = CssVerticalAlign::LENGTH; }
		const Length& getLength() const { return len_; }
		CssVerticalAlign getAlign() const { return va_; }
	private:
		CssVerticalAlign va_;
		Length len_;
	};

	enum class CssVisibility {
		VISIBLE,
		HIDDEN,
		COLLAPSE,
	};

	struct Visibility : public Style
	{
		MAKE_FACTORY(Visibility);
		Visibility() : visibility_(CssVisibility::VISIBLE) {}
		explicit Visibility(CssVisibility visibility) : visibility_(visibility) {}
		Object evaluate(const xhtml::RenderContext& rc) const override { return Object(visibility_); }
		CssVisibility visibility_;
	};

	class Zindex : public Style
	{
	public:
		MAKE_FACTORY(Zindex);
		Zindex() : auto_(true), index_(0) {}
		explicit Zindex(int n) : auto_(false), index_(n) {}
		Object evaluate(const xhtml::RenderContext& rc) const override { return Object(*this); }
		void setIndex(int index) { index_ = index; auto_ = false; }
		bool isAuto() const { return auto_; }
		int getIndex() const { return index_; }
	private:
		bool auto_;
		int index_;
	};

	class BoxShadow
	{
	public:
		BoxShadow();
		explicit BoxShadow(bool inset, const Length& x, const Length& y, const Length& blur, const Length& spread, const CssColor& color);
		bool inset() const { return inset_; }
		const Length& getX() const { return x_offset_; }
		const Length& getY() const { return y_offset_; }
		const Length& getBlur() const { return blur_radius_; }
		const Length& getSpread() const { return spread_radius_; }
		const CssColor& getColor() const { return color_; }
	private:
		bool inset_;
		Length x_offset_;
		Length y_offset_;
		Length blur_radius_;
		Length spread_radius_;
		CssColor color_;
	};

	class BoxShadowStyle : public Style
	{
	public:
		MAKE_FACTORY(BoxShadowStyle);
		BoxShadowStyle() : shadows_() {}
		BoxShadowStyle(const std::vector<BoxShadow>& shadows) : shadows_(shadows) {}
		void setShadows(const std::vector<BoxShadow>& shadows) { shadows_ = shadows; }
		const std::vector<BoxShadow>& getShadows() const { return shadows_; }
		Object evaluate(const xhtml::RenderContext& rc) const override { return Object(*this); }
	private:
		std::vector<BoxShadow> shadows_;
	};

	enum class CssBorderImageRepeat {
		STRETCH,
		REPEAT,
		ROUND,
		SPACE,
	};
	
	struct BorderImageRepeat : public Style
	{
		MAKE_FACTORY(BorderImageRepeat);
		BorderImageRepeat() : image_repeat_horiz_(CssBorderImageRepeat::STRETCH), image_repeat_vert_(CssBorderImageRepeat::STRETCH) {}
		explicit BorderImageRepeat(CssBorderImageRepeat image_repeat_h, CssBorderImageRepeat image_repeat_v) : image_repeat_horiz_(image_repeat_h), image_repeat_vert_(image_repeat_v) {}
		Object evaluate(const xhtml::RenderContext& rc) const override { return Object(*this); }
		CssBorderImageRepeat image_repeat_horiz_;
		CssBorderImageRepeat image_repeat_vert_;
	};

	class WidthList : public Style
	{
	public:
		MAKE_FACTORY(WidthList);
		WidthList() : widths_{} {}
		explicit WidthList(float value);
		explicit WidthList(const std::vector<Width>& widths);
		void setWidths(const std::vector<Width>& widths);
		const std::array<Width,4>& getWidths() const { return widths_; }
		const Width& getTop() const { return widths_[0]; }
		const Width& getLeft() const { return widths_[1]; }
		const Width& getBottom() const { return widths_[2]; }
		const Width& getRight() const { return widths_[3]; }
		Object evaluate(const xhtml::RenderContext& rc) const override { return Object(*this); }
	private:
		std::array<Width,4> widths_;
	};

	class BorderImageSlice : public Style
	{
	public:
		MAKE_FACTORY(BorderImageSlice);
		BorderImageSlice() : slices_{}, fill_(false) {}
		explicit BorderImageSlice(const std::vector<Width>& widths, bool fill);
		bool isFilled() const { return fill_; }
		void setWidths(const std::vector<Width>& widths);
		const std::array<Width,4>& getWidths() const { return slices_; }
		Object evaluate(const xhtml::RenderContext& rc) const override { return Object(*this); }
	private:
		std::array<Width,4> slices_;
		bool fill_;
	};

	class BorderRadius : public Style
	{
	public:
		MAKE_FACTORY(BorderRadius);
		BorderRadius() : horiz_(0), vert_(0) {}
		explicit BorderRadius(const Length& horiz, const Length& vert) : horiz_(horiz), vert_(vert) {}
		const Length& getHoriz() const { return horiz_; }
		const Length& getVert() const { return vert_; }
		Object evaluate(const xhtml::RenderContext& rc) const override { return Object(*this); }
	private:
		Length horiz_;
		Length vert_;
	};

	enum class CssBorderClip {
		BORDER_BOX,
		PADDING_BOX,
		CONTENT_BOX,
	};

	struct BorderClip : public Style
	{
		MAKE_FACTORY(BorderClip);
		BorderClip() : border_clip_(CssBorderClip::BORDER_BOX) {}
		explicit BorderClip(CssBorderClip border_clip) : border_clip_(border_clip) {}
		Object evaluate(const xhtml::RenderContext& rc) const override { return Object(border_clip_); }
		CssBorderClip border_clip_;
	};

	class LinearGradient : public Style
	{
	public:
		MAKE_FACTORY(LinearGradient);
		LinearGradient() : angle_(0), color_stops_() {}
		struct ColorStop {
			ColorStop() : color(), length() {}
			explicit ColorStop(const std::shared_ptr<CssColor>& c, const Length& l) : color(c), length(l) {}
			std::shared_ptr<CssColor> color;
			Length length;
		};
		void setAngle(float angle) { angle_ = angle; }
		void clearColorStops() { color_stops_.clear(); }
		void addColorStop(const ColorStop& cs) { color_stops_.emplace_back(cs); }
		const std::vector<ColorStop>& getColorStops() const { return color_stops_; }
		Object evaluate(const xhtml::RenderContext& rc) const override { return Object(*this); }		
	private:
		float angle_;	// in degrees
		std::vector<ColorStop> color_stops_;
	};
}
