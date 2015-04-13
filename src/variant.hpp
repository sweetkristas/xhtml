#pragma once

#include <map>
#include <string>
#include <vector>
#include <cstdint>

class variant;
typedef std::map<variant,variant> variant_map;
typedef std::vector<variant> variant_list;

class variant
{
public:
	enum variant_type
	{
		VARIANT_TYPE_NULL,
		VARIANT_TYPE_BOOL,
		VARIANT_TYPE_INTEGER,
		VARIANT_TYPE_FLOAT,
		VARIANT_TYPE_STRING,
		VARIANT_TYPE_MAP,
		VARIANT_TYPE_LIST,
	};

	variant();
	variant(const variant&);
	explicit variant(int64_t);
	explicit variant(int);
	explicit variant(float);
	explicit variant(double);
	explicit variant(const std::string&);
	explicit variant(const variant_map&);
	explicit variant(const variant_list&);
	explicit variant(std::vector<variant>* list);
	explicit variant(variant_map* vmap);

	variant_type type() const { return type_; }
	std::string type_as_string() const;

	static variant from_bool(bool b);

	std::string as_string() const;
	std::string as_string_default(const std::string& s) const;
	int64_t as_int() const;
	int64_t as_int(int64_t value) const;
	int as_int32(int value=0) const { return static_cast<int>(as_int(value)); }
	float as_float() const;
	float as_float(float value) const;
	bool as_bool() const;
	bool as_bool(bool default_value) const;
	const variant_list& as_list() const;
	const variant_map& as_map() const;
	std::vector<std::string> as_list_string() const;
	std::vector<int> as_list_int() const;

	variant_list& as_mutable_list();
	variant_map& as_mutable_map();

	bool is_string() const { return type_ == VARIANT_TYPE_STRING; }
	bool is_null() const { return type_ == VARIANT_TYPE_NULL; }
	bool is_bool() const { return type_ == VARIANT_TYPE_BOOL; }
	bool is_numeric() const { return is_int() || is_float(); }
	bool is_int() const { return type_ == VARIANT_TYPE_INTEGER; }
	bool is_float() const { return type_ == VARIANT_TYPE_FLOAT; }
	bool is_map() const { return type_ == VARIANT_TYPE_MAP; }
	bool is_list() const { return type_ == VARIANT_TYPE_LIST; }

	bool operator<(const variant&) const;
	bool operator>(const variant&) const;

	bool operator==(const variant&) const;
	bool operator!=(const variant&) const;

	bool operator==(const std::string&) const;
	bool operator==(int64_t) const;

	const variant& operator[](size_t n) const;
	const variant& operator[](const variant& v) const;
	const variant& operator[](const std::string& key) const;

	int num_elements() const;

	bool has_key(const variant& v) const;
	bool has_key(const std::string& key) const;
			
	void write_json(std::ostream& s, bool pretty=true, int indent=0) const;
	std::string write_json(bool pretty=true, int indent=0) const;

	std::string to_debug_string() const;
protected:
private:
	variant_type type_;

	bool b_;
	int64_t i_;
	float f_;
	std::string s_;
	variant_map m_;
	variant_list l_;
};

std::ostream& operator<<(std::ostream& os, const variant& n);

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

glm::vec3 variant_to_vec3(const variant& v);
variant vec3_to_variant(const glm::vec3& v);

glm::ivec3 variant_to_ivec3(const variant& v);
variant ivec3_to_variant(const glm::ivec3& v);

glm::quat variant_to_quat(const variant& v);
variant quat_to_variant(const glm::quat& v);

glm::vec4 variant_to_vec4(const variant& v);
variant vec4_to_variant(const glm::vec4& v);

