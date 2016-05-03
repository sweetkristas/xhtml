#include <algorithm>

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
		  up_arrow_area_(),
		  down_arrow_area_(),
		  left_arrow_area_(),
		  right_arrow_area_(),
		  thumb_area_(),
		  visible_(false),
		  thumb_color_(205, 205, 205),
		  thumb_selected_color_(95, 95, 95),
		  thumb_mouseover_color_(166, 166, 166),
		  background_color_(240, 240, 240),
		  changed_(true),
		  vertices_()

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
		// number of different positions that we can scroll to.
		const int range = max_range_ - min_range_ + 1;

		const std::vector<std::string> arrow_files{ "scrollbar-up-arrow.svg", "scrollbar-down-arrow.svg", "scrollbar-left-arrow.svg", "scrollbar-right-arrow.svg", "scrollbar-background.svg" };
		std::vector<point> arrow_sizes{ point(loc_.w(), loc_.w()), point(loc_.w(), loc_.w()), point(loc_.h(), loc_.h()), point(loc_.h(), loc_.h()) };
		tex_ = svgs_to_single_texture(arrow_files, arrow_sizes, &tex_coords_);
		tex_->setAddressModes(0, Texture::AddressMode::WRAP, Texture::AddressMode::WRAP);

		if(dir_ == Direction::VERTICAL) {
			up_arrow_area_ = rect(loc_.x(), loc_.y(), loc_.w(), loc_.w());
			down_arrow_area_ = rect(loc_.x(), loc_.y2() - loc_.w(), loc_.w(), loc_.w());

			const int min_length = std::min(loc_.w(), (max_range_ - min_range_) / loc_.h());
			thumb_area_ = rect(loc_.x(), scroll_pos_ / range * loc_.h() + loc_.y(), loc_.w(), min_length);

		} else {
			left_arrow_area_ = rect(loc_.x(), loc_.y(), loc_.h(), loc_.h());
			right_arrow_area_ = rect(loc_.x2() - loc_.h(), loc_.y(), loc_.h(), loc_.h());

			const int min_length = std::min(loc_.h(), (max_range_ - min_range_) / loc_.w());
			thumb_area_ = rect(scroll_pos_ / range * loc_.h() + loc_.x(), loc_.y(), min_length, loc_.h());
		}
	}

	bool Scrollbar::handleMouseMotion(bool claimed, int x, int y)
	{
		if(!claimed && geometry::pointInRect(point(x,y), loc_)) {
			claimed = true;

			// test in the arrow regions and the thumb region.
		}
		return claimed;
	}

	bool Scrollbar::handleMouseButtonDown(bool claimed, int x, int y, unsigned button)
	{
		if(!claimed && geometry::pointInRect(point(x,y), loc_)) {
			claimed = true;
		}
		return claimed;
	}

	bool Scrollbar::handleMouseButtonUp(bool claimed, int x, int y, unsigned button)
	{
		if(!claimed && geometry::pointInRect(point(x,y), loc_)) {
			claimed = true;
		}
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
		if(changed_) {
			changed_ = false;
			// XXX recalculate attribute sets
		}
	}

	void Scrollbar::setRange(int minr, int maxr) 
	{ 
		min_range_ = minr; 
		max_range_ = maxr; 
		if(min_range_ > max_range_) {
			LOG_ERROR("Swapping min and max ranges as they do not satisfy the ordering criterion. " << min_range_ << " > " << max_range_);
			std::swap(min_range_, max_range_);
		}
		if(scroll_pos_ < min_range_) {
			scroll_pos_ = min_range_;
		}
		if(scroll_pos_ > max_range_) {
			scroll_pos_ = max_range_;
		}
	}
}
