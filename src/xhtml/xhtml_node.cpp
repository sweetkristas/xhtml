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

#include <sstream>

#include "asserts.hpp"
#include "css_parser.hpp"
#include "xhtml_node.hpp"

namespace xhtml
{
	namespace {
		struct DocumentImpl : public Document 
		{
			DocumentImpl(css::StyleSheetPtr ss) : Document(ss) {}
		};

		struct DocumentFragmentImpl : public DocumentFragment
		{
			DocumentFragmentImpl(WeakDocumentPtr owner) : DocumentFragment(owner) {}
		};

		struct TextImpl : public Text
		{
			TextImpl(const std::string& txt, WeakDocumentPtr owner) : Text(txt, owner) {}
		};

		struct AttributeImpl : public Attribute
		{
			AttributeImpl(const std::string& name, const std::string& value, WeakDocumentPtr owner) : Attribute(name, value, owner) {}
		};
	}

	Node::Node(NodeId id, WeakDocumentPtr owner)
		: id_(id),
		  children_(),
		  attributes_(),
		  left_(),
		  right_(),
		  parent_(),
		  owner_document_(owner)
	{
	}

	Node::~Node()
	{
	}
	
	void Node::addChild(NodePtr child)
	{		
		if(child->id() == NodeId::DOCUMENT_FRAGMENT) {
			// we add the children of a document fragment rather than the node itself.
			if(children_.empty()) {
				children_ = child->children_;
				for(auto& c : children_) {
					c->setParent(shared_from_this());
				}
			} else {
				if(!child->children_.empty()) {
					children_.back()->right_ = child->children_.front();
					child->children_.front()->left_ = children_.back();
					for(auto& c : child->children_) {
						c->setParent(shared_from_this());
					}
					children_.insert(children_.end(), child->children_.begin(), child->children_.end());
				}
			}
		} else {
			child->left_ = child->right_ = std::weak_ptr<Node>();
			if(!children_.empty()) {
				children_.back()->right_ = child;
				child->left_ = children_.back();
			}
			children_.emplace_back(child);
			child->setParent(shared_from_this());
		}
	}

	void Node::addAttribute(AttributePtr a)
	{
		a->setParent(shared_from_this());
		attributes_[a->getName()] = a;
	}

	void Node::preOrderTraversal(std::function<bool(NodePtr)> fn)
	{
		// Visit node, visit children.
		if(!fn(shared_from_this())) {
			return;
		}
		for(auto& c : children_) {
			c->preOrderTraversal(fn);
		}
	}

	AttributePtr Node::getAttribute(const std::string& name)
	{
		auto it = attributes_.find(name);
		return it != attributes_.end() ? it->second : nullptr;
	}

	std::string Node::nodeToString() const
	{
		std::ostringstream ss;
		for(auto& a : getAttributes()) {
			ss << "{" << a.second->toString() << "}";
		}
		return ss.str();
	}

	const std::string& Node::getValue() const
	{
		static std::string null_str;
		return null_str;
	}

	void Node::normalize()
	{
		std::vector<NodePtr> new_child_list;
		TextPtr new_text_node;
		for(auto& c : children_) {
			if(c->id() == NodeId::TEXT) {
				if(!c->getValue().empty()) {
					if(new_text_node) {
						new_text_node->addText(c->getValue());
					} else {
						new_text_node = Text::create(c->getValue(), owner_document_);
					}
				}
			} else {
				if(new_text_node) {
					new_child_list.emplace_back(new_text_node);
					new_text_node.reset();
				}
				new_child_list.emplace_back(c);
			}
		}
		if(new_text_node != nullptr) {
			new_child_list.emplace_back(new_text_node);
		}
		children_ = new_child_list;
		for(auto& c : children_) {
			c->normalize();
		}
	}

	// Documents do not have an owner document.
	Document::Document(css::StyleSheetPtr ss)
		: Node(NodeId::DOCUMENT, WeakDocumentPtr()),
		  style_sheet_(ss == nullptr ? std::make_shared<css::StyleSheet>() : ss)
	{
	}

	void Document::processStyles()
	{
		// parse all the style nodes into the style sheet.
		auto ss = style_sheet_;
		preOrderTraversal([&ss](NodePtr n) {
			if(n->hasTag(ElementId::STYLE)) {
				for(auto& child : n->getChildren()) {
					if(child->id() == NodeId::TEXT) {						
						css::Parser::parse(ss, child->getValue());
					}
				}
			}
			return true;
		});
		
		preOrderTraversal([&ss](NodePtr n) {
			ss->applyRulesToElement(n);
			return true;
		});

		// XXX Parse and apply specific element style rules from attributes here.
		preOrderTraversal([](NodePtr n) {
			if(n->id() == NodeId::ELEMENT) {
				auto attr = n->getAttribute("style");
				if(attr) {
					auto plist = css::Parser::parseDeclarationList(attr->getValue());
					css::apply_properties_to_css(n->getStyle(), plist);
				}
			}
			return true;
		});
	}

	std::string Document::toString() const 
	{
		std::ostringstream ss;
		ss << "Document(" << nodeToString() << ")";
		return ss.str();
	}

	DocumentPtr Document::create(css::StyleSheetPtr ss)
	{
		return std::make_shared<DocumentImpl>(ss);
	}

	DocumentFragment::DocumentFragment(WeakDocumentPtr owner)
		: Node(NodeId::DOCUMENT_FRAGMENT, owner)
	{
	}

	DocumentFragmentPtr DocumentFragment::create(WeakDocumentPtr owner)
	{
		return std::make_shared<DocumentFragmentImpl>(owner);
	}

	std::string DocumentFragment::toString() const 
	{
		std::ostringstream ss;
		ss << "DocumentFragment(" << nodeToString() << ")";
		return ss.str();
	}

	Text::Text(const std::string& txt, WeakDocumentPtr owner)
		: Node(NodeId::TEXT, owner),
		  text_(txt)
	{
	}

	TextPtr Text::create(const std::string& txt, WeakDocumentPtr owner)
	{
		return std::make_shared<TextImpl>(txt, owner);
	}

	std::string Text::toString() const 
	{
		std::ostringstream ss;
		ss << "Text('" << text_ << "' " << nodeToString() << ")";
		return ss.str();
	}

	Attribute::Attribute(const std::string& name, const std::string& value, WeakDocumentPtr owner)
		: Node(NodeId::ATTRIBUTE, owner),
		  name_(name),
		  value_(value)
	{
	}

	AttributePtr Attribute::create(const std::string& name, const std::string& value, WeakDocumentPtr owner)
	{
		return std::make_shared<AttributeImpl>(name, value, owner);
	}

	std::string Attribute::toString() const 
	{
		std::ostringstream ss;
		ss << "Attribute('" << name_ << ":" << value_ << "'" << nodeToString() << ")";
		return ss.str();
	}
}
