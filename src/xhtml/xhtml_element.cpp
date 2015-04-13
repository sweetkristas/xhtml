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
			explicit HtmlElement(const ptree& pt) : Element(pt) {}
		};
		ElementRegistrar<HtmlElement> html_element("html");

		struct HeadElement : public Element
		{
			explicit HeadElement(const ptree& pt) : Element(pt) {}
		};
		ElementRegistrar<HeadElement> head_element("head");

		struct BodyElement : public Element
		{
			explicit BodyElement(const ptree& pt) : Element(pt) {}
		};
		ElementRegistrar<BodyElement> body_element("body");

		struct ScriptElement : public Element
		{
			explicit ScriptElement(const ptree& pt) : Element(pt) {}
		};
		ElementRegistrar<ScriptElement> script_element("script");

		struct ParagraphElement : public Element
		{
			explicit ParagraphElement(const ptree& pt) : Element(pt) {}
			bool handleParse(const boost::property_tree::ptree& pt) override {
				return parseWithText(pt);
			}
		};
		ElementRegistrar<ParagraphElement> paragraph_element("p");

		struct AbbreviationElement : public Element
		{
			explicit AbbreviationElement(const ptree& pt) : Element(pt) {}
			bool handleParse(const boost::property_tree::ptree& pt) override {
				return parseWithText(pt);
			}
		};
		ElementRegistrar<AbbreviationElement> abbreviation_element("abbr");

		struct EmphasisElement : public Element
		{
			explicit EmphasisElement(const ptree& pt) : Element(pt) {}
			bool handleParse(const boost::property_tree::ptree& pt) override {
				return parseWithText(pt);
			}
		};
		ElementRegistrar<EmphasisElement> emphasis_element("em");

		struct BreakElement : public Element
		{
			explicit BreakElement(const ptree& pt) : Element(pt) {}
		};
		ElementRegistrar<BreakElement> break_element("br");

		struct ImageElement : public Element
		{
			explicit ImageElement(const ptree& pt) : Element(pt) {}
		};
		ElementRegistrar<ImageElement> img_element("img");

		struct ObjectElement : public Element
		{
			explicit ObjectElement(const ptree& pt) : Element(pt) {}
		};
		ElementRegistrar<ObjectElement> object_element("object");

		struct TextElement : public Element
		{
			explicit TextElement(const ptree& pt) : Element(pt) {}
		};
		ElementRegistrar<TextElement> text_element(XmlText);

		// Start of elements.
		struct TitleElement : public Element
		{
			explicit TitleElement(const ptree& pt) : Element(pt) {}
			bool handleParse(const boost::property_tree::ptree& pt) override {
				auto txt = pt.get_child_optional(XmlText);
				if(txt) {
					title_ = txt->data();
				}
				return true;
			}
			std::string title_;
		};
		ElementRegistrar<TitleElement> title_element("title");
	}

	Element::Element(const ptree& pt)
		: attrs_(pt)
	{
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
		return it->second(pt);
	}
}
