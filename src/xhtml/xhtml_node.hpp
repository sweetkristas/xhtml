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

#include <functional>
#include <map>
#include <vector>

#include "xhtml.hpp"
#include "xhtml_element_id.hpp"
#include "css_stylesheet.hpp"

namespace xhtml
{
	enum class NodeId {
		DOCUMENT,
		ELEMENT,
		ATTRIBUTE,
		DOCUMENT_FRAGMENT,
		TEXT,
	};

	typedef std::map<std::string, AttributePtr> AttributeMap;
	typedef std::vector<NodePtr> NodeList;

	class Node : std::enable_shared_from_this<Node>
	{
	public:
		explicit Node(NodeId id, WeakDocumentPtr owner);
		virtual ~Node();
		NodeId id() const { return id_; }
		void addChild(NodePtr child);
		void addAttribute(AttributePtr a);
		NodePtr getLeft() const { return left_.lock(); }
		NodePtr getRight() const { return right_.lock(); }
		NodePtr getParent() const { return parent_.lock(); }
		void setParent(WeakNodePtr p) { parent_ = p; }
		DocumentPtr getOwnerDoc() const { return owner_document_.lock(); }
		virtual std::string toString() const = 0;
		const AttributeMap& getAttributes() const { return attributes_; }
		const NodeList& getChildren() const { return children_; }
		void preOrderTraversal(std::function<bool(NodePtr)> fn);
		virtual bool hasTag(const std::string& tag) const { return false; }
		virtual bool hasTag(ElementId tag) const { return false; }
		AttributePtr getAttribute(const std::string& name);
		virtual const std::string& getValue() const;
		virtual css::CssStyles* getStyle() { return nullptr; }
		void normalize();
		//virtual rect getBoundingBox() const = 0;
	protected:
		std::string nodeToString() const;
	private:
		Node() = delete;
		NodeId id_;
		NodeList children_;
		AttributeMap attributes_;

		WeakNodePtr left_, right_;
		WeakNodePtr parent_;

		WeakDocumentPtr owner_document_;
	};

	class Document : public Node
	{
	public:
		static DocumentPtr create(css::StyleSheetPtr ss=nullptr);
		std::string toString() const override;
		void processStyles();
	protected:
		Document(css::StyleSheetPtr ss);
		css::StyleSheetPtr style_sheet_;
	};

	class DocumentFragment : public Node
	{
	public:
		static DocumentFragmentPtr create(WeakDocumentPtr owner=WeakDocumentPtr());
		std::string toString() const override;
	protected:
		DocumentFragment(WeakDocumentPtr owner);
	};

	class Text : public Node
	{
	public:
		static TextPtr create(const std::string& txt, WeakDocumentPtr owner=WeakDocumentPtr());
		void addText(const std::string& txt) { text_ += txt; }
	protected:
		explicit Text(const std::string& txt, WeakDocumentPtr owner);
		std::string toString() const override;
		const std::string& getValue() const override { return text_; }
	private:
		std::string text_;
	};

	class Attribute : public Node
	{
	public:
		static AttributePtr create(const std::string& name, const std::string& value, WeakDocumentPtr owner=WeakDocumentPtr());
		const std::string& getName() const { return name_; }
		const std::string& getValue() const { return value_; }
		std::string toString() const override;
	protected:
		explicit Attribute(const std::string& name, const std::string& value, WeakDocumentPtr owner);
	private:
		std::string name_;
		std::string value_;
	};
}
