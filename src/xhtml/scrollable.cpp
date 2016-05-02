#include "asserts.hpp"
#include "scrollable.hpp"

#include "easy_svg.hpp"
#include "WindowManager.hpp"

namespace scrollable
{
	using namespace KRE;

	Scrollbar::Scrollbar(Direction d, change_handler onchange)
		: SceneObject("Scrollbar"),
		  on_change_(onchange),
		  dir_(d),
		  min_range_(0),
		  max_range_(100),
		  scroll_pos_(0),
		  loc_(),
		  visible_(false),
		  thumb_color_(205, 205, 205),
		  thumb_selected_color_(95, 95, 95),
		  thumb_mouseover_color_(166, 166, 166),
		  background_color_(240, 240, 240),
		  up_arrow_(),
	      down_arrow_(),
		  left_arrow_(),
		  right_arrow_()

	{
	}

	Scrollbar::~Scrollbar()
	{
	}

	void Scrollbar::setScrollPosition(int pos)
	{
		scroll_pos_ = pos;
		if(pos < min_range_) {
			LOG_WARN("Scrollbar::setScrollPosition() setting scroll position outside minimum range: " << pos << " < " << min_range_ << " defaulting to minimum.");
			scroll_pos_ = min_range_;
		}
		if(pos > max_range_) {
			LOG_WARN("Scrollbar::setScrollPosition() setting scroll position outside maximum range: " << pos << " > " << max_range_ << " defaulting to maximum.");
			scroll_pos_ = max_range_;
		}
	}

	void Scrollbar::init()
	{
		if(dir_ == Direction::VERTICAL) {
			up_arrow_ = svg_texture_from_file("scrollbar-up-arrow.svg", loc_.w(), loc_.w());
			down_arrow_ = svg_texture_from_file("scrollbar-down-arrow.svg", loc_.w(), loc_.w());
		} else {
			left_arrow_ = svg_texture_from_file("scrollbar-left-arrow.svg", loc_.h(), loc_.h());
			right_arrow_ = svg_texture_from_file("scrollbar-right-arrow.svg", loc_.h(), loc_.h());
		}
	}

	bool Scrollbar::handleMouseMotion(bool claimed, int x, int y)
	{
		return claimed;
	}

	bool Scrollbar::handleMouseButtonDown(bool claimed, int x, int y, unsigned button)
	{
		return claimed;
	}

	bool Scrollbar::handleMouseButtonUp(bool claimed, int x, int y, unsigned button)
	{
		return claimed;
	}

	void Scrollbar::setLocation(int x, int y)
	{
		loc_.set_xy(x, y);
	}

	void Scrollbar::setDimensions(int w, int h)
	{
		loc_.set_wh(w, h);
		init();
	}

	void Scrollbar::preRender(const WindowPtr& wm)
	{
	}
}
