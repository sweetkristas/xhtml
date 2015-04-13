#pragma once

#include <map>

namespace sys
{
	typedef std::map<std::string, std::string> file_path_map;

	bool file_exists(const std::string& name);
	std::string read_file(const std::string& name);
	void write_file(const std::string& name, const std::string& data);
	void get_unique_files(const std::string& path, file_path_map& fpm);
}
