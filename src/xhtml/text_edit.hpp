/*
	Copyright (C) 2016 by Kristina Simpson <sweet.kristas@gmail.com>

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

#include <functional>

#include "AttributeSet.hpp"
#include "geometry.hpp"
#include "SceneObject.hpp"
#include "WindowManagerFwd.hpp"
#include "event_listener.hpp"
#include "FontDriver.hpp"
#include "scrollable.hpp"

namespace controls
{
	class TextEdit;
	typedef std::shared_ptr<TextEdit> TextEditPtr;

	typedef std::function<void(const std::string& s)> change_handler;

	enum class TextEditType {
		SINGLE_LINE,
		MULTI_LINE,
	};

	class TextEdit : public KRE::SceneObject, public EventListener
	{
	public:
		TextEdit(const rect& area, TextEditType type=TextEditType::SINGLE_LINE, const std::string& default_value=std::string());
		~TextEdit() {}

		void setHandlers(change_handler onchange);

		const KRE::FontRenderablePtr& getRenderable() const { return renderable_; }

		void preRender(const KRE::WindowPtr& wm) override;

		static TextEditPtr create(const rect& area, TextEditType type=TextEditType::SINGLE_LINE, const std::string& default_value=std::string());
	private:
		bool handleKeyDown(bool claimed, const SDL_Keysym& keysym, bool repeat, bool pressed);
		bool handleKeyUp(bool claimed, const SDL_Keysym& keysym, bool repeat, bool pressed) { return claimed; }
		bool handleTextInput(bool claimed, const std::string& text);
		bool handleTextEditing(bool claimed, const std::string& text, int start, int length);

		bool handleMouseMotion(bool claimed, const point& p, unsigned keymod, bool in_rect) override;
		bool handleMouseButtonUp(bool claimed, const point& p, unsigned buttons, unsigned keymod, bool in_rect) override;
		bool handleMouseButtonDown(bool claimed, const point& p, unsigned buttons, unsigned keymod, bool in_rect) override;	
		bool handleMouseWheel(bool claimed, const point& p, const point& delta, int direction, bool in_rect) override;

		TextEditType type_;
		std::string current_line_text_;
		std::vector<std::string> multi_line_text_;
		rect loc_;
		
		change_handler on_change_;
		
		KRE::Color background_color_;
		KRE::ColorPtr text_color_;

		KRE::FontRenderablePtr renderable_;

		scrollable::ScrollbarPtr scollbar_;
	};
}
