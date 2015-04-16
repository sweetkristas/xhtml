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

#include <sstream>

#include "dom_exception.hpp"

namespace dom
{
	namespace
	{
		const char* exception_code_to_message(ExceptionCode err)
		{
			switch(ExceptionCode)
			{
			case dom::ExceptionCode::INDEX_SIZE_ERR:				return "Index size error";
			case dom::ExceptionCode::DOMSTRING_SIZE_ERR:			return "DOMstring size error";
			case dom::ExceptionCode::HIERARCHY_REQUEST_ERR:			return "Hierarchy request error";
			case dom::ExceptionCode::WRONG_DOCUMENT_ERR:			return "Wrong document error";
			case dom::ExceptionCode::INVALID_CHARACTER_ERR:			return "Invalid character error";
			case dom::ExceptionCode::NO_DATA_ALLOWED_ERR:			return "No data allowed error";
			case dom::ExceptionCode::NO_MODIFICATION_ALLOWED_ERR:	return "No modification allowed error";
			case dom::ExceptionCode::NOT_FOUND_ERR:					return "Not found error";
			case dom::ExceptionCode::NOT_SUPPORTED_ERR:				return "Not supported error";
			case dom::ExceptionCode::INUSE_ATTRIBUTE_ERR:			return "attribute in-use error";
			case dom::ExceptionCode::INVALID_STATE_ERR:				return "invalid state error";
			case dom::ExceptionCode::SYNTAX_ERR:					return "syntax error";
			case dom::ExceptionCode::INVALID_MODIFICATION_ERR:		return "invalid modification error";
			case dom::ExceptionCode::NAMESPACE_ERR:					return "namespace error";
			case dom::ExceptionCode::INVALID_ACCESS_ERR:			return "invalid access error";
			default:
				break;
			}
			return "Unrecognized error code.";
		}
	}

	Exception::Exception(ExceptionCode err)
		: std::runtime_error(exception_code_to_message(err)),
		  code_(err)
	{
	}
}
