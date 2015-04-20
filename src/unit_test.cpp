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

#include <chrono>
#include <iostream>
#include <map>

#include "asserts.hpp"
#include "unit_test.hpp"

namespace test {

	namespace 
	{
		typedef std::map<std::string, unit_test> test_map;
		test_map& get_test_map()
		{
			static test_map map;
			return map;
		}
	}

	int register_test(const std::string& name, unit_test test)
	{
		get_test_map()[name] = test;
		return 0;
	}

	bool run_tests(const std::vector<std::string>* tests)
	{
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

		std::vector<std::string> all_tests;
		if(!tests) {
			for(test_map::const_iterator i = get_test_map().begin(); i != get_test_map().end(); ++i) {
				all_tests.push_back(i->first);
			}

			tests = &all_tests;
		}

		int npass = 0, nfail = 0;
		for(const auto& test : *tests) {
			try {
				get_test_map()[test]();
				LOG_INFO("TEST " << test << " PASSED");
				++npass;
			} catch(failure_exception&) {
				LOG_ERROR("TEST " << test << " FAILED!!");
				++nfail;
			}
		}

		if(nfail) {
			LOG_ERROR(npass << " TESTS PASSED, " << nfail << " TESTS FAILED");
			return false;
		} else {
			const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
			const auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
			LOG_INFO("ALL " << npass << " TESTS PASSED IN " << time_diff << "ms");
			return true;
		}
	}
}
