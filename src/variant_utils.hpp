/*
   Copyright 2014 Kristina Simpson <sweet.kristas@gmail.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#pragma once

#include "geometry.hpp"
#include "variant.hpp"

extern point variant_to_point(const variant& n);

class variant_builder
{
public:
	template<typename T> variant_builder& add(const std::string& name, const T& value)
	{
		return add_value(name, variant(value));
	}

	template<typename T> variant_builder& add(const std::string& name, T& value)
	{
		return add_value(name, variant(value));
	}

	template<typename T> variant_builder& set(const std::string& name, const T& value)
	{
		return set_value(name, variant(value));
	}

	template<typename T> variant_builder& set(const std::string& name, T& value)
	{
		return set_value(name, variant(value));
	}

	variant build();
	variant_builder& clear();
private:
	variant_builder& add_value(const std::string& name, const variant& value);
	variant_builder& set_value(const std::string& name, const variant& value);

	std::map<variant, std::vector<variant>> attr_;
};


template<> inline variant_builder& variant_builder::add(const std::string& name, const variant& value)
{
	return add_value(name, value);
}

template<> inline variant_builder& variant_builder::add(const std::string& name, variant& value)
{
	return add_value(name, value);
}
