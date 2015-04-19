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
		const std::string null_str = "";

		std::string internal_create_qname(const std::string& namespace_uri, const std::string& localname)
		{
			return namespace_uri + ":" + localname;
		}
	}

	NamedNodeMap::NamedNodeMap()
		: map_(),
		  owner_document_()
	{
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
		if(n >= static_cast<int>(map_.size())) {
			return nullptr;
		}
		auto it = map_.begin();
		std::advance(it, n);
		if(it == map_.end()) {
			return nullptr;
		}
		return it->second;
	}

	NodePtr NamedNodeMap::getNamedItemNS(const std::string namespace_uri, const std::string& name) const
	{
		// we store namepsace in the map as namespaceuri + ":" + localname
		auto it = map_.find(internal_create_qname(namespace_uri, name));
		if(it == map_.end()) {
			return nullptr;
		}
		return it->second;
	}

	NodePtr NamedNodeMap::setNamedItemNS(NodePtr node)
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

	NodePtr NamedNodeMap::removeNamedItemNS(const std::string namespace_uri, const std::string& name)
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
		  left_(),
		  right_(),
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
		return left_.lock();
	}

	NodePtr Node::getNextSibling() const
	{
		return right_.lock();
	}

	NodePtr Node::insertAfter(NodePtr new_child, NodePtr ref_child)
	{
		if(new_child->getOwnerDocument() != getOwnerDocument()) {
			throw Exception(ExceptionCode::WRONG_DOCUMENT_ERR);
		}
		if(ref_child->isReadOnly() || isReadOnly()) {
			throw Exception(ExceptionCode::NO_MODIFICATION_ALLOWED_ERR);
		}
		return children_.insertAfter(new_child, ref_child);
	}

	NodePtr Node::insertBefore(NodePtr new_child, NodePtr ref_child)
	{
		if(new_child->getOwnerDocument() != getOwnerDocument()) {
			throw Exception(ExceptionCode::WRONG_DOCUMENT_ERR);
		}
		if(ref_child->isReadOnly() || isReadOnly()) {
			throw Exception(ExceptionCode::NO_MODIFICATION_ALLOWED_ERR);
		}
		return children_.insertBefore(new_child, ref_child);
	}

	NodePtr Node::replaceChild(NodePtr new_child, NodePtr old_child)
	{
		if(new_child->getOwnerDocument() != getOwnerDocument()) {
			throw Exception(ExceptionCode::WRONG_DOCUMENT_ERR);
		}
		if(old_child->isReadOnly() || isReadOnly()) {
			throw Exception(ExceptionCode::NO_MODIFICATION_ALLOWED_ERR);
		}
		return children_.replaceChild(new_child, old_child);
	}

	NodePtr Node::removeChild(NodePtr new_child)
	{
		if(new_child->isReadOnly() || isReadOnly()) {
			throw Exception(ExceptionCode::NO_MODIFICATION_ALLOWED_ERR);
		}
		return children_.removeChild(new_child);
	}

	NodePtr Node::appendChild(NodePtr new_child)
	{
		if(new_child->isReadOnly() || isReadOnly()) {
			throw Exception(ExceptionCode::NO_MODIFICATION_ALLOWED_ERR);
		}
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

	void Node::getElementsByTagName(NodeList* nl, const std::string& tagname)
	{
		// pre-order traversal to extract all children with the given tagname.
		// * is the special "any" tag.
		if(tagname == getNodeName() || (tagname == "*" && type() == NodeType::ELEMENT_NODE)) {
			nl->appendChild(shared_from_this());
		}
		for(auto& c : getChildNodes()) {
			c->getElementsByTagName(nl, tagname);
		}
	}

	void Node::getElementsByTagNameNS(NodeList* nl, const std::string& namespaceuri, const std::string& localname)
	{
		if((namespaceuri == getNamespaceURI() && localname == getLocalName())
			|| (namespaceuri == "*" && getLocalName() == localname) 
			|| (namespaceuri == getNamespaceURI() && localname == "*")
			|| (namespaceuri == "*" && localname == "*")) {
			nl->appendChild(shared_from_this());
		}
		for(auto& c : getChildNodes()) {
			c->getElementsByTagNameNS(nl, namespaceuri, localname);
		}
	}

	const std::string& Node::getNamespaceURI() const 
	{ 
		return null_str;
	}
	
	const std::string& Node::getPrefix() const 
	{
		return null_str;
	}
	
	const std::string& Node::getLocalName() const 
	{ 
		return null_str; 
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

	NodeList::NodeList(bool read_only)
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

	NodePtr NodeList::insertAfter(NodePtr new_node, NodePtr ref_node)
	{
		auto it = nodes_.begin();
		for(; it != nodes_.end(); ++it) {
			if(*it == ref_node) {
				// XXX handle DocumentFragment nodes.
				if(ref_node->getNextSibling()) {
					ref_node->getNextSibling()->setPreviousSibling(new_node);
				}
				new_node->setNextSibling(ref_node->getNextSibling());
				ref_node->setNextSibling(new_node);
				new_node->setPreviousSibling(ref_node);
				nodes_.insert(it+1, new_node);
				return new_node;
			}
		}
		throw Exception(ExceptionCode::NOT_FOUND_ERR);
	}

	NodePtr NodeList::insertBefore(NodePtr new_node, NodePtr ref_node)
	{
		auto it = nodes_.begin();
		for(; it != nodes_.end(); ++it) {
			if(*it == ref_node) {
				// XXX handle DocumentFragment nodes.
				if(ref_node->getPreviousSibling()) {
					ref_node->getPreviousSibling()->setNextSibling(new_node);
				}
				new_node->setPreviousSibling(ref_node->getPreviousSibling());
				ref_node->setPreviousSibling(new_node);
				new_node->setNextSibling(ref_node);
				nodes_.insert(it, new_node);
				return new_node;
			}
		}
		throw Exception(ExceptionCode::NOT_FOUND_ERR);
	}

	NodePtr NodeList::replaceChild(NodePtr new_node, NodePtr old_node)
	{
		auto it = nodes_.begin();
		for(; it != nodes_.end(); ++it) {
			if(*it == old_node) {
				// XXX handle DocumentFragment nodes.
				if(old_node->getPreviousSibling()) {
					old_node->getPreviousSibling()->setNextSibling(new_node);
				}
				if(old_node->getNextSibling()) {
					old_node->getNextSibling()->setPreviousSibling(new_node);
				}
				new_node->setNextSibling(old_node->getNextSibling());
				new_node->setPreviousSibling(old_node->getPreviousSibling());
				*it = new_node;
				old_node->setNextSibling(std::weak_ptr<Node>());
				old_node->setPreviousSibling(std::weak_ptr<Node>());
				return old_node;
			}
		}
		throw Exception(ExceptionCode::NOT_FOUND_ERR);
	}

	NodePtr NodeList::removeChild(NodePtr old_node)
	{
		auto it = nodes_.begin();
		for(; it != nodes_.end(); ++it) {
			if(*it == old_node) {
				if(old_node->getPreviousSibling()) {
					old_node->getPreviousSibling()->setNextSibling(old_node->getNextSibling());
				}
				if(old_node->getNextSibling()) {
					old_node->getNextSibling()->setPreviousSibling(old_node->getPreviousSibling());
				}
				old_node->setNextSibling(std::weak_ptr<Node>());
				old_node->setPreviousSibling(std::weak_ptr<Node>());
				nodes_.erase(it);
				return old_node;
			}
		}
		throw Exception(ExceptionCode::NOT_FOUND_ERR);
	}

	NodePtr NodeList::appendChild(NodePtr new_child)
	{
		// XXX handle DocumentFragment nodes.
		nodes_.back()->setNextSibling(new_child);
		new_child->setPreviousSibling(nodes_.back());
		nodes_.emplace_back(new_child);
		return new_child;
	}
	
	NodeList::~NodeList()
	{
		for(auto& n : nodes_) {
			n->setNextSibling(std::weak_ptr<Node>());
			n->setPreviousSibling(std::weak_ptr<Node>());
		}
	}

	Notation::Notation(const std::string& name, const std::string& public_id, const std::string& system_id, std::weak_ptr<Document> owner)
		: Node(NodeType::NOTATION_NODE, name, owner),
		  public_id_(public_id),
		  system_id_(system_id)
	{
	}

	Notation::Notation(const Notation& node, bool deep)
		: Node(node, deep),
		  public_id_(node.public_id_),
		  system_id_(node.system_id_)
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

	Entity::Entity(const Entity& node, bool deep)
		: Node(node, deep),
		  public_id_(node.public_id_),
		  system_id_(node.system_id_),
		  notation_name_(node.notation_name_)
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

	ProcessingInstruction::ProcessingInstruction(const ProcessingInstruction& node, bool deep)
		: Node(node, deep),
		  data_(node.data_)
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

	Attribute::Attribute(const Attribute& node, bool deep)
		: Node(node, deep),
		  value_(node.value_),
		  specified_(true),
		  prefix_(node.prefix_),
		  local_name_(node.local_name_),
		  namespace_uri_(node.namespace_uri_)
	{
	}

	NodePtr Attribute::cloneNode(bool deep) const 
	{
		return std::make_shared<Attribute>(*this, deep);
	}

	Element::Element(const std::string& tagname, std::weak_ptr<Document> owner)
		: Node(NodeType::ELEMENT_NODE, tagname, owner),
		  attributes_(getOwnerDocument()),
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
		static std::string null_str;
		return null_str;
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
		auto attr_node = attributes_.setNamedItem(attr);
		return std::dynamic_pointer_cast<Attribute>(attr_node);
	}

	AttributePtr Element::removeAttributeNode(AttributePtr attr)
	{
		auto res = attributes_.removeNamedItem(attr->getNodeName());
		// XXX If the remove attribute has a default value then replace it with that.
		return std::dynamic_pointer_cast<Attribute>(res);
	}

	NodeList Element::getElementsByTagName(const std::string& tagname)
	{
		NodeList node_list;
		Node::getElementsByTagName(&node_list, tagname);
		return node_list;
	}

	const std::string& Element::getAttributeNS(const std::string& namespaceuri, const std::string& localname) const
	{
		auto res = attributes_.getNamedItemNS(namespaceuri, localname);
		if(res) {
			return res->getNodeValue();
		}
		// return a default value if any
		return null_str;
	}

	void Element::setAttributeNS(const std::string& namespaceuri, const std::string& qualifiedname, const std::string& value)
	{
		if((qualifiedname == "xml" && namespaceuri != "http://www.w3.org/XML/1998/namespace") 
			|| (qualifiedname == "xmlns" && namespaceuri != "http://www.w3.org/2000/xmlns/")
			|| (!qualifiedname.empty() && namespaceuri.empty())) {
			throw Exception(ExceptionCode::NAMESPACE_ERR);
		}
		auto attr = getOwnerDocument()->createAttributeNS(namespaceuri, qualifiedname);
		attr->setNodeValue(value);
		attributes_.setNamedItemNS(attr);
	}

	void Element::removeAttributeNS(const std::string& namespaceuri, const std::string& localname)
	{
		attributes_.removeNamedItemNS(namespaceuri, localname);
	}

	AttributePtr Element::getAttributeNodeNS(const std::string& namespaceuri, const std::string& localname) const
	{
		auto res = attributes_.getNamedItemNS(namespaceuri, localname);
		return std::dynamic_pointer_cast<Attribute>(res);
	}

	AttributePtr Element::setAttributeNodeNS(AttributePtr attr)
	{
		attributes_.setNamedItemNS(attr);
		return attr;
	}

	NodeList Element::getElementsByTagNameNS(const std::string& namespaceuri, const std::string& localname)
	{
		NodeList node_list;
		Node::getElementsByTagNameNS(&node_list, namespaceuri, localname);
		return node_list;
	}

	bool Element::hasAttribute(const std::string& name)
	{
		auto res = attributes_.getNamedItem(name);
		return res != nullptr ? true : false;
	}

	bool Element::hasAttributeNS(const std::string& namespaceuri, const std::string& localname)
	{
		auto res = attributes_.getNamedItemNS(namespaceuri, localname);
		return res != nullptr ? true : false;
	}

	CharacterData::CharacterData(const std::string& data)
		: data_(data)
	{
	}

	CharacterData::CharacterData(const CharacterData& cd, bool deep)
		: data_(cd.data_)
	{
	}

	std::string CharacterData::substring(int offset, int count)
	{
		if(offset < 0 || offset >= static_cast<int>(data_.size()) || count < 0) {
			throw Exception(ExceptionCode::INDEX_SIZE_ERR);
		}
		return data_.substr(offset, count);
	}

	void CharacterData::append(const std::string& arg)
	{
		data_ += arg;
	}

	void CharacterData::insert(int offset, const std::string& arg)
	{
		if(offset < 0 || offset >= static_cast<int>(data_.size())) {
			throw Exception(ExceptionCode::INDEX_SIZE_ERR);
		}
		data_.insert(offset, arg);
	}

	void CharacterData::replace(int offset, int count, const std::string& arg)
	{
		if(offset < 0 || offset >= static_cast<int>(data_.size()) || count < 0) {
			throw Exception(ExceptionCode::INDEX_SIZE_ERR);
		}
		if(offset + count >= static_cast<int>(data_.size())) {
			data_ = arg;
		} else {
			data_ = data_.substr(0, offset) + arg + data_.substr(offset + count);
		}
	}

	Text::Text(const std::string& data, std::weak_ptr<Document> owner)
		: Node(NodeType::TEXT_NODE, "#text", owner),
		  CharacterData(data)
	{
	}

	Text::Text(NodeType type, const std::string& name, const std::string& data, std::weak_ptr<Document> owner)
		: Node(type, name, owner),
		  CharacterData(data)
	{
	}

	Text::Text(const Text& node, bool deep)
		: Node(node, deep),
		  CharacterData(node, deep)
	{
	}

	NodePtr Text::cloneNode(bool deep) const
	{
		return std::make_shared<Text>(*this, deep);
	}

	TextPtr Text::splitText(int offset)
	{
		auto s = substring(offset, getData().size()-offset);
		setData(substring(0, offset));
		auto new_text_node = std::make_shared<Text>(s, getOwnerDocument());
		auto parent = getParentNode();
		if(parent) {
			parent->insertAfter(shared_from_this(), new_text_node);
		}
		return new_text_node;
	}

	Comment::Comment(const std::string& data, std::weak_ptr<Document> owner)
		: Node(NodeType::COMMENT_NODE, "#comment", owner),
		  CharacterData(data)
	{
	}

	Comment::Comment(const Comment& node, bool deep)
		: Node(node, deep),
		  CharacterData(node, deep)
	{
	}

	NodePtr Comment::cloneNode(bool deep) const
	{
		return std::make_shared<Comment>(*this, deep);
	}

	CDATASection::CDATASection(const std::string& data, std::weak_ptr<Document> owner)
		: Text(NodeType::CDATA_SECTION_NODE, "#cdata-section", data, owner)
	{
	}

	CDATASection::CDATASection(const CDATASection& node, bool deep)
		: Text(node, deep)
	{
	}

	NodePtr CDATASection::cloneNode(bool deep) const
	{
		return std::make_shared<CDATASection>(*this, deep);
	}

	DocumentType::DocumentType(const std::string& name, const std::string& public_id, const std::string& system_id)
		: name_(name),
		  entities_(),
		  notations_(),
		  public_id_(public_id),
		  system_id_(system_id),
		  internal_subset_()
	{
	}

	Implementation::Implementation()
	{
	}

	bool Implementation::hasFeature(const std::string& feature, const std::string& version) const
	{
		// XXX
		return false;
	}

	DocumentTypePtr Implementation::createDocumentType(const std::string& qualified_name, const std::string& public_id, const std::string& system_id)
	{
		return std::unique_ptr<DocumentType>(new DocumentType(qualified_name, public_id, system_id));
	}

	DocumentPtr Implementation::createDocument(const std::string& namespace_uri, const std::string& qualified_name, DocumentTypePtr doctype)
	{
		return nullptr;
	}

	Document::Document()
		: Node(NodeType::DOCUMENT_NODE, "#document", std::weak_ptr<Document>()),
		  doctype_(),
		  implementation_(),
		  element_()
	{
	}

	ElementPtr Document::createElement(const std::string& tagname)
	{
		// XXX
		return nullptr;
	}

	NodePtr Document::createDocumentFragment()
	{
		// XXX
		return nullptr;
	}

	TextPtr Document::createTextNode(const std::string& data)
	{
		// XXX
		return nullptr;
	}

	CommentPtr Document::createComment(const std::string& data)
	{
		// XXX
		return nullptr;
	}

	CDATASectionPtr Document::createCDATASection(const std::string& data)
	{
		// XXX
		return nullptr;
	}

	ProcessingInstructionPtr Document::createProcessingInstruction(const std::string& target, const std::string& data)
	{
		// XXX
		return nullptr;
	}

	AttributePtr Document::createAttribute(const std::string& name)
	{
		return std::make_shared<Attribute>(name, "", get_this_ptr());
	}

	NodePtr Document::createEntityReference(const std::string& name)
	{
		// XXX
		return nullptr;
	}

	NodeList Document::getElementsByTagName(const std::string& tagname)
	{
		// XXX
		return nullptr;
	}

	NodePtr Document::importNode(NodePtr imported_node, bool deep)
	{
		// XXX
		return nullptr;
	}

	ElementPtr Document::createElementNS(const std::string& namespaceuri, const std::string& qualified_name)
	{
		// XXX
		return nullptr;
	}

	AttributePtr Document::createAttributeNS(const std::string& namespaceuri, const std::string& qualified_name)
	{
		// XXX
		return nullptr;
	}

	NodeList Document::getElementsByTagNameNS(const std::string& namespaceuri, const std::string& localname)
	{
		// XXX
		return nullptr;
	}

	ElementPtr Document::getElementById(const std::string& id)
	{
		// XXX
		return nullptr;
	}

}
