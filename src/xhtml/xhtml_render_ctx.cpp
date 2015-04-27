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

#pragma once

#include "xhtml_render_ctx.hpp"

namespace xhtml
{
	RenderContext::RenderContext(const std::string& font_name, double font_size)
	{
		std::vector<std::string> names(1, font_name);
		fh_.emplace(KRE::FontDriver::getFontHandle(names, font_size));
	}

	void RenderContext::pushFont(const std::string& name, double size)
	{
		std::vector<std::string> names(1, name);
		pushFont(names, size);
	}

	void RenderContext::pushFont(const std::vector<std::string>& name, double size)
	{
		fh_.emplace(KRE::FontDriver::getFontHandle(name, size));
	}

	void RenderContext::popFont()
	{
		fh_.pop();
		ASSERT_LOG(!fh_.empty(), "Popped too many fonts and emptied the stack");
	}

	RenderContext& RenderContext::get()
	{
		static RenderContext res("FreeSerif.ttf", 14);
		return res;
	}
}
