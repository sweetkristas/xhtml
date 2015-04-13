#include <sstream>
#include "asserts.hpp"
#include "json.hpp"
#include "variant.hpp"

namespace
{
	const variant& null_variant()
	{
		static variant res;
		return res;
	}
}

variant::variant()
	: type_(VARIANT_TYPE_NULL), i_(0), f_(0.0f), b_(false)
{
}

variant::variant(const variant& rhs) 
	: type_(rhs.type()), i_(0), f_(0.0f), b_(false)
{
	switch(type_) {
	case VARIANT_TYPE_NULL:		break;
	case VARIANT_TYPE_INTEGER:	i_ = rhs.i_; break;
	case VARIANT_TYPE_FLOAT:	f_ = rhs.f_; break;
	case VARIANT_TYPE_BOOL:		b_ = rhs.b_; break;
	case VARIANT_TYPE_STRING:	s_ = rhs.s_; break;
	case VARIANT_TYPE_MAP:		m_ = rhs.m_; break;
	case VARIANT_TYPE_LIST:		l_ = rhs.l_; break;
	default:
		ASSERT_LOG(false, "Unrecognised type in copy constructor: " << type_);
	}
}

variant::variant(int64_t n)
	: type_(VARIANT_TYPE_INTEGER), i_(n), f_(0.0f), b_(false)
{

}

variant::variant(int n)
	: type_(VARIANT_TYPE_INTEGER), i_(n), f_(0.0f), b_(false)
{

}

variant::variant(float f)
	: type_(VARIANT_TYPE_FLOAT), i_(0), f_(f), b_(false)
{
}

variant::variant(double f)
	: type_(VARIANT_TYPE_FLOAT), i_(0), f_(static_cast<float>(f)), b_(false)
{
}

variant::variant(const std::string& s)
	: type_(VARIANT_TYPE_STRING), i_(0), f_(0.0f), s_(s), b_(false)
{
}

variant::variant(const std::map<variant,variant>& m)
	: type_(VARIANT_TYPE_MAP), i_(0), f_(0.0f), m_(m), b_(false)
{
}

variant::variant(const std::vector<variant>& l)
	: type_(VARIANT_TYPE_LIST), i_(0), f_(0.0f), l_(l), b_(false)
{
}

variant::variant(std::vector<variant>* list)
	: type_(VARIANT_TYPE_LIST), i_(0), f_(0.0f), b_(false)
{
	l_.swap(*list);
}

variant::variant(variant_map* vmap)
	: type_(VARIANT_TYPE_MAP), i_(0), f_(0), b_(false)
{
	m_.swap(*vmap);
}

variant variant::from_bool(bool b)
{
	variant n;
	n.type_ = VARIANT_TYPE_BOOL;
	n.b_ = b;
	return n;
}


std::string variant::type_as_string() const
{
	switch(type_) {
	case VARIANT_TYPE_NULL:
		return "null";
	case VARIANT_TYPE_INTEGER:
		return "int";
	case VARIANT_TYPE_FLOAT:
		return "float";
	case VARIANT_TYPE_BOOL:
		return "bool";
	case VARIANT_TYPE_STRING:
		return "string";
	case VARIANT_TYPE_MAP:
		return "map";
	case VARIANT_TYPE_LIST:
		return "list";
	}
	ASSERT_LOG(false, "Unrecognised type converting to string: " << type_);
}

int64_t variant::as_int() const
{
	switch(type()) {
	case VARIANT_TYPE_INTEGER:
		return i_;
	case VARIANT_TYPE_FLOAT:
		return int64_t(f_);
	case VARIANT_TYPE_BOOL:
		return b_ ? 1 : 0;
	default: break;
	}
	ASSERT_LOG(false, "as_int() type conversion error from " << type_as_string() << " to int");
	return 0;
}

int64_t variant::as_int(int64_t value) const
{
	switch(type()) {
	case VARIANT_TYPE_INTEGER:
		return i_;
	case VARIANT_TYPE_FLOAT:
		return int64_t(f_);
	case VARIANT_TYPE_BOOL:
		return b_ ? 1 : 0;
	default: break;
	}
	return value;
}


std::string variant::as_string() const
{
	switch(type()) {
	case VARIANT_TYPE_STRING:
		return s_;
	case VARIANT_TYPE_INTEGER: {
		std::stringstream s;
		s << i_;
		return s.str();
	}
	case VARIANT_TYPE_FLOAT: {
		std::stringstream s;
		s << f_;
		return s.str();
	}
	default: break;
	}
	ASSERT_LOG(false, "as_string() type conversion error from " << type_as_string() << " to string");
	return "";
}

std::string variant::as_string_default(const std::string& s) const
{
	switch(type()) {
	case VARIANT_TYPE_STRING:
		return s_;
	case VARIANT_TYPE_INTEGER: {
		std::stringstream s;
		s << i_;
		return s.str();
	}
	case VARIANT_TYPE_FLOAT: {
		std::stringstream s;
		s << f_;
		return s.str();
	}
	default: break;
	}
	return s;
}

float variant::as_float() const
{
	switch(type()) {
	case VARIANT_TYPE_INTEGER:
		return float(i_);
	case VARIANT_TYPE_FLOAT:
		return f_;
	case VARIANT_TYPE_BOOL:
		return b_ ? 1.0f : 0.0f;
	default: break;
	}
	ASSERT_LOG(false, "as_float() type conversion error from " << type_as_string() << " to float");
	return 0;
}

float variant::as_float(float value) const
{
	switch(type()) {
	case VARIANT_TYPE_INTEGER:
		return float(i_);
	case VARIANT_TYPE_FLOAT:
		return f_;
	case VARIANT_TYPE_BOOL:
		return b_ ? 1.0f : 0.0f;
	default: break;
	}
	return value;
}

bool variant::as_bool() const
{
	switch(type()) {
	case VARIANT_TYPE_INTEGER:
		return i_ ? true : false;
	case VARIANT_TYPE_FLOAT:
		return f_ == 0.0f ? false : true;
	case VARIANT_TYPE_BOOL:
		return b_;
	case VARIANT_TYPE_STRING:
		return s_.empty() ? false : true;
	case VARIANT_TYPE_LIST:
		return l_.empty() ? false : true;
	case VARIANT_TYPE_MAP:
		return m_.empty() ? false : true;
	default: break;
	}
	ASSERT_LOG(false, "as_bool() type conversion error from " << type_as_string() << " to boolean");
	return 0;
}

bool variant::as_bool(bool default_value) const
{
	switch(type_) {
	case VARIANT_TYPE_INTEGER: return i_ != 0;
	case VARIANT_TYPE_BOOL: return b_;
	default: return default_value;
	}
}

const variant_list& variant::as_list() const
{
	ASSERT_LOG(type() == VARIANT_TYPE_LIST, "as_list() type conversion error from " << type_as_string() << " to list");
	return l_;
}

const variant_map& variant::as_map() const
{
	ASSERT_LOG(type() == VARIANT_TYPE_MAP, "as_map() type conversion error from " << type_as_string() << " to map");
	return m_;
}

variant_list& variant::as_mutable_list()
{
	ASSERT_LOG(type() == VARIANT_TYPE_LIST, "as_mutable_list() type conversion error from " << type_as_string() << " to list");
	return l_;
}

variant_map& variant::as_mutable_map()
{
	ASSERT_LOG(type() == VARIANT_TYPE_MAP, "as_mutable_map() type conversion error from " << type_as_string() << " to map");
	return m_;
}

bool variant::operator<(const variant& n) const
{
	if(type() != n.type()) {
		return type_ < n.type();
	}
	switch(type()) {
	case VARIANT_TYPE_NULL:
		return true;
	case VARIANT_TYPE_BOOL:
		return b_ < n.b_;
	case VARIANT_TYPE_INTEGER:
		return i_ < n.i_;
	case VARIANT_TYPE_FLOAT:
		return f_ < n.f_;
	case VARIANT_TYPE_STRING:
		return s_ < n.s_;
	case VARIANT_TYPE_MAP:
		return m_.size() < n.m_.size();
	case VARIANT_TYPE_LIST:
		for(int i = 0; i != l_.size() && i != n.l_.size(); ++i) {
			if(l_[i] < n.l_[i]) {
				return true;
			} else if(l_[i] > n.l_[i]) {
				return false;
			}
		}
		return l_.size() < n.l_.size();
	default: break;
	}
	ASSERT_LOG(false, "operator< unknown type: " << type_as_string());
	return false;
}

bool variant::operator>(const variant& n) const
{
	return !(*this < n);
}

const variant& variant::operator[](size_t n) const
{
	ASSERT_LOG(type() == VARIANT_TYPE_LIST, "Tried to index variant that isn't a list, was: " << type_as_string());
	ASSERT_LOG(n < l_.size(), "Tried to index a list outside of list bounds: " << n << " >= " << l_.size());
	return l_[n];
}

const variant& variant::operator[](const variant& v) const
{
	if(type() == VARIANT_TYPE_LIST) {
		return l_[size_t(v.as_int())];
	} else if(type() == VARIANT_TYPE_MAP) {
		auto it = m_.find(v);
		ASSERT_LOG(it != m_.end(), "Couldn't find key in map");
		return it->second;
	} else {
		ASSERT_LOG(false, "Tried to index a variant that isn't a list or map: " << type_as_string());
	}
	return null_variant();
}

const variant& variant::operator[](const std::string& key) const
{
	ASSERT_LOG(type() == VARIANT_TYPE_MAP, "Tried to index variant that isn't a map, was: " << type_as_string());
	auto it = m_.find(variant(key));
	//ASSERT_LOG(it != m_.end(), "Couldn't find key(" << key << ") in map");
	if(it != m_.end()) {
		return it->second;
	}
	return null_variant();
}

bool variant::has_key(const variant& v) const
{
	if(type() == VARIANT_TYPE_LIST) {
		return v.as_int() < l_.size() ? true : false;
	} else if(type() == VARIANT_TYPE_MAP) {
		return m_.find(v) != m_.end() ? true : false;
	} else {
		ASSERT_LOG(false, "Tried to index a variant that isn't a list or map: " << type_as_string());
	}
	return false;
}

bool variant::has_key(const std::string& key) const
{
	//ASSERT_LOG(type() == VARIANT_TYPE_MAP, "Tried to index variant that isn't a map, was: " << type_as_string());
	if(type() != VARIANT_TYPE_MAP) {
		return false;
	}
	return m_.find(variant(key)) != m_.end() ? true : false;
}

bool variant::operator==(const std::string& s) const
{
	return *this == variant(s);
}

bool variant::operator==(int64_t n) const
{
	return *this == variant(n);
}

bool variant::operator==(const variant& n) const
{
	if(type() != n.type()) {
		if(type() == VARIANT_TYPE_FLOAT || n.type() == VARIANT_TYPE_FLOAT) {
			if(!is_numeric() && !is_null() || !n.is_numeric() && !n.is_null()) {
				return false;
			}
			return is_numeric() == n.is_numeric();
		}
		return false;
	}
	switch(type()) {
	case VARIANT_TYPE_NULL:
		return n.is_null();
	case VARIANT_TYPE_BOOL:
		return b_ == n.b_;
	case VARIANT_TYPE_INTEGER:
		return i_ == n.i_;
	case VARIANT_TYPE_FLOAT:
		return f_ == n.f_;
	case VARIANT_TYPE_STRING:
		return s_ == n.s_;
	case VARIANT_TYPE_MAP:
		return m_ == n.m_;
	case VARIANT_TYPE_LIST:
		if(l_.size() != n.l_.size()) {
			return false;
		}
		for(size_t ndx = 0; ndx != l_.size(); ++ndx) {
			if(l_[ndx] != n.l_[ndx]) {
				return false;
			}
		}
		return true;
	default: break;
	}
	return false;
}

int variant::num_elements() const
{
	if(type_ == VARIANT_TYPE_NULL){
		return 0;
	} else if(type_ == VARIANT_TYPE_BOOL) {
		return 1;
	} else if(type_ == VARIANT_TYPE_INTEGER) {
		return 1;
	} else if(type_ == VARIANT_TYPE_FLOAT) {
		return 1;
	} else if (type_ == VARIANT_TYPE_LIST) {
		return static_cast<int>(l_.size());
	} else if (type_ == VARIANT_TYPE_STRING) {
		return static_cast<int>(s_.size());
	} else if (type_ == VARIANT_TYPE_MAP) {
		return static_cast<int>(m_.size());
	}
	return 0;
}

bool variant::operator!=(const variant& n) const
{
	return !operator==(n);
}

void variant::write_json(std::ostream& os, bool pretty, int indent) const
{
	switch(type()) {
	case VARIANT_TYPE_NULL:
		os << "null";
		break;
	case VARIANT_TYPE_BOOL:
		os << b_ ? "true" : "false";
		break;
	case VARIANT_TYPE_INTEGER:
		os << i_;
		break;
	case VARIANT_TYPE_FLOAT:
		os << f_;
		break;
	case VARIANT_TYPE_STRING:
		for(auto it = s_.begin(); it != s_.end(); ++it) {
			if(*it == '"') {
				os << "\\\"";
			} else if(*it == '\\') {
				os << "\\\\";
			} else if(*it == '/') {
				os << "\\/";
			} else if(*it == '\b') {
				os << "\\b";
			} else if(*it == '\f') {
				os << "\\f";
			} else if(*it == '\n') {
				os << "\\n";
			} else if(*it == '\r') {
				os << "\\r";
			} else if(*it == '\t') {
				os << "\\t";
			} else if(*it > 128) {
				uint8_t value = uint8_t(*it);
				uint16_t code_point = 0;
				if(value & 0xe0) {
					code_point = (value & 0x1f) << 12;
					value = uint8_t(*it++);
					code_point |= (value & 0x7f) << 6;
					value = uint8_t(*it++);
					code_point |= (value & 0x7f);
				} else if(value & 0xc0) {
					code_point = (value & 0x3f) << 6;
					value = uint8_t(*it++);
					code_point |= (value & 0x7f);
				}
			} else {
				os << *it;
			}
		}
		break;
	case VARIANT_TYPE_MAP:
		os << pretty ? "{\n" + std::string(' ', indent) : "{";
		for(auto pr = m_.begin(); pr != m_.end(); ++pr) {
			if(pr != m_.begin()) {
				os << pretty ? ",\n" + std::string(' ', indent) : ",";
			}
			pr->first.write_json(os, pretty, indent + 4);
			os << pretty ? ": " : ":";
			pr->second.write_json(os, pretty, indent + 4);
		}
		os << pretty ? "\n" + std::string(' ', indent) + "}" : "}";
	case VARIANT_TYPE_LIST:
		os << pretty ? "[\n" + std::string(' ', indent) : "[";
		for(auto it = l_.begin(); it != l_.end(); ++it) {
			if(it != l_.begin()) {
				os << pretty ? ",\n" + std::string(' ', indent) : ",";
			}
			it->write_json(os, pretty, indent + 4);
		}
		os << pretty ? "\n" + std::string(' ', indent) + "]" : "]";
		break;
	}
}

std::string variant::write_json(bool pretty, int indent) const
{
	std::stringstream ss;
	write_json(ss, pretty, indent);
	return ss.str();
}

std::vector<std::string> variant::as_list_string() const
{
	std::vector<std::string> result;
	ASSERT_LOG(type_ == VARIANT_TYPE_LIST, "as_list_string: variant must be a list.");
	result.reserve(l_.size());
	for(auto& el : l_) {
		ASSERT_LOG(el.is_string(), "as_list_string: Each element in list must be a string.");
		result.emplace_back(el.as_string());
	}
	return result;
}

std::vector<int> variant::as_list_int() const
{
	std::vector<int> result;
	ASSERT_LOG(type_ == VARIANT_TYPE_LIST, "as_list_int: variant must be a list.");
	result.reserve(l_.size());
	for(auto& el : l_) {
		ASSERT_LOG(el.is_numeric(), "as_list_int: Each element in list must be an integer");
		result.emplace_back(el.as_int32());
	}
	return result;
}

std::ostream& operator<<(std::ostream& os, const variant& n)
{
	n.write_json(os);
	return os;
}

glm::vec3 variant_to_vec3(const variant& v)
{
	if(v.is_numeric()) {
		return glm::vec3(v.as_float(), 0.0f, 0.0f);
	}
	ASSERT_LOG(v.is_list() && v.num_elements() > 0 && v.num_elements() <= 3, "Expected vec3 variant but found " << v);
	glm::vec3 result(0.0f);
	result[0] = v[0].as_float();
	if(v.num_elements() >= 2) {
		result[1] = v[1].as_float();
	}
	if(v.num_elements() >= 3) {
		result[2] = v[2].as_float();
	}
	return result;
}

variant vec3_to_variant(const glm::vec3& v)
{
	std::vector<variant> result;
	result.push_back(variant(float(v[0])));
	result.push_back(variant(float(v[1])));
	result.push_back(variant(float(v[2])));
	return variant(&result);
}

glm::ivec3 variant_to_ivec3(const variant& v)
{
	ASSERT_LOG(v.is_list() && v.num_elements() == 3, "Expected ivec3 variant but found " << v);
	return glm::ivec3(v[0].as_int(), v[1].as_int(), v[2].as_int());
}

variant ivec3_to_variant(const glm::ivec3& v)
{
	std::vector<variant> result;
	result.push_back(variant(int64_t(v.x)));
	result.push_back(variant(int64_t(v.y)));
	result.push_back(variant(int64_t(v.z)));
	return variant(&result);
}

glm::quat variant_to_quat(const variant& v)
{
	ASSERT_LOG(v.is_list() && v.num_elements() == 4, "Expected vec4 variant but found " << v);
	glm::quat result;
	result.w = v[0].as_float();
	result.x = v[1].as_float();
	result.y = v[2].as_float();
	result.z = v[3].as_float();
	return result;
}

variant quat_to_variant(const glm::quat& v)
{
	std::vector<variant> result;
	result.push_back(variant(float(v.w)));
	result.push_back(variant(float(v.x)));
	result.push_back(variant(float(v.y)));
	result.push_back(variant(float(v.z)));
	return variant(&result);
}

glm::vec4 variant_to_vec4(const variant& v)
{
	ASSERT_LOG(v.is_list() && v.num_elements() == 4, "Expected vec4 variant but found " << v);
	glm::vec4 result;
	result[0] = v[0].as_float();
	result[1] = v[1].as_float();
	result[2] = v[2].as_float();
	result[3] = v[2].as_float();
	return result;
}

variant vec4_to_variant(const glm::vec4& v)
{
	std::vector<variant> result;
	result.push_back(variant(float(v.x)));
	result.push_back(variant(float(v.y)));
	result.push_back(variant(float(v.z)));
	result.push_back(variant(float(v.w)));
	return variant(&result);
}

std::string variant::to_debug_string() const
{
	return write_json(true, 0);
}
