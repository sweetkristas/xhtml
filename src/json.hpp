#pragma once

#include "variant.hpp"


namespace json
{
	class parse_error : public std::runtime_error
	{
	public:
		parse_error(const std::string& error)
			: std::runtime_error(error)
		{}
	};

	variant parse(const std::string& s);
	variant parse_from_file(const std::string& fname);
	void write(std::ostream& os, const variant& n, bool pretty=true);
}
