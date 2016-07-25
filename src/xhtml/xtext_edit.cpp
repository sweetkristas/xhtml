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

#include "xtext_edit.hpp"
#include "profile_timer.hpp"

#include "DisplayDevice.hpp"
#include "Shaders.hpp"

namespace xhtml
{
	using namespace KRE;

	TextEdit::TextEdit(const rect& area, TextEditType type, const std::string& default_value)
		: SceneObject("TextEdit"),
		  type_(type),
		  current_line_text_(default_value),
		  multi_line_text_(),
		  loc_(area),
		  background_color_(Color::colorWhite()),
		  text_color_(new Color(Color::colorBlack())),
		  renderable_(nullptr)
	{
		setShader(ShaderProgram::getProgram("simple"));

		auto ab = DisplayDevice::createAttributeSet(false, false, false);
		attr_ = std::make_shared<Attribute<glm::u16vec2>>(AccessFreqHint::STATIC, AccessTypeHint::DRAW);
		attr_->addAttributeDesc(AttributeDesc(AttrType::POSITION, 2, AttrFormat::SHORT, false));
		ab->addAttribute(attr_);

		ab->setDrawMode(DrawMode::LINE_LOOP);
		addAttributeSet(ab);

		init();
	}

	TextEditPtr TextEdit::create(const rect& area, TextEditType type, const std::string& default_value)
	{
		return std::make_shared<TextEdit>(area, type, default_value);
	}

	void TextEdit::init()
	{
		//if(changed_) {
		std::vector<glm::u16vec2> vertices;
		vertices.emplace_back(loc_.x1(), loc_.y1());
		vertices.emplace_back(loc_.x2(), loc_.y1());
		vertices.emplace_back(loc_.x2(), loc_.y2());
		vertices.emplace_back(loc_.x1(), loc_.y2());
		attr_->update(&vertices);
		//}


		if(fh_) {
			profile::manager pman("render text");
			auto r = fh_->getBoundingBox(current_line_text_);
			LOG_INFO("bounding box: " << r);
			
			std::vector<point> path;
			path = fh_->getGlyphPath(current_line_text_);
			if(renderable_) {
				renderable_->clear();
			}
			renderable_ = fh_->createRenderableFromPath(renderable_, current_line_text_, path);
			renderable_->setPosition(loc_.x(), loc_.y() + (loc_.h() - r.h()) / 2 + fh_->getBaseline()/65536);
		}
	}

	void TextEdit::setText(const std::string& text)
	{
		current_line_text_ = text;
		init();
	}

	void TextEdit::setHandlers(change_handler onchange)
	{
		on_change_ = onchange;
	}

	void TextEdit::preRender(const WindowPtr& wm)
	{
	}

	bool TextEdit::handleMouseMotion(bool claimed, const point& p, unsigned keymod, bool in_rect) 
	{
		return claimed;
	}

	bool TextEdit::handleMouseButtonUp(bool claimed, const point& p, unsigned buttons, unsigned keymod, bool in_rect) 
	{
		return claimed;
	}

	bool TextEdit::handleMouseButtonDown(bool claimed, const point& p, unsigned buttons, unsigned keymod, bool in_rect) 
	{
		return claimed;
	}

	bool TextEdit::handleMouseWheel(bool claimed, const point& p, const point& delta, int direction, bool in_rect) 
	{
		return claimed;
	}

	bool TextEdit::handleKeyDown(bool claimed, const SDL_Keysym& keysym, bool repeat, bool pressed) 
	{
		LOG_INFO("key down: " << keysym.sym << "; repeat: " << (repeat ? "true" : "false") << "; " << (pressed ? "pressed" : "released"));
		//current_line_text_ +=  keysym.sym;
		//init();
		return claimed;
	}

	bool TextEdit::handleTextInput(bool claimed, const std::string& text) 
	{
		LOG_INFO("TextEdit::handleTextInput: " << text);
		current_line_text_ = text;
		init();
		return claimed;
	}

	bool TextEdit::handleTextEditing(bool claimed, const std::string& text, int start, int length) 
	{
		LOG_INFO("TextEdit::handleTextEditing: " << text << "; start: " << start << "; length: " << length);
		return claimed;
	}

	void TextEdit::setFont(const KRE::FontHandlePtr& fh) 
	{ 
		fh_ = fh;
		init();
	}
}
