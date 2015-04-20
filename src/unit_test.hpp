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

#include <functional>

#include <iostream>
#include <string>
#include <vector>

namespace test 
{
	struct failure_exception 
	{
	};

	typedef std::function<void ()> unit_test;

	int register_test(const std::string& name, unit_test test);
	
	bool run_tests(const std::vector<std::string>* tests=NULL);
}

#define CHECK(cond, msg) if(!(cond)) { std::cerr << __FILE__ << ":" << __LINE__ << ": TEST CHECK FAILED: " << #cond << ": " << msg << "\n"; throw test::failure_exception(); }

#define CHECK_CMP(a, b, cmp) CHECK((a) cmp (b), #a << ": " << (a) << "; " << #b << ": " << (b))

#define CHECK_EQ(a, b) CHECK_CMP(a, b, ==)
#define CHECK_NE(a, b) CHECK_CMP(a, b, !=)
#define CHECK_LE(a, b) CHECK_CMP(a, b, <=)
#define CHECK_GE(a, b) CHECK_CMP(a, b, >=)
#define CHECK_LT(a, b) CHECK_CMP(a, b, <)
#define CHECK_GT(a, b) CHECK_CMP(a, b, >)

#define UNIT_TEST(name) \
	namespace test {    \
	void TEST_##name(); \
	static int TEST_VAR_##name = register_test(#name, TEST_##name); \
	void debug_fn_##name() { std::cerr << TEST_VAR_##name << "\n"; } \
    }                   \
	void test::TEST_##name()
