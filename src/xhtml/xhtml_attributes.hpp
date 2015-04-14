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

#include <string>
#include <vector>

#include <boost/property_tree/ptree.hpp>

#include "xhtml_fwd.hpp"

namespace xhtml
{
	class Attributes
	{
	public:
		explicit Attributes(const boost::property_tree::ptree& pt);

		void setID(const std::string& id) { id_ = id; }
		void setClass(const std::string& cls) { class_ = cls; }
		void setStyle(const std::string& stl) { style_ = stl; }
		void setTitle(const std::string& title) { title_ = title; }

		void setLang(const std::string& lang) { lang_ = lang; }
		void setDir(const std::string& dir) { dir_ = dir; }

		void setOnClick(const std::string& value) { on_click_ = value; }
		void setOnDblClick(const std::string& value) { on_dbl_click_ = value; }
		void setOnMouseDown(const std::string& value) { on_mouse_down_ = value; }
		void setOnMouseUp(const std::string& value) { on_mouse_up_ = value; }
		void setOnMouseOver(const std::string& value) { on_mouse_over_ = value; }
		void setOnMouseMove(const std::string& value) { on_mouse_move_ = value; }
		void setOnMouseOut(const std::string& value) { on_mouse_out_ = value; }
		void setOnKeyPress(const std::string& value) { on_key_press_ = value; }
		void setOnKeyDown(const std::string& value) { on_key_down_ = value; }
		void setOnKeyUp(const std::string& value) { on_key_up_ = value; }

		const std::string& getID() { return id_; }
		const std::string& getClass() { return class_; }
		const std::string& getStyle() { return style_; }
		const std::string& getTitle() { return title_; }
	private:
		// %coreattrs
		std::string id_;
		std::string class_;
		std::string style_;
		std::string title_;

		// % i18n
		std::string lang_;	// Language information
		std::string dir_;	// text direction.
		
		// %events
		std::string on_click_;
		std::string on_dbl_click_;
		std::string on_mouse_down_;
		std::string on_mouse_up_;
		std::string on_mouse_over_;
		std::string on_mouse_move_;
		std::string on_mouse_out_;
		std::string on_key_press_;
		std::string on_key_down_;
		std::string on_key_up_;
	};
}
