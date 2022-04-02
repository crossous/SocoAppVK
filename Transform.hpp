#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

class Transform
{
private:
	glm::vec3 mLocalPosition{ 0, 0, 0 };
	glm::quat mLocalRotation = { 1, 0, 0, 0 };
	glm::vec3 mLocalScale{ 1, 1, 1 };

public:
	Transform* mParent = nullptr;

	Transform() {}
	Transform(glm::vec3 position, Transform* parent = nullptr) : mLocalPosition(position), mParent(parent){}
	Transform(glm::vec3 position, glm::quat rotation, Transform* parent = nullptr) : mLocalPosition(position), mLocalRotation(rotation), mParent(parent) {}
	Transform(glm::vec3 position, glm::quat rotation, glm::vec3 scale, Transform* parent = nullptr) : mLocalPosition(position), mLocalRotation(rotation), mLocalScale(scale), mParent(parent) {}

	inline glm::vec3 GetLocalPosition() const { return mLocalPosition; }
	inline glm::quat GetLocalRotation() const { return mLocalRotation; }
	inline glm::vec3 GetLocalScale() const { return mLocalScale; }

	inline void SetLocalPosition(glm::vec3 position) { mLocalPosition = position; }
	inline void SetLocalRotation(glm::quat rotation) { mLocalRotation = rotation; }
	inline void SetLocalScale(glm::vec3 scale) { mLocalScale = scale; }

	inline glm::vec3 GetGlobalPosition() const {
		return mParent == nullptr ? mLocalPosition : glm::vec3(mParent->GetGlobalMatrix() * glm::vec4(mLocalPosition, 1));
	}

	inline glm::mat4 GetLocalMatrix() const
	{
		return glm::scale(glm::mat4(1), mLocalScale) * glm::mat4_cast(mLocalRotation) * glm::translate(glm::mat4(1), mLocalPosition);
	}

	inline glm::mat4 GetGlobalMatrix() const
	{
		return mParent == nullptr ? GetLocalMatrix() : mParent->GetGlobalMatrix() * GetLocalMatrix();
	}

	inline glm::vec3 GetGlobalForward() const
	{
		return glm::vec3(GetGlobalMatrix() * glm::vec4(0, 0, 1, 0));
	}
};