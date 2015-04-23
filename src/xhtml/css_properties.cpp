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
#include "formatter.hpp"
#include "css_parser.hpp"
#include "css_properties.hpp"

namespace css
{
	namespace 
	{
		const double border_width_thin = 2.0;
		const double border_width_medium = 4.0;
		const double border_width_thick = 10.0;

		typedef std::map<std::string, property_parse_function> property_map;
		property_map& get_property_table()
		{
			static property_map res;
			return res;
		}

		KRE::Color hsla_to_color(double h, double s, double l, double a)
		{
			const float hue_upper_limit = 360.0;
			double c = 0.0, m = 0.0, x = 0.0;
			c = (1.0 - std::abs(2 * l - 1.0)) * s;
			m = 1.0 * (l - 0.5 * c);
			x = c * (1.0 - std::abs(std::fmod(h / 60.0, 2) - 1.0));
			if (h >= 0.0 && h < (hue_upper_limit / 6.0)) {
				return KRE::Color(static_cast<float>(c+m), static_cast<float>(x+m), static_cast<float>(m), static_cast<float>(a));
			} else if (h >= (hue_upper_limit / 6.0) && h < (hue_upper_limit / 3.0)) {
				return KRE::Color(static_cast<float>(x+m), static_cast<float>(c+m), static_cast<float>(m), static_cast<float>(a));
			} else if (h < (hue_upper_limit / 3.0) && h < (hue_upper_limit / 2.0)) {
				return KRE::Color(static_cast<float>(m), static_cast<float>(c+m), static_cast<float>(x+m), static_cast<float>(a));
			} else if (h >= (hue_upper_limit / 2.0) && h < (2.0f * hue_upper_limit / 3.0)) {
				return KRE::Color(static_cast<float>(m), static_cast<float>(x+m), static_cast<float>(c+m), static_cast<float>(a));
			} else if (h >= (2.0 * hue_upper_limit / 3.0) && h < (5.0 * hue_upper_limit / 6.0)) {
				return KRE::Color(static_cast<float>(x+m), static_cast<float>(m), static_cast<float>(c+m), static_cast<float>(a));
			} else if (h >= (5.0 * hue_upper_limit / 6.0) && h < hue_upper_limit) {
				return KRE::Color(static_cast<float>(c+m), static_cast<float>(m), static_cast<float>(x+m), static_cast<float>(a));
			}
			return KRE::Color(static_cast<float>(m), static_cast<float>(m), static_cast<float>(m), static_cast<float>(a));
		}

		struct PropertyRegistrar
		{
			PropertyRegistrar(const std::string& name, property_parse_function fn) {
				get_property_table()[name] = fn;
			}
		};

		void skip_whitespace(std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end)
		{
			while(it != end && (*it)->id() == TokenId::WHITESPACE) {
				++it;
			}
		}

		TokenId get_token_id(std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end)
		{
			if(it == end) {
				return TokenId::EOF_TOKEN;
			}
			return (*it)->id();
		}

		std::vector<TokenPtr> parse_csv_list(std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, TokenId end_token)
		{
			std::vector<TokenPtr> res;
			while(it != end && (*it)->id() != end_token) {
				skip_whitespace(it, end);
				res.emplace_back(*it++);
				skip_whitespace(it, end);
				if((*it)->id() == TokenId::COMMA) {
					++it;
				} else if((*it)->id() != end_token && it != end) {
					throw ParserError("Expected ',' (COMMA) while parsing color value.");
				}
			}
			if((*it)->id() == end_token) {
				++it;
			}
			return res;
		}

		void parse_csv_number_list(std::vector<TokenPtr>::const_iterator& it, 
			std::vector<TokenPtr>::const_iterator end, 
			TokenId end_token, 
			std::function<void(int,double,bool)> fn)
		{
			auto toks = parse_csv_list(it, end, end_token);
			int n = 0;
			for(auto& t : toks) {
				if(t->id() == TokenId::PERCENT) {
					fn(n, t->getNumericValue(), true);
				} else if(t->id() == TokenId::NUMBER) {
					fn(n, t->getNumericValue(), false);
				} else {
					throw ParserError("Expected percent or numeric value while parsing numeric list.");
				}
				++n;
			}
		}

		// Helper function
		CssColor parse_a_color_value(std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end)
		{
			CssColor color;
			if((*it)->id() == TokenId::IDENT) {
				const std::string& ref = (*it)->getStringValue();
				++it;
				if(ref == "transparent") {
					color.setParam(ColorParam::TRANSPARENT);
				} else if(ref == "inherit") {
					color.setParam(ColorParam::INHERIT);
				} else {
					// color value is in ref.
					color.setColor(KRE::Color(ref));
				}
			} else if((*it)->id() == TokenId::FUNCTION) {
				const std::string& ref = (*it++)->getStringValue();
				if(ref == "rgb") {
					int values[3] = { 255, 255, 255 };
					parse_csv_number_list(it, end, TokenId::RPAREN, [&values](int n, double value, bool is_percent) {
						if(n < 3) {
							if(is_percent) {
								value *= 255.0 / 100.0;
							}
							values[n] = std::min(255, std::max(0, static_cast<int>(value)));
						}
					});
					color.setColor(KRE::Color(values[0], values[1], values[2]));
				} else if(ref == "rgba") {
					int values[4] = { 255, 255, 255, 255 };
					parse_csv_number_list(it, end, TokenId::RPAREN, [&values](int n, double value, bool is_percent) {
						if(n < 4) {
							if(is_percent) {
								value *= 255.0 / 100.0;
							}
							values[n] = std::min(255, std::max(0, static_cast<int>(value)));
						}
					});
					color.setColor(KRE::Color(values[0], values[1], values[2], values[3]));
				} else if(ref == "hsl") {
					double values[3];
					const double multipliers[3] = { 360.0, 1.0, 1.0 };
					parse_csv_number_list(it, end, TokenId::RPAREN, [&values, &multipliers](int n, double value, bool is_percent) {
						if(n < 3) {
							if(is_percent) {
								value *= multipliers[n] / 100.0;
							}
							values[n] = value;
						}
					});					
					color.setColor(hsla_to_color(values[0], values[1], values[2], 1.0));
				} else if(ref == "hsla") {
					double values[4];
					const double multipliers[4] = { 360.0, 1.0, 1.0, 1.0 };
					parse_csv_number_list(it, end, TokenId::RPAREN, [&values, &multipliers](int n, double value, bool is_percent) {
						if(n < 4) {
							if(is_percent) {
								value *= multipliers[n] / 100.0;
							}
							values[n] = value;
						}
					});					
					color.setColor(hsla_to_color(values[0], values[1], values[2], values[3]));
				}
			} else if((*it)->id() == TokenId::HASH) {
				const std::string& ref = (*it)->getStringValue();
				// is #fff or #ff00ff representation
				color.setColor(KRE::Color(ref));
				++it;
			}
			return color;
		}

		void parse_background_color(std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist)
		{
			(*plist)[CssProperty::BACKGROUND_COLOR] = Object(parse_a_color_value(it, end));			
		}
		PropertyRegistrar background_color("background-color", parse_background_color);

		void parse_color(std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist)
		{
			(*plist)[CssProperty::COLOR] = Object(parse_a_color_value(it, end));
		}
		PropertyRegistrar color("color", parse_color);

		Object parse_width(std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end)
		{
			skip_whitespace(it, end);
			if((*it)->id() == TokenId::IDENT) {
				const std::string ref = (*it)->getStringValue();
				if(ref == "inherit") {
					++it;
					return Object(CssLength(CssLengthParam::INHERIT));
				} else if(ref == "auto") {
					++it;
					return Object(CssLength(CssLengthParam::AUTO));
				}
			} else if((*it)->id() == TokenId::DIMENSION) {
				const std::string units = (*it)->getStringValue();
				double value = (*it)->getNumericValue();
				++it;
				return Object(CssLength(value, units));
			} else if((*it)->id() == TokenId::PERCENT) {
				return Object(CssLength((*it++)->getNumericValue(), true));
			} else if((*it)->id() == TokenId::NUMBER) {
				return Object(CssLength((*it++)->getNumericValue()));
			}
			return Object();
		}
		PropertyRegistrar margin_top("margin-top", 
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) { 
				(*plist)[CssProperty::MARGIN_TOP] = parse_width(it, end); 
		});
		PropertyRegistrar margin_bottom("margin-bottom", 
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) { 
				(*plist)[CssProperty::MARGIN_BOTTOM] = parse_width(it, end); 
		});
		PropertyRegistrar margin_left("margin-left", 
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) { 
				(*plist)[CssProperty::MARGIN_LEFT] = parse_width(it, end); 
		});
		PropertyRegistrar margin_right("margin-right", 
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) { 
				(*plist)[CssProperty::MARGIN_RIGHT] = parse_width(it, end); 
		});
		PropertyRegistrar margin_all("margin", 
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) { 
				Object w1(parse_width(it, end));
				skip_whitespace(it, end);
				TokenId id = get_token_id(it, end);
				if(id == TokenId::SEMICOLON || id == TokenId::RBRACE || it == end) {
					// one value, apply to all elements.
					(*plist)[CssProperty::MARGIN_TOP] = (*plist)[CssProperty::MARGIN_BOTTOM] = w1;
					(*plist)[CssProperty::MARGIN_LEFT] = (*plist)[CssProperty::MARGIN_RIGHT] = w1;
					return;
				}
				Object w2(parse_width(it, end));
				skip_whitespace(it, end);
				id = get_token_id(it, end);
				if(id == TokenId::SEMICOLON || id == TokenId::RBRACE || it == end) {
					// two values, apply to pairs of elements.
					(*plist)[CssProperty::MARGIN_TOP] = (*plist)[CssProperty::MARGIN_BOTTOM] = w1;
					(*plist)[CssProperty::MARGIN_LEFT] = (*plist)[CssProperty::MARGIN_RIGHT] = w2;
					return;
				}
				Object w3(parse_width(it, end));
				skip_whitespace(it, end);
				id = get_token_id(it, end);
				if(id == TokenId::SEMICOLON || id == TokenId::RBRACE || it == end) {
					// three values, apply to top, left/right, bottom
					(*plist)[CssProperty::MARGIN_TOP] = w1;
					(*plist)[CssProperty::MARGIN_LEFT] = (*plist)[CssProperty::MARGIN_RIGHT] = w2;
					(*plist)[CssProperty::MARGIN_BOTTOM] = w3;
					return;
				}
				Object w4(parse_width(it, end));
				skip_whitespace(it, end);

				// four values, apply to individual elements.
				(*plist)[CssProperty::MARGIN_TOP] = w1;
				(*plist)[CssProperty::MARGIN_RIGHT] = w2;
				(*plist)[CssProperty::MARGIN_BOTTOM] = w3;
				(*plist)[CssProperty::MARGIN_LEFT] = w4;
				
				skip_whitespace(it, end);
		});

		PropertyRegistrar padding_top("padding-top", 
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) { 
				(*plist)[CssProperty::PADDING_TOP] = parse_width(it, end); 
		});
		PropertyRegistrar padding_bottom("padding-bottom", 
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) { 
				(*plist)[CssProperty::PADDING_BOTTOM] = parse_width(it, end); 
		});
		PropertyRegistrar padding_left("padding-left", 
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) { 
				(*plist)[CssProperty::PADDING_LEFT] = parse_width(it, end); 
		});
		PropertyRegistrar padding_right("padding-right", 
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) { 
				(*plist)[CssProperty::PADDING_RIGHT] = parse_width(it, end); 
		});
		PropertyRegistrar padding_all("padding", 
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) { 
				Object w1(parse_width(it, end));
				skip_whitespace(it, end);
				TokenId id = (*it)->id();
				if(id == TokenId::SEMICOLON || id == TokenId::RBRACE || it == end) {
					// one value, apply to all elements.
					(*plist)[CssProperty::PADDING_TOP] = (*plist)[CssProperty::PADDING_BOTTOM] = w1;
					(*plist)[CssProperty::PADDING_LEFT] = (*plist)[CssProperty::PADDING_RIGHT] = w1;
					return;
				}
				Object w2(parse_width(it, end));
				skip_whitespace(it, end);
				id = (*it)->id();
				if(id == TokenId::SEMICOLON || id == TokenId::RBRACE || it == end) {
					// two values, apply to pairs of elements.
					(*plist)[CssProperty::PADDING_TOP] = (*plist)[CssProperty::PADDING_BOTTOM] = w1;
					(*plist)[CssProperty::PADDING_LEFT] = (*plist)[CssProperty::PADDING_RIGHT] = w2;
					return;
				}
				Object w3(parse_width(it, end));
				skip_whitespace(it, end);
				id = (*it)->id();
				if(id == TokenId::SEMICOLON || id == TokenId::RBRACE || it == end) {
					// three values, apply to top, left/right, bottom
					(*plist)[CssProperty::PADDING_TOP] = w1;
					(*plist)[CssProperty::PADDING_LEFT] = (*plist)[CssProperty::PADDING_RIGHT] = w2;
					(*plist)[CssProperty::PADDING_BOTTOM] = w3;
					return;
				}
				Object w4(parse_width(it, end));
				skip_whitespace(it, end);
				id = (*it)->id();

				// four values, apply to individual elements.
				(*plist)[CssProperty::PADDING_TOP] = w1;
				(*plist)[CssProperty::PADDING_RIGHT] = w2;
				(*plist)[CssProperty::PADDING_BOTTOM] = w3;
				(*plist)[CssProperty::PADDING_LEFT] = w4;
				
				skip_whitespace(it, end);
		});

		PropertyRegistrar border_top_color("border-top-color", 
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) { 
				(*plist)[CssProperty::BORDER_TOP_COLOR] = Object(parse_a_color_value(it, end));
		});
		PropertyRegistrar border_left_color("border-left-color", 
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) { 
				(*plist)[CssProperty::BORDER_LEFT_COLOR] = Object(parse_a_color_value(it, end));
		});
		PropertyRegistrar border_right_color("border-right-color", 
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) { 
				(*plist)[CssProperty::BORDER_RIGHT_COLOR] = Object(parse_a_color_value(it, end));
		});
		PropertyRegistrar border_bottom_color("border-bottom-color", 
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) { 
				(*plist)[CssProperty::BORDER_BOTTOM_COLOR] = Object(parse_a_color_value(it, end));
		});

		BorderStyle parse_border_style(std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end)
		{
			skip_whitespace(it, end);
			if((*it)->id() == TokenId::IDENT) {
				const std::string ref = (*it++)->getStringValue();
				skip_whitespace(it, end);
				if(ref == "none") {
					return BorderStyle::NONE;
				} else if(ref == "inherit") {
					return BorderStyle::INHERIT;
				} else if(ref == "hidden") {
					return BorderStyle::HIDDEN;
				} else if(ref == "dotted") {
					return BorderStyle::DOTTED;
				} else if(ref == "dashed") {
					return BorderStyle::DASHED;
				} else if(ref == "solid") {
					return BorderStyle::SOLID;
				} else if(ref == "double") {
					return BorderStyle::DOUBLE;
				} else if(ref == "groove") {
					return BorderStyle::GROOVE;
				} else if(ref == "ridge") {
					return BorderStyle::RIDGE;
				} else if(ref == "inset") {
					return BorderStyle::INSET;
				} else if(ref == "outset") {
					return BorderStyle::OUTSET;
				} else {
					throw ParserError(formatter() << "Expected identifier '" << ref << "' while parsing border style");
				}
			}
			throw ParserError(formatter() << "Expected IDENTIFIER, found: " << (*it)->toString());
			return BorderStyle::NONE;
		}

		PropertyRegistrar border_top_style("border-top-style",
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) {
				(*plist)[CssProperty::BORDER_TOP_STYLE] = Object(parse_border_style(it, end));
		});
		PropertyRegistrar border_left_style("border-left-style",
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) {
				(*plist)[CssProperty::BORDER_LEFT_STYLE] = Object(parse_border_style(it, end));
		});
		PropertyRegistrar border_right_style("border-right-style",
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) {
				(*plist)[CssProperty::BORDER_RIGHT_STYLE] = Object(parse_border_style(it, end));
		});
		PropertyRegistrar border_bottom_style("border-bottom-style",
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) {
				(*plist)[CssProperty::BORDER_BOTTOM_STYLE] = Object(parse_border_style(it, end));
		});

		Object border_width_parse(std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end)
		{
			skip_whitespace(it, end);
			if((*it)->id() == TokenId::IDENT) {
				const std::string ref = (*it)->getStringValue();
				if(ref == "thin") {
					++it;
					return Object(CssLength(border_width_thin));
				} else if(ref == "medium") {
					++it;
					return Object(CssLength(border_width_medium));
				} else if(ref == "thick") {
					++it;
					return Object(CssLength(border_width_thick));
				}
			}	
			return parse_width(it, end);
		}

		PropertyRegistrar border_top_width("border-top-width",
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) {
				(*plist)[CssProperty::BORDER_TOP_WIDTH] = border_width_parse(it, end);
		});
		PropertyRegistrar border_left_width("border-left-width",
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) {
				(*plist)[CssProperty::BORDER_LEFT_WIDTH] = border_width_parse(it, end);
		});
		PropertyRegistrar border_right_width("border-right-width",
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) {
				(*plist)[CssProperty::BORDER_RIGHT_WIDTH] = border_width_parse(it, end);
		});
		PropertyRegistrar border_bottom_width("border-bottom-width",
			[](std::vector<TokenPtr>::const_iterator& it, std::vector<TokenPtr>::const_iterator end, PropertyList* plist) {
				(*plist)[CssProperty::BORDER_BOTTOM_WIDTH] = border_width_parse(it, end);
		});
	}

	void apply_properties_to_css(CssStyles* attr, const PropertyList& plist) 
	{
		for(auto& p : plist) {
			switch(p.first) {
				case CssProperty::BACKGROUND_COLOR:
					attr->background_color_ = p.second.getValue<CssColor>();
					break;
				case CssProperty::BACKGROUND_ATTACHMENT:
					ASSERT_LOG(false, "Add background attachment handling");
					break;
				case CssProperty::COLOR:
					attr->color_ = p.second.getValue<CssColor>();
					break;
				case CssProperty::BORDER_TOP_COLOR:
					attr->border_top_.setColor(p.second.getValue<CssColor>());
					break;
				case CssProperty::BORDER_TOP_STYLE:
					attr->border_top_.setStyle(p.second.getValue<BorderStyle>());
					break;
				case CssProperty::BORDER_TOP_WIDTH:
					attr->border_top_.setWidth(p.second.getValue<CssLength>());
					break;

				case CssProperty::BORDER_RIGHT_COLOR:
					attr->border_right_.setColor(p.second.getValue<CssColor>());
					break;
				case CssProperty::BORDER_RIGHT_STYLE:
					attr->border_right_.setStyle(p.second.getValue<BorderStyle>());
					break;
				case CssProperty::BORDER_RIGHT_WIDTH:
					attr->border_right_.setWidth(p.second.getValue<CssLength>());
					break;

				case CssProperty::BORDER_LEFT_COLOR:
					attr->border_left_.setColor(p.second.getValue<CssColor>());
					break;
				case CssProperty::BORDER_LEFT_STYLE:
					attr->border_left_.setStyle(p.second.getValue<BorderStyle>());
					break;
				case CssProperty::BORDER_LEFT_WIDTH:
					attr->border_left_.setWidth(p.second.getValue<CssLength>());
					break;

				case CssProperty::BORDER_BOTTOM_COLOR:
					attr->border_bottom_.setColor(p.second.getValue<CssColor>());
					break;
				case CssProperty::BORDER_BOTTOM_STYLE:
					attr->border_bottom_.setStyle(p.second.getValue<BorderStyle>());
					break;
				case CssProperty::BORDER_BOTTOM_WIDTH:
					attr->border_bottom_.setWidth(p.second.getValue<CssLength>());
					break;

				case MARGIN_TOP:
					attr->margin_top_ = p.second.getValue<CssLength>();
					break;
				case MARGIN_BOTTOM:
					attr->margin_bottom_ = p.second.getValue<CssLength>();
					break;
				case MARGIN_LEFT:
					attr->margin_left_ = p.second.getValue<CssLength>();
					break;
				case MARGIN_RIGHT:
					attr->margin_right_ = p.second.getValue<CssLength>();
					break;

				case PADDING_TOP:
					attr->padding_top_ = p.second.getValue<CssLength>();
					break;
				case PADDING_BOTTOM:
					attr->padding_bottom_ = p.second.getValue<CssLength>();
					break;
				case PADDING_LEFT:
					attr->padding_left_ = p.second.getValue<CssLength>();
					break;
				case PADDING_RIGHT:
					attr->padding_right_ = p.second.getValue<CssLength>();
					break;

				default: break;
			}
		}
	}

	property_parse_function find_property_handler(const std::string& name)
	{
		auto it = get_property_table().find(name);
		if(it == get_property_table().end()) {
			return nullptr;
		}
		return it->second;
	}
}
