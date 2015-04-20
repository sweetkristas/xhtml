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

		// Used to track what custom element value we should next allocate.
		int custom_element_counter = -1;

		struct ElementFunctionAndId
		{
			ElementFunctionAndId() : id(ElementId::ANY), fn() {}
			ElementFunctionAndId(ElementId i, ElementFactoryFnType f) : id(i), fn(f) {}
			ElementId id;
			ElementFactoryFnType fn;
		};

		typedef std::map<std::string, ElementFunctionAndId> ElementRegistry;
		ElementRegistry& get_element_registry()
		{
			static ElementRegistry res;
			return res;
		}
		
		std::map<ElementId, std::string>& get_id_registry()
		{
			static std::map<ElementId, std::string> res;
			if(res.empty()) {
				res[ElementId::ANY] = "*";
			}
			return res;
		}

		// Start of elements.
		struct HtmlElement : public Element
		{
			explicit HtmlElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<HtmlElement> html_element(ElementId::HTML, "html");

		struct HeadElement : public Element
		{
			explicit HeadElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<HeadElement> head_element(ElementId::HEAD, "head");

		struct BodyElement : public Element
		{
			explicit BodyElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<BodyElement> body_element(ElementId::BODY, "body");

		struct ScriptElement : public Element
		{
			explicit ScriptElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<ScriptElement> script_element(ElementId::SCRIPT, "script");

		struct ParagraphElement : public Element
		{
			explicit ParagraphElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
			bool handleParse(const boost::property_tree::ptree& pt) override {
				return parseWithText(pt);
			}
		};
		ElementRegistrar<ParagraphElement> paragraph_element(ElementId::P, "p");

		struct AbbreviationElement : public Element
		{
			explicit AbbreviationElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
			bool handleParse(const boost::property_tree::ptree& pt) override {
				return parseWithText(pt);
			}
		};
		ElementRegistrar<AbbreviationElement> abbreviation_element(ElementId::ABBR, "abbr");

		struct EmphasisElement : public Element
		{
			explicit EmphasisElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
			bool handleParse(const boost::property_tree::ptree& pt) override {
				return parseWithText(pt);
			}
		};
		ElementRegistrar<EmphasisElement> emphasis_element(ElementId::EM, "em");

		struct BreakElement : public Element
		{
			explicit BreakElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<BreakElement> break_element(ElementId::BR, "br");

		struct ImageElement : public Element
		{
			explicit ImageElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<ImageElement> img_element(ElementId::IMG, "img");

		struct ObjectElement : public Element
		{
			explicit ObjectElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<ObjectElement> object_element(ElementId::OBJECT, "object");

		struct TextElement : public Element
		{
			explicit TextElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<TextElement> text_element(ElementId::XMLTEXT, XmlText);

		struct StyleElement : public Element
		{
			explicit StyleElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
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
		ElementRegistrar<StyleElement> style_element(ElementId::STYLE, "style");

		// Start of elements.
		struct TitleElement : public Element
		{
			explicit TitleElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
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
		ElementRegistrar<TitleElement> title_element(ElementId::TITLE, "title");

		struct LinkElement : public Element
		{
			explicit LinkElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<LinkElement> link_element(ElementId::LINK, "link");

		struct MetaElement : public Element
		{
			explicit MetaElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<MetaElement> meta_element(ElementId::META, "meta");

		struct BaseElement : public Element
		{
			explicit BaseElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<BaseElement> base_element(ElementId::BASE, "base");

		struct FormElement : public Element
		{
			explicit FormElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<FormElement> form_element(ElementId::FORM, "form");

		struct SelectElement : public Element
		{
			explicit SelectElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<SelectElement> select_element(ElementId::SELECT, "select");

		struct OptionGroupElement : public Element
		{
			explicit OptionGroupElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<OptionGroupElement> option_group_element(ElementId::OPTGROUP, "optgroup");

		struct OptionElement : public Element
		{
			explicit OptionElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<OptionElement> option_element(ElementId::OPTION, "option");

		struct InputElement : public Element
		{
			explicit InputElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<InputElement> input_element(ElementId::INPUT, "input");

		struct TextAreaElement : public Element
		{
			explicit TextAreaElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<TextAreaElement> text_area_element(ElementId::TEXTAREA, "textarea");

		struct ButtonElement : public Element
		{
			explicit ButtonElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<ButtonElement> button_element(ElementId::BUTTON, "button");

		struct LabelElement : public Element
		{
			explicit LabelElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<LabelElement> label_element(ElementId::LABEL, "label");

		struct FieldSetElement : public Element
		{
			explicit FieldSetElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<FieldSetElement> field_set_element(ElementId::FIELDSET, "fieldset");

		struct LegendElement : public Element
		{
			explicit LegendElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<LegendElement> legend_element(ElementId::LEGEND, "legend");

		struct UnorderedListElement : public Element
		{
			explicit UnorderedListElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<UnorderedListElement> unordered_list_element(ElementId::UL, "ul");

		struct OrderedListElement : public Element
		{
			explicit OrderedListElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<OrderedListElement> ordered_list_element(ElementId::OL, "ol");

		struct DefinitionListElement : public Element
		{
			explicit DefinitionListElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<DefinitionListElement> definition_list_element(ElementId::DL, "dl");

		struct DirectoryElement : public Element
		{
			explicit DirectoryElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<DirectoryElement> directory_element(ElementId::DIR, "dir");

		struct MenuElement : public Element
		{
			explicit MenuElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<MenuElement> menu_element(ElementId::MENU, "menu");

		struct ListItemElement : public Element
		{
			explicit ListItemElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<ListItemElement> list_item_element(ElementId::LI, "li");

		struct DivElement : public Element
		{
			explicit DivElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<DivElement> div_element(ElementId::DIV, "div");

		struct HeadingElement : public Element
		{
			explicit HeadingElement(ElementId id, const std::string& name, const ptree& pt, int level) : Element(id, name, pt), level_(level) {}
		private:
			int level_;
		};
		ElementRegistrarInt<HeadingElement> h1_element(ElementId::H1, "h1", 1);
		ElementRegistrarInt<HeadingElement> h2_element(ElementId::H2, "h2", 2);
		ElementRegistrarInt<HeadingElement> h3_element(ElementId::H3, "h3", 3);
		ElementRegistrarInt<HeadingElement> h4_element(ElementId::H4, "h4", 4);
		ElementRegistrarInt<HeadingElement> h5_element(ElementId::H5, "h5", 5);
		ElementRegistrarInt<HeadingElement> h6_element(ElementId::H6, "h6", 6);

		struct QuoteElement : public Element
		{
			explicit QuoteElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<QuoteElement> quote_element(ElementId::Q, "q");

		struct BlockQuoteElement : public Element
		{
			explicit BlockQuoteElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<BlockQuoteElement> block_quote_element(ElementId::BLOCKQUOTE, "blockquote");

		struct PreformattedElement : public Element
		{
			explicit PreformattedElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<PreformattedElement> preformatted_element(ElementId::PRE, "pre");

		struct HorizontalRuleElement : public Element
		{
			explicit HorizontalRuleElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<HorizontalRuleElement> horizontal_rule_element(ElementId::HR, "hr");

		struct ModElement : public Element
		{
			explicit ModElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<ModElement> mod_element(ElementId::MOD, "mod");

		struct AnchorElement : public Element
		{
			explicit AnchorElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<AnchorElement> anchor_element(ElementId::A, "a");

		struct ParamElement : public Element
		{
			explicit ParamElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<ParamElement> param_element(ElementId::PARAM, "param");

		struct AppletElement : public Element
		{
			explicit AppletElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<AppletElement> applet_element(ElementId::APPLET, "applet");

		struct MapElement : public Element
		{
			explicit MapElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<MapElement> map_element(ElementId::MAP, "map");

		struct AreaElement : public Element
		{
			explicit AreaElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<AreaElement> area_element(ElementId::AREA, "area");

		struct TableElement : public Element
		{
			explicit TableElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<TableElement> table_element(ElementId::TABLE, "table");

		struct TableCaptionElement : public Element
		{
			explicit TableCaptionElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<TableCaptionElement> table_caption_element(ElementId::CAPTION, "caption");

		struct TableColElement : public Element
		{
			explicit TableColElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<TableColElement> table_col_element(ElementId::COL, "col");

		struct TableColGroupElement : public Element
		{
			explicit TableColGroupElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<TableColGroupElement> table_col_group_element(ElementId::COLGROUP, "colgroup");

		struct TableSectionElement : public Element
		{
			explicit TableSectionElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<TableSectionElement> table_head_element(ElementId::THEAD, "thead");
		ElementRegistrar<TableSectionElement> table_foot_element(ElementId::TFOOT, "tfoot");
		ElementRegistrar<TableSectionElement> table_body_element(ElementId::TBODY, "tbody");

		struct TableRowElement : public Element
		{
			explicit TableRowElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<TableRowElement> table_row_element(ElementId::TR, "tr");

		struct TableCellElement : public Element
		{
			explicit TableCellElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<TableCellElement> table_cell_element(ElementId::TD, "td");

		struct FrameSetElement : public Element
		{
			explicit FrameSetElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<FrameSetElement> frame_set_element(ElementId::FRAMESET, "frameset");

		struct FrameElement : public Element
		{
			explicit FrameElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<FrameElement> frame_element(ElementId::FRAME, "frame");

		struct IFrameElement : public Element
		{
			explicit IFrameElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<IFrameElement> i_frame_element(ElementId::IFRAME, "iframe");

		struct SpanElement : public Element
		{
			explicit SpanElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<SpanElement> span_element(ElementId::SPAN, "span");

		struct AcronymElement : public Element
		{
			explicit AcronymElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<AcronymElement> acronym_element(ElementId::ACRONYM, "acronym");
		
		struct AddressElement : public Element
		{
			explicit AddressElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<AddressElement> address_element(ElementId::ADDRESS, "address");
		
		struct BoldElement : public Element
		{
			explicit BoldElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<SpanElement> bold_element(ElementId::B, "b");
		
		struct BDOElement : public Element
		{
			explicit BDOElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<SpanElement> bdo_element(ElementId::BDO, "bdo");
		
		struct BigElement : public Element
		{
			explicit BigElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<BigElement> big_element(ElementId::BIG, "big");
		
		struct CitationElement : public Element
		{
			explicit CitationElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<CitationElement> cite_element(ElementId::CITE, "cite");
		
		struct CodeElement : public Element
		{
			explicit CodeElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<CodeElement> code_element(ElementId::CODE, "code");
		
		struct DefinitionDescriptionElement : public Element
		{
			explicit DefinitionDescriptionElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<DefinitionDescriptionElement> dd_element(ElementId::DD, "dd");
		
		struct InsertedContentElement : public Element
		{
			explicit InsertedContentElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<InsertedContentElement> ins_element(ElementId::INS, "ins");
		
		struct DeletedContentElement : public Element
		{
			explicit DeletedContentElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<DeletedContentElement> del_element(ElementId::DEL, "del");
		
		struct DefiningInstanceElement : public Element
		{
			explicit DefiningInstanceElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<DefiningInstanceElement> dfn_element(ElementId::DFN, "dfn");
		
		struct DefinitionTermElement : public Element
		{
			explicit DefinitionTermElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<DefinitionTermElement> dt_element(ElementId::DT, "dt");
		
		struct ItalicsElement : public Element
		{
			explicit ItalicsElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<ItalicsElement> italics_element(ElementId::I, "i");
		
		struct KbdElement : public Element
		{
			explicit KbdElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<KbdElement> kbd_element(ElementId::KBD, "kbd");
		
		struct NoScriptElement : public Element
		{
			explicit NoScriptElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<NoScriptElement> noscript_element(ElementId::NOSCRIPT, "noscript");
		
		struct RubyBaseTextElement : public Element
		{
			explicit RubyBaseTextElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<RubyBaseTextElement> rb_element(ElementId::RB, "rb");
		
		struct RubyBaseContainerElement : public Element
		{
			explicit RubyBaseContainerElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<RubyBaseContainerElement> rbc_element(ElementId::RBC, "rbc");
		
		struct RubyAnnotationTextElement : public Element
		{
			explicit RubyAnnotationTextElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<RubyAnnotationTextElement> rt_element(ElementId::RT, "rt");
		
		struct RubyTextContainerElement : public Element
		{
			explicit RubyTextContainerElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<RubyTextContainerElement> rtc_element(ElementId::RTC, "rtc");
		
		struct RubyAnnotaionElement : public Element
		{
			explicit RubyAnnotaionElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<RubyAnnotaionElement> ruby_element(ElementId::RUBY, "ruby");
		
		struct SampleOutputElement : public Element
		{
			explicit SampleOutputElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<SpanElement> samp_element(ElementId::SAMP, "samp");
		
		struct SmallElement : public Element
		{
			explicit SmallElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<SmallElement> small_element(ElementId::SMALL, "small");
		
		struct StrongElement : public Element
		{
			explicit StrongElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<StrongElement> strong_element(ElementId::STRONG, "strong");
		
		struct SubscriptElement : public Element
		{
			explicit SubscriptElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<SubscriptElement> subscript_element(ElementId::SUB, "sub");
		
		struct SuperscriptElement : public Element
		{
			explicit SuperscriptElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<SuperscriptElement> superscript_element(ElementId::SUP, "sup");
		
		struct TeleTypeElement : public Element
		{
			explicit TeleTypeElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<TeleTypeElement> teletype_element(ElementId::TT, "tt");
		
		struct VariableElement : public Element
		{
			explicit VariableElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
		ElementRegistrar<VariableElement> var_element(ElementId::VAR, "var");

		struct CustomElement : public Element
		{
			explicit CustomElement(ElementId id, const std::string& name, const ptree& pt) : Element(id, name, pt) {}
		};
	}

	Element::Element(ElementId id, const std::string& name, const ptree& pt)
		: id_(id),
		  name_(name),
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

	void Element::registerFactoryFunction(ElementId id, const std::string& type, ElementFactoryFnType fn)
	{
		get_element_registry()[type] = ElementFunctionAndId(id, fn);
		get_id_registry()[id] = type;
	}

	void Element::addCustomElement(const std::string& e)
	{
		LOG_INFO("Creating custom element '" << e << "' with id: " << custom_element_counter);

		registerFactoryFunction(static_cast<ElementId>(custom_element_counter), 
			e, 
			[](ElementId id, const std::string& name, const ptree& pt) -> ElementPtr { return std::make_shared<CustomElement>(id, name, pt); });

		--custom_element_counter;
	}

	ElementPtr Element::factory(const std::string& name, const ptree& pt)
	{
		auto it = get_element_registry().find(name);
		if(it == get_element_registry().end()) {
			addCustomElement(name);
			it = get_element_registry().find(name);
			ASSERT_LOG(it != get_element_registry().end(), "Couldn't find factory function for '" << name << "' though one was recently added.");
		}
		return it->second.fn(it->second.id, name, pt);
	}

	ElementId string_to_element_id(const std::string& e)
	{
		auto it = get_element_registry().find(e);
		if(it == get_element_registry().end()) {
			Element::addCustomElement(e);
			it = get_element_registry().find(e);
		}
		ASSERT_LOG(it != get_element_registry().end(), "No element with type '" << e << "' was found.");
		return it->second.id;
	}

	const std::string& element_id_to_string(ElementId id)
	{
		auto it = get_id_registry().find(id);
		ASSERT_LOG(it != get_id_registry().end(), "Couldn't find an element with id of: " << static_cast<int>(id));
		return it->second;
	}
}
