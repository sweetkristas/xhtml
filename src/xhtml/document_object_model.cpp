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

#include "document_object_model.hpp"

namespace dom
{
	namespace
	{
		std::string internal_create_qname(const std::string& namespace_uri, const std::string& localname)
		{
			return namespace_uri + ":" + localname;
		}
	}

	NamedNodeMap::NamedNodeMap(std::weak_ptr<Document> owner)
		: map_(),
		  owner_document_(owner)
	{
	}

	NamedNodeMap::NamedNodeMap(const NamedNodeMap& m, bool deep)
		: map_(),
		  owner_document_(m.owner_document_)
	{
		if(deep) {
			for(auto& item : m.map_) {
				map_[item.first] = item.second->cloneNode(true);
			}
		} else {
			map_ = m.map_;
		}
	}

	NodePtr NamedNodeMap::getNamedItem(const std::string& name) const
	{
		auto it = map_.find(name);
		if(it == map_.end()) {
			return nullptr;
		}
		return it->second;
	}

	NodePtr NamedNodeMap::setNamedItem(NodePtr node)
	{
		// XXX INUSE_ATTRIBUTE_ERR: Raised if arg is an Attr that is already an attribute of another Element object. 
		// The DOM user must explicitly clone Attr nodes to re-use them in other elements.
		if(node == nullptr) {
			return nullptr;
		}
		if(node->getOwnerDocument() != getOwnerDocument()) {
			throw Exception(ExceptionCode::WRONG_DOCUMENT_ERR);
		}
		if(isReadOnly()) {
			throw Exception(ExceptionCode::NO_MODIFICATION_ALLOWED_ERR);
		}
		auto it = map_.find(node->getNodeName());
		NodePtr res = node;
		if(it != map_.end()) {
			res = it->second;
		}
		map_[node->getNodeName()] = node;
		return res;
	}

	NodePtr NamedNodeMap::removeNamedItem(const std::string& name)
	{
		if(isReadOnly()) {
			throw Exception(ExceptionCode::NO_MODIFICATION_ALLOWED_ERR);
		}
		auto it = map_.find(name);
		if(it == map_.end()) {
			throw Exception(ExceptionCode::NOT_FOUND_ERR);
		}
		NodePtr res = it->second;
		map_.erase(it);
		return res;
	}

	NodePtr NamedNodeMap::getItem(int n) const
	{
		if(n >= map_.size()) {
			return nullptr;
		}
		auto it = map_.begin();
		std::advance(it, n);
		if(it == map_.end()) {
			return nullptr;
		}
		return it->second;
	}

	NodePtr NamedNodeMap::getNamedItemNS(const std::string namespace_uri, std::string& name) const
	{
		// we store namepsace in the map as namespaceuri + ":" + localname
		auto it = map_.find(internal_create_qname(namespace_uri, name));
		if(it == map_.end()) {
			return nullptr;
		}
		return it->second;
	}

	NodePtr NamedNodeMap::setNamedItemNS(NodePtr node) const
	{
		// XXX INUSE_ATTRIBUTE_ERR: Raised if arg is an Attr that is already an attribute of another Element object. 
		// The DOM user must explicitly clone Attr nodes to re-use them in other elements.
		if(node == nullptr) {
			return nullptr;
		}
		if(node->getOwnerDocument() != getOwnerDocument()) {
			throw Exception(ExceptionCode::WRONG_DOCUMENT_ERR);
		}
		if(isReadOnly()) {
			throw Exception(ExceptionCode::NO_MODIFICATION_ALLOWED_ERR);
		}
		std::string qualified_name = internal_create_qname(node->getNamespaceURI(), node->getLocalName());
		auto it = map_.find(qualified_name);
		NodePtr res = node;
		if(it != map_.end()) {
			res = it->second;
		}
		map_[qualified_name] = node;
		return res;
	}

	NodePtr NamedNodeMap::removeNamedItemNS(const std::string namespace_uri, std::string& name)
	{
		if(isReadOnly()) {
			throw Exception(ExceptionCode::NO_MODIFICATION_ALLOWED_ERR);
		}
		auto it = map_.find(internal_create_qname(namespace_uri, name));
		if(it == map_.end()) {
			throw Exception(ExceptionCode::NOT_FOUND_ERR);
		}
		NodePtr res = it->second;
		map_.erase(it);
		return res;
	}

	DocumentPtr NamedNodeMap::getOwnerDocument() const
	{
		return owner_document_.lock();
	}

	Node::Node(NodeType type, const std::string& name, std::weak_ptr<Document> owner)
		: type_(type),
		  name_(name),
		  owner_document_(owner),
		  parent_(),
		  left_(),
		  right_(),
		  children_()
	{
	}

	Node::Node(const Node& node, bool deep)
		: type_(node.type_),
		  name_(node.name_),
		  owner_document_(node.owner_document_),
		  parent_(),
		  left_(nullptr),
		  right_(nullptr),
		  children_()
	{
		if(deep) {
			auto it = node.children_.front();
			while(it != nullptr) {
				children_.appendChild(it->cloneNode(deep));
			}
		} else {
			children_ = node.children_;
		}
	}
	
	void Node::setParent(std::weak_ptr<Node> parent)
	{
		parent_ = parent;
	}

	DocumentPtr Node::getOwnerDocument() const
	{
		return owner_document_.lock();
	}

	NodePtr Node::getParentNode() const
	{
		return parent_.lock();
	}

	NodePtr Node::getFirstChild() const
	{
		return children_.front();
	}

	NodePtr Node::getLastChild() const
	{
		return children_.back();
	}

	NodePtr Node::getPreviousSibling() const
	{
		return left_;
	}

	NodePtr Node::getNextSibling() const
	{
		return right_;
	}

	NodePtr Node::insertBefore(NodePtr new_child, NodePtr ref_child)
	{
		return children_.insertBefore(new_child, ref_child);
	}

	NodePtr Node::replaceChild(NodePtr new_child, NodePtr old_child)
	{
		return children_.replaceChild(new_child, old_child);
	}

	NodePtr Node::removeChild(NodePtr new_child)
	{
		return children_.removeChild(new_child);
	}

	NodePtr Node::appendChild(NodePtr new_child)
	{
		return children_.appendChild(new_child);
	}

	NodePtr Node::cloneNode(bool deep) const
	{
		return std::make_shared<Node>(*this);
	}

	void Node::normalize()
	{
		// XXX
	}

	bool Node::isSupported(const std::string& feature, const std::string& version) const
	{
		auto doc = getOwnerDocument();
		if(doc != nullptr) {
			return doc->getImplementation()->hasFeature(feature, version);
		}
		return false;
	}

	void Node::getElementsByTagName(NodeList* nl, const std::string& tagname) const
	{
		// pre-order traversal to extract all children with the given tagname.
		// * is the special "any" tag.
		if(tagname == getNodeName() || (tagname == "*" && type() == NodeType::ELEMENT_NODE)) {
			nl->appendChild(get_this_ptr());
		}
		for(auto& c : getChildNodes()) {
			c->getElementsByTagName(nl, tagname);
		}
	}


	/*NodeList::NodeList(bool read_only)
		: head_(),
		  tail_(),
		  count_(0),
		  read_only_(read_only)
	{
	}

	NodePtr NodeList::insertBefore(NodePtr new_node, NodePtr ref_node)
	{
		if(isReadOnly()) {
			throw Exception(ExceptionCode::NO_MODIFICATION_ALLOWED_ERR);
		}
		// check if new_node is already in tree, remove it if it is.
		auto it = head_;
		while(it != nullptr) {
			if(it == new_node) {
				removeChild(new_node);
				it = nullptr;
			} else {
				it = it->getNextSibling();
			}
		}
		// XXX we should have a parent list pointer, so we can track that old_child is in this list.
		if(ref_node == nullptr) {
			appendChild(new_node);
		} else {
			if(new_node->type() == NodeType::DOCUMENT_FRAGMENT_NODE) {
				auto& children = new_node->getChildNodes();
				if(head_ == ref_node) {
					head_ = children.head_;
				}
				if(ref_node->getPreviousSibling() != nullptr) {
					ref_node->getPreviousSibling()->getNextSibling() = children.tail_;
				}
				children.head_->getPreviousSibling() = ref_node->getPreviousSibling();
				ref_node->getPreviousSibling() = children.tail_;
				children.tail_->getNextSibling() = ref_node;
				count_ += children.count_;
				children.clear();
			} else {
				if(head_ == ref_node) {
					head_ = new_node;
				}
				if(ref_node->getPreviousSibling() != nullptr) {
					ref_node->getPreviousSibling()->getNextSibling() = new_node;
				}
				new_node->getPreviousSibling() = ref_node->getPreviousSibling();
				ref_node->getPreviousSibling() = new_node;
				new_node->getNextSibling() = ref_node;
				++count_;
			}
		}
		return new_node;
	}

	NodePtr NodeList::replaceChild(NodePtr new_node, NodePtr old_node)
	{
		if(isReadOnly()) {
			throw Exception(ExceptionCode::NO_MODIFICATION_ALLOWED_ERR);
		}
		// check if new_node is already in tree, remove it if it is.
		auto it = head_;
		while(it != nullptr) {
			if(it == new_node) {
				removeChild(new_node);
				it = nullptr;
			} else {
				it = it->getNextSibling();
			}
		}
		removeChild(old_node);
		appendChild(new_node);
	}

	NodePtr NodeList::removeChild(NodePtr old_child)
	{
		// XXX we should have a parent list pointer, so we can track that old_child is in this list.
		if(isReadOnly()) {
			throw Exception(ExceptionCode::NO_MODIFICATION_ALLOWED_ERR);
		}
		if(head_ == old_child) {
			head_ = head_->getNextSibling();
		}
		if(tail_ == old_child) {
			tail_ = tail_->getPreviousSibling();
		}
		if(old_child->getNextSibling() != nullptr) {
			old_child->getNextSibling()->getPreviousSibling() = old_child->getPreviousSibling();
		}
		if(old_child->getPreviousSibling() != nullptr) {
			old_child->getPreviousSibling()->getNextSibling() = old_child->getNextSibling();
		}
		old_child->getNextSibling() = old_child->getPreviousSibling() = nullptr;
		--count_;
		return old_child;
	}

	NodePtr NodeList::appendChild(NodePtr new_child)
	{
		if(isReadOnly()) {
			throw Exception(ExceptionCode::NO_MODIFICATION_ALLOWED_ERR);
		}
		if(new_child->type() == NodeType::DOCUMENT_FRAGMENT_NODE) {
			// document fragments move their entire contents into the list.
			if(head_ == nullptr) {
				*this = new_child->getChildNodes();
				new_child->getChildNodes().clear();
			} else {
				auto& children = new_child->getChildNodes();
				tail_->getNextSibling() = children.head_;
				children.head_->getPreviousSibling() = tail_;
				tail_ = children.tail_;
				count_ += children.count_;
				children.clear();
			}
		} else {
			if(head_ == nullptr) {
				tail_ = head_ = new_child;
				count_ = 1;
			} else {
				tail_->getNextSibling() = new_child;
				new_child->getPreviousSibling() = tail_;
				tail_ = new_child;
				new_child->getNextSibling() = nullptr;
				++count_;
			}
		}
	}

	NodePtr NodeList::getItem(int n) const
	{
		if(n < 0 || n >= count_) {
			return nullptr;
		}
		auto it = head_;
		while(n-- > 0) {
			it = it->getNextSibling();
		}
		return it;
	}

	void NodeList::clear()
	{
		// This is a dangerous function as it does not release the links between nodes.
		// Use with care only after extracting the contents to somewhere else.
		count_ = 0;
		head_ = tail_ = nullptr;
	}

	NodeList::~NodeList()
	{
		auto it = head_;
		while(it != nullptr) {
			NodePtr next = it->getNextSibling();
			it->getNextSibling() = it->getPreviousSibling() = nullptr;
			it = next;
		}
		head_ = tail_ = nullptr;
		count_ = 0;
	}*/

	NodeList::NodeList(bool read_only=false)
		: nodes_(),
		  read_only_(read_only)
	{
	}

	NodePtr NodeList::getItem(int n) const
	{
		if(n < 0 || n >= static_cast<int>(nodes_.size())) {
			return nullptr;
		}
		return nodes_[n];
	}

	NodePtr NodeList::insertBefore(NodePtr new_node, NodePtr ref_node)
	{
		// XXX
	}

	NodePtr NodeList::replaceChild(NodePtr new_node, NodePtr old_node)
	{
		// XXX
	}

	NodePtr NodeList::removeChild(NodePtr new_child)
	{
		// XXX
	}

	NodePtr NodeList::appendChild(NodePtr new_child)
	{
		// XXX
	}

	Notation::Notation(const std::string& name, const std::string& public_id, const std::string& system_id, std::weak_ptr<Document> owner)
		: Node(NodeType::NOTATION_NODE, name, owner),
		  public_id_(public_id),
		  system_id_(system_id)
	{
	}

	NodePtr Notation::cloneNode(bool deep) const 
	{
		return std::make_shared<Notation>(*this);
	}

	// XXX i'm don't think the notation name should be the node name.
	Entity::Entity(const std::string& public_id, const std::string& system_id, const std::string& notation_name, std::weak_ptr<Document> owner)
		: Node(NodeType::ENTITY_NODE, notation_name, owner),
		  public_id_(public_id),
		  system_id_(system_id),
		  notation_name_(notation_name)
	{
	}

	NodePtr Entity::cloneNode(bool deep) const
	{
		return std::make_shared<Entity>(*this, deep);
	}

	ProcessingInstruction::ProcessingInstruction(const std::string& target, const std::string& data, std::weak_ptr<Document> owner)
		: Node(NodeType::PROCESSING_INSTRUCTION_NODE, target, owner),
		  data_(data)
	{
	}

	NodePtr ProcessingInstruction::cloneNode(bool deep) const
	{
		return std::make_shared<ProcessingInstruction>(*this, deep);
	}

	Attribute::Attribute(const std::string& name, const std::string& value, std::weak_ptr<Document> owner)
		: Node(NodeType::ATTRIBUTE_NODE, name, owner),
		  value_(value),
		  specified_(value.empty() ? false : true),
		  prefix_(),
		  local_name_(),
		  namespace_uri_()
	{
	}

	NodePtr Attribute::cloneNode(bool deep) const 
	{
		return std::make_shared<Attribute>(*this, deep);
	}

	Element::Element(const std::string& tagname, std::weak_ptr<Document> owner)
		: Node(NodeType::ELEMENT_NODE, tagname, owner),
		  attributes_(),
		  prefix_(),
		  local_name_(),
		  namespace_uri_()
	{
	}

	Element::Element(const Element& e, bool deep)
		: Node(e, deep),
		  attributes_(e.attributes_, deep),
		  prefix_(e.prefix_),
		  local_name_(e.local_name_),
		  namespace_uri_(e.namespace_uri_)
	{
	}

	NodePtr Element::cloneNode(bool deep) const
	{
		return std::make_shared<Element>(*this, deep);
	}

	const std::string& Element::getAttribute(const std::string& name) const
	{
		auto attr = attributes_.getNamedItem(name);
		if(attr != nullptr) {
			return attr->getNodeValue();
		}
		return "";
	}

	void Element::setAttribute(const std::string& name, const std::string& value)
	{
		auto attr = getOwnerDocument()->createAttribute(name);
		attr->setNodeValue(value);
		attributes_.setNamedItem(attr);
	}

	void Element::removeAttribute(const std::string& name)
	{
		attributes_.removeNamedItem(name);
	}

	AttributePtr Element::getAttributeNode(const std::string& name) const
	{
		auto attr = attributes_.getNamedItem(name);
		return std::dynamic_pointer_cast<Attribute>(attr);
	}

	AttributePtr Element::setAttributeNode(AttributePtr attr)
	{
		auto attr = attributes_.setNamedItem(attr);
		return std::dynamic_pointer_cast<Attribute>(attr);
	}

	AttributePtr Element::removeAttributeNode(AttributePtr attr)
	{
		auto res = attributes_.removeNamedItem(attr->getNodeName());
		// XXX If the remove attribute has a default value then replace it with that.
		return std::dynamic_pointer_cast<Attribute>(res);
	}

	NodeList Element::getElementsByTagName(const std::string& tagname) const
	{
		NodeList node_list;
		Node::getElementsByTagName(&node_list, tagname);
		return node_list;
	}

	const std::string& Element::getAttributeNS(const std::string& namespaceuri, const std::string& localname) const
	{
	}

	void Element::setAttributeNS(const std::string& namespaceuri, const std::string& qualifiedname, const std::string& value)
	{
	}

	void Element::removeAttributeNS(const std::string& namespaceuri, const std::string& localname)
	{
	}

	AttributePtr Element::getAttributeNodeNS(const std::string& namespaceuri, const std::string& localname) const
	{
	}

	AttributePtr Element::setAttributeNodeNS(AttributePtr attr)
	{
	}

	NodeList Element::getElementsByTagNameNS(const std::string& namespaceuri, const std::string& localname) const
	{
	}

	bool Element::hasAttribute(const std::string& name)
	{
	}

	bool Element::hasAttributeNS(const std::string& namespaceuri, const std::string& localname)
	{
	}

}
