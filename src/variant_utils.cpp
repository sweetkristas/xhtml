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

#include "asserts.hpp"
#include "variant_utils.hpp"

point variant_to_point(const variant& n)
{
	ASSERT_LOG(n.is_list() && n.num_elements() == 2, "points must be lists of 2 numbers.");
	return point(n[0].as_int32(), n[1].as_int32());
}

variant_builder& variant_builder::add_value(const std::string& name, const variant& value)
{
	attr_[variant(name)].emplace_back(value);
	return *this;
}

variant_builder& variant_builder::set_value(const std::string& name, const variant& value)
{
	variant key(name);
	attr_.erase(key);
	attr_[key].emplace_back(value);
	return *this;
}

variant variant_builder::build()
{
	std::map<variant, variant> res;
	for(auto& i : attr_) {
		if(i.second.size() == 1) {
			res[i.first] = i.second[0];
		} else {
			res[i.first] = variant(&i.second);
		}
	}
	return variant(&res);
}

variant_builder& variant_builder::clear()
{
	attr_.clear();
	return *this;
}
