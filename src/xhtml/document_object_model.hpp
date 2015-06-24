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

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "dom_exception.hpp"

namespace dom
{
	// XXX N.B. we eschew the actual DOM spec by using utf-8 encoded strings internally instead of UTF-16

	class Node;
	typedef std::shared_ptr<Node> NodePtr;
	typedef std::map<std::string, NodePtr> NodeMap;
	class Document;
	typedef std::shared_ptr<Document> DocumentPtr;
	class Element;
	typedef std::shared_ptr<Element> ElementPtr;

	enum class NodeType {
		ELEMENT_NODE                   = 1,
		ATTRIBUTE_NODE                 = 2,
		TEXT_NODE                      = 3,
		CDATA_SECTION_NODE             = 4,
		ENTITY_REFERENCE_NODE          = 5,
		ENTITY_NODE                    = 6,
		PROCESSING_INSTRUCTION_NODE    = 7,
		COMMENT_NODE                   = 8,
		DOCUMENT_NODE                  = 9,
		DOCUMENT_TYPE_NODE             = 10,
		DOCUMENT_FRAGMENT_NODE         = 11,
		NOTATION_NODE                  = 12,
	};

	class NamedNodeMap
	{
	public:
		NamedNodeMap();
		explicit NamedNodeMap(std::weak_ptr<Document> owner);
		NamedNodeMap(const NamedNodeMap& m, bool deep);
		NodePtr getNamedItem(const std::string& name) const;
		NodePtr setNamedItem(NodePtr node);
		NodePtr removeNamedItem(const std::string& name);
		NodePtr getItem(int n) const;
		int getLength() const { return map_.size(); }
		NodePtr getNamedItemNS(const std::string namespace_uri, const std::string& name) const;
		NodePtr setNamedItemNS(NodePtr node);
		NodePtr removeNamedItemNS(const std::string namespace_uri, const std::string& name);
		DocumentPtr getOwnerDocument() const;
		bool isReadOnly() const { return false; }
	private:
		NodeMap map_;
		std::weak_ptr<Document> owner_document_;
	};

	/*class NodeList
	{
	public:
		NodeList(bool read_only=false);
		~NodeList();
		bool empty() const { return count_ == 0; }
		NodePtr getItem(int n) const;
		NodePtr front() const { return head_; }
		NodePtr back() const { return tail_; }
		NodePtr insertBefore(NodePtr new_node, NodePtr ref_node);
		NodePtr replaceChild(NodePtr new_node, NodePtr old_node);
		NodePtr removeChild(NodePtr new_child);
		NodePtr appendChild(NodePtr new_child);
		bool isReadOnly() const { return read_only_; }
	private:
		void clear();
		NodePtr head_;
		NodePtr tail_;
		int count_;
		bool read_only_;
	};*/

	class NodeList
	{
	public:
		typedef std::vector<NodePtr>::iterator iterator;
		typedef std::vector<NodePtr>::const_iterator const_iterator;
		NodeList(bool read_only=false);
		~NodeList();
		bool empty() const { return nodes_.empty(); }
		NodePtr getItem(int n) const;
		NodePtr front() const { return nodes_.front(); }
		NodePtr back() const { return nodes_.back(); }
		NodePtr insertAfter(NodePtr new_node, NodePtr ref_node);
		NodePtr insertBefore(NodePtr new_node, NodePtr ref_node);
		NodePtr replaceChild(NodePtr new_node, NodePtr old_node);
		NodePtr removeChild(NodePtr new_child);
		NodePtr appendChild(NodePtr new_child);
		bool isReadOnly() const { return read_only_; }
		iterator begin() { return nodes_.begin(); }
		iterator end() { return nodes_.end(); }
		const_iterator begin() const { return nodes_.begin(); }
		const_iterator end() const { return nodes_.end(); }
	private:
		std::vector<NodePtr> nodes_;
		bool read_only_;
	};

	class Node : public std::enable_shared_from_this<Node>
	{
	public:
		explicit Node(NodeType type, const std::string& name, std::weak_ptr<Document> owner);
		Node(const Node& node, bool deep);
		virtual ~Node() {}
		void setParent(std::weak_ptr<Node> parent);
		void setNextSibling(std::weak_ptr<Node> right) { right_ = right; }
		void setPreviousSibling(std::weak_ptr<Node> left) { left_ = left; }
		virtual const std::string& getNodeName() const { return name_; }
		virtual const std::string& getNodeValue() const { return value_; }
		virtual void setNodeValue(const std::string& value) {}
		NodeType type() const { return type_; }
		DocumentPtr getOwnerDocument() const;
		NodePtr getParentNode() const;
		NodePtr getFirstChild() const;
		NodePtr getLastChild() const;
		NodePtr getPreviousSibling() const;
		NodePtr getNextSibling() const;
		const NodeList& getChildNodes() const { return children_; }
		NodeList& getChildNodes() { return children_; }

		NodePtr insertAfter(NodePtr new_child, NodePtr ref_child);
		NodePtr insertBefore(NodePtr new_child, NodePtr ref_child);
		NodePtr replaceChild(NodePtr new_child, NodePtr old_child);
		NodePtr removeChild(NodePtr new_child);
		NodePtr appendChild(NodePtr new_child);
		bool hasChildNodes() const { return !children_.empty(); }

		virtual NodePtr cloneNode(bool deep) const;
		void normalize();
		bool isSupported(const std::string& feature, const std::string& version) const;
		virtual const std::string& getNamespaceURI() const;
		virtual const std::string& getPrefix() const;
		virtual const std::string& getLocalName() const;
		virtual bool hasAttributes() const { return false; }
		virtual bool isReadOnly() const { return false; }
	protected:
		void getElementsByTagName(NodeList* nl, const std::string& tagname);
		void getElementsByTagNameNS(NodeList* nl, const std::string& namespaceuri, const std::string& localname);
	private:
		NodeType type_;
		std::string name_;
		std::string value_;
		std::weak_ptr<Document> owner_document_;
		std::weak_ptr<Node> parent_;
		std::weak_ptr<Node> left_, right_;

		NodeList children_;
	};
	
	class Notation : public Node
	{
	public:
		explicit Notation(const std::string& name, const std::string& public_id, const std::string& system_id, std::weak_ptr<Document> owner);
		Notation(const Notation& node, bool deep);
		NodePtr cloneNode(bool deep) const override;
		const std::string& getPublicId() const { return public_id_; }
		const std::string& getSystemId() const { return system_id_; }
		bool isReadOnly() const override { return true; }
	private:
		std::string public_id_;
		std::string system_id_;
	};

   	class Entity : public Node
	{
	public:
		explicit Entity(const std::string& public_id, const std::string& system_id, const std::string& notation_name, std::weak_ptr<Document> owner);
		Entity(const Entity& node, bool deep);
		NodePtr cloneNode(bool deep) const override;
		const std::string& getPublicId() const { return public_id_; }		
		const std::string& getSystemId() const { return system_id_; }
		const std::string& getNotationName() const { return notation_name_; }
	private:
		std::string public_id_;
		std::string system_id_;
		std::string notation_name_;
	};

	class ProcessingInstruction : public Node
	{
	public:
		explicit ProcessingInstruction(const std::string& target, const std::string& data, std::weak_ptr<Document> owner);
		ProcessingInstruction(const ProcessingInstruction& node, bool deep);
		NodePtr cloneNode(bool deep) const override;
		const std::string& getData() const { return data_; }
	private:
		std::string data_;
	};
	typedef std::shared_ptr<ProcessingInstruction> ProcessingInstructionPtr;

	class Attribute : public Node
	{
	public:
		explicit Attribute(const std::string& name, const std::string& value, std::weak_ptr<Document> owner);
		Attribute(const Attribute& node, bool deep);
		NodePtr cloneNode(bool deep) const override;
		const std::string& getNodeValue() const override { return value_; }
		void setNodeValue(const std::string& value) override { value_ = value; specified_ = true; }
		bool isSpecified() const { return specified_; }
		const std::string& getNamespaceURI() const override { return namespace_uri_; }
		const std::string& getPrefix() const override { return prefix_; }
		const std::string& getLocalName() const override { return local_name_; }
	private:
		std::string value_;
		bool specified_;
		std::string prefix_;
		std::string local_name_;
		std::string namespace_uri_;
	};
	typedef std::shared_ptr<Attribute> AttributePtr;

	class Element : public Node
	{
	public:
		explicit Element(const std::string& tagname, std::weak_ptr<Document> owner);
		Element(const Element& e, bool deep);
		NodePtr cloneNode(bool deep) const override;
		const std::string& getAttribute(const std::string& name) const;
		void setAttribute(const std::string& name, const std::string& value);
		void removeAttribute(const std::string& name);
		AttributePtr getAttributeNode(const std::string& name) const;
		AttributePtr setAttributeNode(AttributePtr attr);
		AttributePtr removeAttributeNode(AttributePtr attr);
		NodeList getElementsByTagName(const std::string& tagname);
		const std::string& getAttributeNS(const std::string& namespaceuri, const std::string& localname) const;
		void setAttributeNS(const std::string& namespaceuri, const std::string& qualifiedname, const std::string& value);
		void removeAttributeNS(const std::string& namespaceuri, const std::string& localname);
		AttributePtr getAttributeNodeNS(const std::string& namespaceuri, const std::string& localname) const;
		AttributePtr setAttributeNodeNS(AttributePtr attr);
		NodeList getElementsByTagNameNS(const std::string& namespaceuri, const std::string& localname);
		bool hasAttribute(const std::string& name);
		bool hasAttributeNS(const std::string& namespaceuri, const std::string& localname);
		const std::string& getNamespaceURI() const override { return namespace_uri_; }
		const std::string& getPrefix() const override { return prefix_; }
		const std::string& getLocalName() const override { return local_name_; }
		ElementPtr get_this_ptr() { return std::static_pointer_cast<Element>(shared_from_this()); }
	private:
		NamedNodeMap attributes_;
		std::string prefix_;
		std::string local_name_;
		std::string namespace_uri_;
	};

	class CharacterData
	{
	public:
		explicit CharacterData(const std::string& data);
		CharacterData(const CharacterData& node, bool deep);
		const std::string& getData() const { return data_; }
		void setData(const std::string& data) { data_ = data; }
		int length() const { return data_.size(); }
		int size() const { return data_.size(); }
		std::string substring(int offset, int count);
		void append(const std::string& arg);
		void insert(int offset, const std::string& arg);
		void replace(int offset, int count, const std::string& arg);
	private:
		std::string data_;
	};

	class Text;
	typedef std::shared_ptr<Text> TextPtr;

	class Text : public CharacterData, public Node
	{
	public:
		explicit Text(const std::string& data, std::weak_ptr<Document> owner);
		Text(const Text& node, bool deep);
		NodePtr cloneNode(bool deep) const override;
		TextPtr splitText(int offset);
	protected:
		explicit Text(NodeType type, const std::string& name, const std::string& data, std::weak_ptr<Document> owner);
	};

	class Comment : public CharacterData, public Node
	{
	public:
		explicit Comment(const std::string& data, std::weak_ptr<Document> owner);
		Comment(const Comment& node, bool deep);
		NodePtr cloneNode(bool deep) const override;
	};
	typedef std::shared_ptr<Comment> CommentPtr;

	class CDATASection : public Text
	{
	public:
		explicit CDATASection(const std::string& data, std::weak_ptr<Document> owner);
		CDATASection(const CDATASection& node, bool deep);
		NodePtr cloneNode(bool deep) const override;
	};
	typedef std::shared_ptr<CDATASection> CDATASectionPtr;

	class DocumentType
	{
	public:
		explicit DocumentType(const std::string& name, const std::string& public_id, const std::string& system_id);
		const std::string& getName() const { return name_; }
		const NamedNodeMap& getEntities() const { return entities_; }
		const NamedNodeMap& getNotations() const { return notations_; }
		const std::string& getPublicId() const { return public_id_; }
		const std::string& getSystemId() const { return system_id_; }
		const std::string& getInternalSubset() const { return internal_subset_; }
	private:
		std::string name_;
		NamedNodeMap entities_;
		NamedNodeMap notations_;
		std::string public_id_;
		std::string system_id_;
		std::string internal_subset_;
		std::weak_ptr<Document> owner_;
	};
	typedef std::unique_ptr<DocumentType> DocumentTypePtr;

	class Implementation
	{
	public:
		Implementation();
		bool hasFeature(const std::string& feature, const std::string& version) const;
		static DocumentTypePtr createDocumentType(const std::string& qualified_name, const std::string& public_id, const std::string& system_id);
		static DocumentPtr createDocument(const std::string& namespace_uri, const std::string& qualified_name, DocumentTypePtr doctype);
	private:
	};
	typedef std::shared_ptr<Implementation> ImplementationPtr;

	class Document : public Node
	{
	public:
		Document();
		ElementPtr createElement(const std::string& tagname);
		ImplementationPtr getImplementation() { return implementation_.lock(); }
		NodePtr createDocumentFragment();
		TextPtr createTextNode(const std::string& data);
		CommentPtr createComment(const std::string& data);
		CDATASectionPtr createCDATASection(const std::string& data);
		ProcessingInstructionPtr createProcessingInstruction(const std::string& target, const std::string& data);
		AttributePtr createAttribute(const std::string& name);
		NodePtr createEntityReference(const std::string& name);
		NodeList getElementsByTagName(const std::string& tagname);
		NodePtr importNode(NodePtr imported_node, bool deep);
		ElementPtr createElementNS(const std::string& namespaceuri, const std::string& qualified_name);
		AttributePtr createAttributeNS(const std::string& namespaceuri, const std::string& qualified_name);
		NodeList getElementsByTagNameNS(const std::string& namespaceuri, const std::string& localname);
		ElementPtr getElementById(const std::string& id);
		DocumentPtr get_this_ptr() { return std::static_pointer_cast<Document>(shared_from_this()); }
	private:
		// Document type definition.
		DocumentTypePtr doctype_;
		// pointer to the implentation that created this.
		std::weak_ptr<Implementation> implementation_;
		// root child element
		ElementPtr element_;
	};
}
