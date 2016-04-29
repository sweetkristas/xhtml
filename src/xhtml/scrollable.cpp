#include "asserts.hpp"
#include "scrollable.hpp"

namespace scrollable
{
	Scrollbar::Scrollbar(Direction d, change_handler onchange)
		: on_change_(onchange),
		dir_(d),
		min_range_(0),
		max_range_(100),
		scroll_pos_(0),
		loc_()
	{
	}

	Scrollbar::~Scrollbar()
	{
	}

	void Scrollbar::setScrollPosition(int pos)
	{
		scroll_pos_ = pos;
		if (pos < min_range_) {
			LOG_WARN("Scrollbar::setScrollPosition() setting scroll position outside minimum range: " << pos << " < " << min_range_ << " defaulting to minimum.");
			scroll_pos_ = min_range_;
		}
		if (pos > max_range_) {
			LOG_WARN("Scrollbar::setScrollPosition() setting scroll position outside maximum range: " << pos << " < " << max_range_ << " defaulting to maximum.");
			scroll_pos_ = max_range_;
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
	}
}
