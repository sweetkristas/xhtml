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

#include <memory>
#include <vector>

#include "DisplayDeviceFwd.hpp"
#include "ScopeableValue.hpp"
#include "SceneFwd.hpp"
#include "WindowManagerFwd.hpp"

namespace KRE
{
	class SceneTree;
	typedef std::shared_ptr<SceneTree> SceneTreePtr;
	typedef std::weak_ptr<SceneTree> WeakSceneTreePtr;

	class SceneTree : public std::enable_shared_from_this<SceneTree>
	{
	public:
		virtual ~SceneTree() {}
		SceneTreePtr getParent() const { return parent_.lock(); }

		void addObject(const SceneObjectPtr& obj) { objects_.emplace_back(obj); }
		void clearObjects() { objects_.clear(); }
		void removeObject(const SceneObjectPtr& obj);
		void addChild(const SceneTreePtr& child) { children_.emplace_back(child); }

		void preRender(const WindowPtr& wnd);
		void render(const WindowPtr& wnd) const;

		void setPosition(const glm::vec3& position);
		void setPosition(float x, float y, float z=0.0f);
		void setPosition(int x, int y, int z=0);
		const glm::vec3& getPosition() const { return position_; }

		void setRotation(float angle, const glm::vec3& axis);
		void setRotation(const glm::quat& rot);
		const glm::quat& getRotation() const { return rotation_; }

		void setScale(float xs, float ys, float zs=1.0f);
		void setScale(const glm::vec3& scale);
		const glm::vec3& getScale() const { return scale_; }

		void clearRenderTargets() { render_targets_.clear(); }
		void addRenderTarget(const RenderTargetPtr& render_target) { render_targets_.emplace_back(render_target); }
		const std::vector<RenderTargetPtr>& getRenderTargets() const { return render_targets_; }

		const glm::mat4& getModelMatrix() const;

		static SceneTreePtr create(SceneTreePtr parent);
	protected:
		explicit SceneTree(const SceneTreePtr& parent);
	private:

		WeakSceneTreePtr parent_;
		std::vector<SceneTreePtr> children_;
		std::vector<SceneObjectPtr> objects_;

		ScopeableValue scopeable_;
		CameraPtr camera_;
		std::vector<RenderTargetPtr> render_targets_;
		rect render_target_window_;
		
		RenderablePtr clip_shape_;

		glm::vec3 position_;
		glm::quat rotation_;
		glm::vec3 scale_;

		bool model_changed_;
		mutable glm::mat4 model_matrix_;

		ColorPtr color_;
	};

	const glm::vec3& get_xaxis();
	const glm::vec3& get_yaxis();
	const glm::vec3& get_zaxis();
	const glm::mat4& get_identity_matrix();
}
