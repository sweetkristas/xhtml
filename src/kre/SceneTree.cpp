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

#include "CameraObject.hpp"
#include "BlendModeScope.hpp"
#include "ClipScope.hpp"
#include "ColorScope.hpp"
#include "DisplayDevice.hpp"
#include "LightObject.hpp"
#include "Renderable.hpp"
#include "RenderManager.hpp"
#include "RenderTarget.hpp"
#include "SceneObject.hpp"
#include "SceneTree.hpp"
#include "WindowManager.hpp"
#include "variant_utils.hpp"

namespace KRE
{
	namespace
	{
		struct CameraScope
		{
			CameraScope(const CameraPtr& cam) 
				: old_cam(nullptr)
			{
				if(cam) {
					old_cam = DisplayDevice::getCurrent()->setDefaultCamera(cam);
				}
			}
			~CameraScope()
			{
				if(old_cam) {
					DisplayDevice::getCurrent()->setDefaultCamera(old_cam);
				}
			}
			CameraPtr old_cam;
		};

		struct SceneTreeImpl : public SceneTree
		{
			SceneTreeImpl(const SceneTreePtr& parent) : SceneTree(parent) {}
		};
	}

	SceneTree::SceneTree(const SceneTreePtr& parent)
		: parent_(parent),
		  children_(),
		  objects_(),
		  scopeable_(),
		  camera_(nullptr),
		  render_targets_(),
		  render_target_window_(),
		  clip_shape_(nullptr),
		  position_(0.0f),
		  rotation_(1.0f, 0.0f, 0.0f, 0.0f),
		  scale_(1.0f),
		  model_changed_(true),
		  model_matrix_(1.0f),
		  color_(nullptr)
	{
	}

	SceneTreePtr SceneTree::create(SceneTreePtr parent)
	{
		return std::make_shared<SceneTreeImpl>(parent);
	}

	void SceneTree::removeObject(const SceneObjectPtr& obj)
	{
		objects_.erase(std::remove_if(objects_.begin(), objects_.end(), [obj](const SceneObjectPtr& object) {
			return object == obj;
		}), objects_.end());
	}

	void SceneTree::setPosition(const glm::vec3& position) 
	{
		position_ = position;
		model_changed_ = true;
	}

	void SceneTree::setPosition(float x, float y, float z) 
	{
		position_ = glm::vec3(x, y, z);
		model_changed_ = true;
	}

	void SceneTree::setPosition(int x, int y, int z) 
	{
		position_ = glm::vec3(float(x), float(y), float(z));
		model_changed_ = true;
	}

	void SceneTree::setRotation(float angle, const glm::vec3& axis) 
	{
		rotation_ = glm::angleAxis(angle, axis);
		model_changed_ = true;
	}

	void SceneTree::setRotation(const glm::quat& rot) 
	{
		rotation_ = rot;
		model_changed_ = true;
	}

	void SceneTree::setScale(float xs, float ys, float zs) 
	{
		scale_ = glm::vec3(xs, ys, zs);
		model_changed_ = true;
	}

	void SceneTree::setScale(const glm::vec3& scale) 
	{
		scale_ = scale;
		model_changed_ = true;
	}

	const glm::mat4& SceneTree::getModelMatrix() const 
	{
		if(model_changed_) {
			model_matrix_ = glm::translate(get_identity_matrix(), position_) * glm::toMat4(rotation_) * glm::scale(get_identity_matrix(), scale_);
		}
		return model_matrix_;
	}

	void SceneTree::preRender(const WindowPtr& wnd)
	{
		for(auto& obj : objects_) {
			obj->preRender(wnd);
		}

		for(auto& child : children_) {
			child->preRender(wnd);
		}
	}

	void SceneTree::render(const WindowPtr& wnd) const
	{
		//if(scopeable_.isBlendEnabled()) {
		//}

		CameraScope cs(camera_);
		ClipShapeScope::Manager cssm(clip_shape_, nullptr);
		// blend
		// color
		//BlendModeScope bms(scopeable_);
		//BlendEquationScope bes(scopeable_);
		ColorScope color_scope(color_);

		//XXX set global model matrix here. getModelMatrix()

		// render all the objects and children into a render target if one exists.
		// which is why we introduce a new scope
		{
			RenderTarget::RenderScope(!render_targets_.empty() ? render_targets_.front() : nullptr, render_target_window_);

			for(auto& obj : objects_) {
				wnd->render(obj.get());
			}

			for(auto& child : children_) {
				child->render(wnd);
			}
		}

		if(render_targets_.size() > 1) {
			for(auto it = render_targets_.cbegin() + 1; it != render_targets_.cend(); ) {
				RenderTarget::RenderScope(*it, render_target_window_);
				wnd->render((it-1)->get());
			}
		}

		// Output the last render target
		if(!render_targets_.empty()) {
			wnd->render(render_targets_.back().get());
		}
	}

	const glm::vec3& get_xaxis()
	{
		static glm::vec3 x_axis(1.0f, 0.0f, 0.0f);
		return x_axis;
	}

	const glm::vec3& get_yaxis()
	{
		static glm::vec3 y_axis(0.0f, 1.0f, 0.0f);
		return y_axis;
	}

	const glm::vec3& get_zaxis()
	{
		static glm::vec3 z_axis(0.0f, 0.0f, 1.0f);
		return z_axis;
	}

	const glm::mat4& get_identity_matrix()
	{
		static glm::mat4 imatrix(1.0f);
		return imatrix;
	}
}
