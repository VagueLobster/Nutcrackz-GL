#pragma once

//#include <glm/glm.hpp>
#include "rtmcpp/Common.hpp"
//#include "rtmcpp/Transforms.hpp"

namespace Nutcrackz::Math {

	//bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
	//bool DecomposeTransformScale(const glm::mat4& transform, glm::vec3& scale);
	bool DecomposeTransform(const rtmcpp::Mat4& transform, rtmcpp::Vec3& translation, rtmcpp::Vec3& rotation, rtmcpp::Vec3& scale);
	bool DecomposeTransformScale(const rtmcpp::Mat4& transform, rtmcpp::Vec3& scale);
	
}