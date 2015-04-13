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

#include <functional>
#include <map>

#include "asserts.hpp"
#include "xhtml_attributes.hpp"

namespace xhtml
{
	using namespace boost::property_tree;

	namespace 
	{
		const std::string XmlAttr = "<xmlattr>";

		typedef std::map<std::string, std::function<void(Attributes*,const std::string&)>> attribute_map_type;
		attribute_map_type& get_attribute_map()
		{
			static attribute_map_type res;
			if(res.empty()) {
				res["id"] = [](Attributes* attr, const std::string& value) {
					attr->setID(value);
				};
				res["class"] = [](Attributes* attr, const std::string& value) {
					attr->setClass(value);
				};
				res["style"] = [](Attributes* attr, const std::string& value) {
					attr->setStyle(value);
				};
				res["title"] = [](Attributes* attr, const std::string& value) {
					attr->setTitle(value);
				};

				res["lang"] = [](Attributes* attr, const std::string& value) {
					attr->setLang(value);
				};
				res["dir"] = [](Attributes* attr, const std::string& value) {
					attr->setDir(value);
				};

				res["onclick"] = [](Attributes* attr, const std::string& value) {
					attr->setOnClick(value);
				};
				res["ondblclick"] = [](Attributes* attr, const std::string& value) {
					attr->setOnDblClick(value);
				};
				res["onmousedown"] = [](Attributes* attr, const std::string& value) {
					attr->setOnMouseDown(value);
				};
				res["onmouseup"] = [](Attributes* attr, const std::string& value) {
					attr->setOnMouseUp(value);
				};
				res["onmouseover"] = [](Attributes* attr, const std::string& value) {
					attr->setOnMouseOver(value);
				};
				res["onmousemove"] = [](Attributes* attr, const std::string& value) {
					attr->setOnMouseMove(value);
				};
				res["onmouseout"] = [](Attributes* attr, const std::string& value) {
					attr->setOnMouseOut(value);
				};
				res["onkeypress"] = [](Attributes* attr, const std::string& value) {
					attr->setOnKeyPress(value);
				};
				res["onkeydown"] = [](Attributes* attr, const std::string& value) {
					attr->setOnKeyDown(value);
				};
				res["onkeyup"] = [](Attributes* attr, const std::string& value) {
					attr->setOnKeyUp(value);
				};
			}
			return res;
		}
	}

	Attributes::Attributes(const ptree& pt)
	{
		auto attributes = pt.get_child_optional(XmlAttr);
		if(attributes) {
			for(auto& attr : *attributes) {
				const std::string attr_data = attr.second.data();
				auto it = get_attribute_map().find(attr.first);
				if(it != get_attribute_map().end()) {
					it->second(this, attr_data);
				} else {
					LOG_ERROR("Unrecognized attribute value: " << attr.first << " with data: " << attr_data);
				}
			}
		}
	}
}
