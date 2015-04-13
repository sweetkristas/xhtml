#include <sstream>
#include <stdexcept>

namespace lex
{
	class bad_lexical_cast : public std::runtime_error
	{
	public:
		bad_lexical_cast(std::string const& error) : std::runtime_error(error) {}
		virtual ~bad_lexical_cast() {}
	};

	template <typename T>
	T lexical_cast(const std::string& str)
	{
	    T var;
	    std::istringstream iss;
	    iss.str(str);
	    iss >> var;
		if(iss.rdstate() & std::istream::eofbit) {
			return var;
		}
	    if(!(iss.rdstate() & std::istream::goodbit)) {
	    	throw bad_lexical_cast("bad_lexical_cast");
	    }
	    // deal with any error bits that may have been set on the stream
	    return var;
	}
}
