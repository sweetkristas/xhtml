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

#include <string>
#include <vector>

#include <boost/property_tree/ptree.hpp>

#include "xhtml_attributes.hpp"
#include "xhtml_element_id.hpp"
#include "xhtml_fwd.hpp"

namespace xhtml
{
	typedef std::function<ElementPtr(ElementId, const std::string&, const boost::property_tree::ptree&)> ElementFactoryFnType;
	class Element : public std::enable_shared_from_this<Element>
	{
	public:
		virtual ~Element();
		ElementId id() const { return id_; }
		const std::string& getName() const { return name_; }
		static ElementPtr factory(const std::string& name, const boost::property_tree::ptree& pt);
		void parse(const boost::property_tree::ptree& pt);
		void render(RenderContext* ctx) const;
		static void registerFactoryFunction(ElementId id, const std::string& type, ElementFactoryFnType fn);
		void preOrderTraverse(std::function<void(ElementPtr)> fn);
		static void addCustomElement(const std::string& e);
	protected:
		explicit Element(ElementId id, const std::string& name, const boost::property_tree::ptree& pt);
		bool parseWithText(const boost::property_tree::ptree& pt);
		void addChild(ElementPtr child);
	private:
		virtual bool handleParse(const boost::property_tree::ptree& pt) { return true; }
		virtual void handleRender(RenderContext* ctx) const {}
		ElementId id_;
		std::string name_;
		Attributes attrs_;
		std::vector<ElementPtr> children_;
	};

	template<typename T>
	struct ElementRegistrar
	{
		ElementRegistrar(ElementId id, const std::string& type)
		{
			// register the class factory function 
			Element::registerFactoryFunction(id, type, [](ElementId id, const std::string& name, const boost::property_tree::ptree& pt) -> ElementPtr { return std::make_shared<T>(id, name, pt); });
		}
	};

	// ugly hack -- need a more elegant template/std::forward solution
	template<typename T>
	struct ElementRegistrarInt
	{
		ElementRegistrarInt(ElementId id, const std::string& type, int value)
		{
			// register the class factory function 
			Element::registerFactoryFunction(id, type, [value](ElementId id, const std::string& name, const boost::property_tree::ptree& pt) -> ElementPtr { return std::make_shared<T>(id, name, pt, value); });
		}
	};
}
