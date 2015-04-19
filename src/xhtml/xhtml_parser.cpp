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

#include <boost/property_tree/xml_parser.hpp>

#include "asserts.hpp"
#include "xhtml_doc.hpp"
#include "xhtml_element.hpp"
#include "xhtml_parser.hpp"

namespace xhtml
{
	namespace 
	{
		const std::string XmlAttr = "<xmlattr>";
		const std::string XmlText = "<xmltext>";
	}

	using namespace boost::property_tree;

	DocPtr Parser::parseFromFile(const std::string& filename)
	{
		auto doc = Doc::create();
		Parser parser(doc);
		try {
			ptree pt;
			read_xml(filename, pt, xml_parser::no_concat_text);
			parser.parse(pt);
		} catch(ptree_error& e) {
			ASSERT_LOG(false, "Error parsing XHTML: " << e.what() << " : " << filename);
		}
		return doc;
	}

	DocPtr Parser::parseFromString(const std::string& str)
	{
		auto doc = Doc::create();
		Parser parser(doc);
		std::istringstream markup(str);
		try {
			ptree pt;
			read_xml(markup, pt, xml_parser::no_concat_text);
			parser.parse(pt);
		} catch(ptree_error& e) {
			ASSERT_LOG(false, "Error parsing XHTML: " << e.what() << " : " << str);
		}
		return doc;
	}

	Parser::Parser(DocPtr doc)
		: doc_(doc)
	{
	}

	void Parser::parse(const boost::property_tree::ptree& pt)
	{
		auto element = pt.get_child_optional("html");
		if(element) {
			auto ele = Element::factory("html", *element);
			ele->parse(*element);
			//doc_->root = ele;


			// XXX for testing
			ele->preOrderTraverse([](ElementPtr e) { std::cerr << "  Element: " << e->getName() << "\n"; });
		}
		//for(auto& element : pt) {
		//	auto ele = Element::factory(element.first, element.second);
		//	ele->parse(element.second);
		//}
	}
}

