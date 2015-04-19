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

#include <map>

#include "asserts.hpp"
#include "css_parser.hpp"
#include "xhtml_element.hpp"

namespace xhtml
{
	using namespace boost::property_tree;

	namespace
	{
		const std::string XmlAttr = "<xmlattr>";
		const std::string XmlText = "<xmltext>";

		typedef std::map<std::string, ElementFactoryFnType> ElementRegistry;
		ElementRegistry& get_element_registry()
		{
			static ElementRegistry res;
			return res;
		}

		// Start of elements.
		struct HtmlElement : public Element
		{
			explicit HtmlElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<HtmlElement> html_element("html");

		struct HeadElement : public Element
		{
			explicit HeadElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<HeadElement> head_element("head");

		struct BodyElement : public Element
		{
			explicit BodyElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<BodyElement> body_element("body");

		struct ScriptElement : public Element
		{
			explicit ScriptElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<ScriptElement> script_element("script");

		struct ParagraphElement : public Element
		{
			explicit ParagraphElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
			bool handleParse(const boost::property_tree::ptree& pt) override {
				return parseWithText(pt);
			}
		};
		ElementRegistrar<ParagraphElement> paragraph_element("p");

		struct AbbreviationElement : public Element
		{
			explicit AbbreviationElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
			bool handleParse(const boost::property_tree::ptree& pt) override {
				return parseWithText(pt);
			}
		};
		ElementRegistrar<AbbreviationElement> abbreviation_element("abbr");

		struct EmphasisElement : public Element
		{
			explicit EmphasisElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
			bool handleParse(const boost::property_tree::ptree& pt) override {
				return parseWithText(pt);
			}
		};
		ElementRegistrar<EmphasisElement> emphasis_element("em");

		struct BreakElement : public Element
		{
			explicit BreakElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<BreakElement> break_element("br");

		struct ImageElement : public Element
		{
			explicit ImageElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<ImageElement> img_element("img");

		struct ObjectElement : public Element
		{
			explicit ObjectElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<ObjectElement> object_element("object");

		struct TextElement : public Element
		{
			explicit TextElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<TextElement> text_element(XmlText);

		struct StyleElement : public Element
		{
			explicit StyleElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
			bool handleParse(const boost::property_tree::ptree& pt) override {
				auto styles = pt.get_child_optional(XmlText);
				if(styles) {
				// XXX added as a test here only.
					css::Tokenizer tok(styles->data());
					css::Parser parse(tok.getTokens());
					for(auto& r : parse.getStyleSheet()->getRules()) {
						parse.parseRule(r);
					}
				}
				return false;
			}
		};
		ElementRegistrar<StyleElement> style_element("style");

		// Start of elements.
		struct TitleElement : public Element
		{
			explicit TitleElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
			bool handleParse(const boost::property_tree::ptree& pt) override {
				auto txt = pt.get_child_optional(XmlText);
				if(txt) {
					title_ = txt->data();
				}
				return true;
			}
		private:
			std::string title_;
		};
		ElementRegistrar<TitleElement> title_element("title");

		struct LinkElement : public Element
		{
			explicit LinkElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<LinkElement> link_element("link");

		struct MetaElement : public Element
		{
			explicit MetaElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<MetaElement> meta_element("meta");

		struct BaseElement : public Element
		{
			explicit BaseElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<BaseElement> base_element("base");

		struct FormElement : public Element
		{
			explicit FormElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<FormElement> form_element("form");

		struct SelectElement : public Element
		{
			explicit SelectElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<SelectElement> select_element("select");

		struct OptionGroupElement : public Element
		{
			explicit OptionGroupElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<OptionGroupElement> option_group_element("optgroup");

		struct OptionElement : public Element
		{
			explicit OptionElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<OptionElement> option_element("option");

		struct InputElement : public Element
		{
			explicit InputElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<InputElement> input_element("input");

		struct TextAreaElement : public Element
		{
			explicit TextAreaElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<TextAreaElement> text_area_element("textarea");

		struct ButtonElement : public Element
		{
			explicit ButtonElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<ButtonElement> button_element("button");

		struct LabelElement : public Element
		{
			explicit LabelElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<LabelElement> label_element("label");

		struct FieldSetElement : public Element
		{
			explicit FieldSetElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<FieldSetElement> field_set_element("fieldset");

		struct LegendElement : public Element
		{
			explicit LegendElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<LegendElement> legend_element("legend");

		struct UnorderedListElement : public Element
		{
			explicit UnorderedListElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<UnorderedListElement> unordered_list_element("ul");

		struct OrderedListElement : public Element
		{
			explicit OrderedListElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<OrderedListElement> ordered_list_element("ol");

		struct DefinitionListElement : public Element
		{
			explicit DefinitionListElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<DefinitionListElement> definition_list_element("dl");

		struct DirectoryElement : public Element
		{
			explicit DirectoryElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<DirectoryElement> directory_element("dir");

		struct MenuElement : public Element
		{
			explicit MenuElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<MenuElement> menu_element("menu");

		struct ListItemElement : public Element
		{
			explicit ListItemElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<ListItemElement> list_item_element("li");

		struct DivElement : public Element
		{
			explicit DivElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<DivElement> div_element("div");

		struct HeadingElement : public Element
		{
			explicit HeadingElement(const std::string& name, const ptree& pt, int level) : Element(name, pt), level_(level) {}
		private:
			int level_;
		};
		ElementRegistrarInt<HeadingElement> h1_element("h1", 1);
		ElementRegistrarInt<HeadingElement> h2_element("h2", 2);
		ElementRegistrarInt<HeadingElement> h3_element("h3", 3);
		ElementRegistrarInt<HeadingElement> h4_element("h4", 4);
		ElementRegistrarInt<HeadingElement> h5_element("h5", 5);
		ElementRegistrarInt<HeadingElement> h6_element("h6", 6);
		ElementRegistrarInt<HeadingElement> h7_element("h7", 7);
		ElementRegistrarInt<HeadingElement> h8_element("h8", 8);
		ElementRegistrarInt<HeadingElement> h9_element("h9", 9);
		ElementRegistrarInt<HeadingElement> h10_element("h10", 10);
		ElementRegistrarInt<HeadingElement> h11_element("h11", 11);
		ElementRegistrarInt<HeadingElement> h12_element("h12", 12);
		ElementRegistrarInt<HeadingElement> h13_element("h13", 13);
		ElementRegistrarInt<HeadingElement> h14_element("h14", 14);
		ElementRegistrarInt<HeadingElement> h15_element("h15", 15);
		ElementRegistrarInt<HeadingElement> h16_element("h16", 16);

		struct QuoteElement : public Element
		{
			explicit QuoteElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<QuoteElement> quote_element("q");

		struct BlockQuoteElement : public Element
		{
			explicit BlockQuoteElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<BlockQuoteElement> block_quote_element("blockquote");

		struct PreformattedElement : public Element
		{
			explicit PreformattedElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<PreformattedElement> preformatted_element("pre");

		struct HorizontalRuleElement : public Element
		{
			explicit HorizontalRuleElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<HorizontalRuleElement> horizontal_rule_element("hr");

		struct ModElement : public Element
		{
			explicit ModElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<ModElement> mod_element("mod");

		struct AnchorElement : public Element
		{
			explicit AnchorElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<AnchorElement> anchor_element("a");

		struct ParamElement : public Element
		{
			explicit ParamElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<ParamElement> param_element("param");

		struct AppletElement : public Element
		{
			explicit AppletElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<AppletElement> applet_element("applet");

		struct MapElement : public Element
		{
			explicit MapElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<MapElement> map_element("map");

		struct AreaElement : public Element
		{
			explicit AreaElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<AreaElement> area_element("area");

		struct TableElement : public Element
		{
			explicit TableElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<TableElement> table_element("table");

		struct TableCaptionElement : public Element
		{
			explicit TableCaptionElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<TableCaptionElement> table_caption_element("caption");

		struct TableColElement : public Element
		{
			explicit TableColElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<TableColElement> table_col_element("col");

		struct TableColGroupElement : public Element
		{
			explicit TableColGroupElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<TableColGroupElement> table_col_group_element("colgroup");

		struct TableSectionElement : public Element
		{
			explicit TableSectionElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<TableSectionElement> table_head_element("thead");
		ElementRegistrar<TableSectionElement> table_foot_element("tfoot");
		ElementRegistrar<TableSectionElement> table_body_element("tbody");

		struct TableRowElement : public Element
		{
			explicit TableRowElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<TableRowElement> table_row_element("tr");

		struct TableCellElement : public Element
		{
			explicit TableCellElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<TableCellElement> table_cell_element("td");

		struct FrameSetElement : public Element
		{
			explicit FrameSetElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<FrameSetElement> frame_set_element("frameset");

		struct FrameElement : public Element
		{
			explicit FrameElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<FrameElement> frame_element("frame");

		struct IFrameElement : public Element
		{
			explicit IFrameElement(const std::string& name, const ptree& pt) : Element(name, pt) {}
		};
		ElementRegistrar<IFrameElement> i_frame_element("iframe");
	}

	Element::Element(const std::string& name, const ptree& pt)
		: name_(name),
		  attrs_(pt),
		  children_()
	{
		// XXX added as a test here only.
		if(!attrs_.getStyle().empty()) {
			css::Tokenizer tok(attrs_.getStyle());
			css::Parser parse(tok.getTokens());
		}
	}

	Element::~Element()
	{
	}

	void Element::parse(const boost::property_tree::ptree& pt)
	{
		if(handleParse(pt)) {
			for(auto& element : pt) {
				if(element.first != XmlText && element.first != XmlAttr) {
					auto elem = Element::factory(element.first, element.second);
					if(elem) {
						elem->parse(element.second);
						addChild(elem);
					}
				}
			}
		}
	}

	bool Element::parseWithText(const boost::property_tree::ptree& pt)
	{
		// Do not call handleParse() here. This is for dervied class use.
		for(auto& element : pt) {
			if(element.first != XmlAttr) {
				auto elem = Element::factory(element.first, element.second);
				if(elem) {
					elem->parse(element.second);
					addChild(elem);
				}
			}
		}
		return false;
	}

	void Element::render(RenderContext* ctx) const
	{
		handleRender(ctx);
		for(auto& child : children_) {
			child->render(ctx);
		}
	}

	void Element::addChild(ElementPtr child)
	{
		children_.emplace_back(child);
	}

	void Element::preOrderTraverse(std::function<void(ElementPtr)> fn)
	{
		fn(shared_from_this());
		for(auto& child : children_) {
			child->preOrderTraverse(fn);
		}
	}

	void Element::registerFactoryFunction(const std::string& type, ElementFactoryFnType fn)
	{
		get_element_registry()[type] = fn;
	}

	ElementPtr Element::factory(const std::string& name, const ptree& pt)
	{
		auto it = get_element_registry().find(name);
		if(it == get_element_registry().end()) {
			LOG_ERROR("Ignoring creating node for element: " << name << " no handler for that type.");
			return nullptr;
		}
		return it->second(name, pt);
	}
}
