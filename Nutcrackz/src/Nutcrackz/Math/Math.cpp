#include "nzpch.hpp"
#include "Math.hpp"

#include <limits>

/*#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include <glm/gtx/norm.hpp>
#include <glm/gtx/quaternion.hpp>*/

namespace Nutcrackz::Math {

	template<typename genType>
	genType Epsilon()
	{
		NZ_CORE_ASSERT(std::numeric_limits<genType>::is_iec559, "'Epsilon' only accepts floating-point inputs");
		return std::numeric_limits<genType>::epsilon();
	}

	bool EpsilonEqual(const float& x, const float& y, const float& epsilon)
	{
		return abs(x - y) < epsilon;
	}

	bool EpsilonNotEqual(const float& x, const float& y, const float& epsilon)
	{
		return abs(x - y) >= epsilon;
	}

	template<typename T>
	rtmcpp::Vec3 Scale(const rtmcpp::Vec3& v, T desiredLength)
	{
		return rtmcpp::Vec3{ v.X * desiredLength / v.Length(), v.Y * desiredLength / v.Length(), v.Z * desiredLength / v.Length() };
	}

	bool DecomposeTransform(const rtmcpp::Mat4& transform, rtmcpp::Vec3& translation, rtmcpp::Vec3& rotation, rtmcpp::Vec3& scale)
	{
		// From glm::decompose in matrix_decompose.inl

		//using namespace glm;
		using T = float;

		rtmcpp::Mat4 LocalMatrix(transform);

		// Normalize the matrix.
		//if (rtm::vector_all_equal3())
		if (EpsilonEqual(LocalMatrix.Value.w_axis.m128_f32[3], static_cast<float>(0), Epsilon<T>()))
			return false;

		// First, isolate perspective. This is the messiest.
		if (
			EpsilonNotEqual(LocalMatrix.Value.x_axis.m128_f32[3], static_cast<T>(0), Epsilon<T>()) ||
			EpsilonNotEqual(LocalMatrix.Value.y_axis.m128_f32[3], static_cast<T>(0), Epsilon<T>()) ||
			EpsilonNotEqual(LocalMatrix.Value.z_axis.m128_f32[3], static_cast<T>(0), Epsilon<T>()))
		{
			LocalMatrix.Value.x_axis.m128_f32[3] = LocalMatrix.Value.y_axis.m128_f32[3] = LocalMatrix.Value.z_axis.m128_f32[3] = static_cast<T>(0);
			LocalMatrix.Value.w_axis.m128_f32[3] = static_cast<T>(1);
		}

		// Next take care of translation
		translation = rtmcpp::Vec3(LocalMatrix.Value.w_axis);
		LocalMatrix.Value.w_axis = rtm::vector_set(0, 0, 0, LocalMatrix.Value.w_axis.m128_f32[3]);

		rtmcpp::Vec3 Row[3];// , Pdum3;

		// Now get scale and shear.
		for (uint32_t i = 0; i < 3; i++)
		{
			for (uint32_t j = 0; j < 3; j++)
			{
				Row[i].X = LocalMatrix.Value.x_axis.m128_f32[j];
				Row[i].Y = LocalMatrix.Value.y_axis.m128_f32[j];
				Row[i].Z = LocalMatrix.Value.z_axis.m128_f32[j];
			}
		}

		// Compute X scale factor and normalize first row
		scale.X = Row[0].Length();
		Row[0] = Scale(Row[0], static_cast<T>(1));
		scale.Y = Row[1].Length();
		Row[1] = Scale(Row[1], static_cast<T>(1));
		scale.Z = Row[2].Length();
		Row[2] = Scale(Row[2], static_cast<T>(1));

		// At this point, the matrix (in rows[]) is orthonormal.
		// Check for a coordinate system flip. If the determinant
		// is -1, then negate the matrix and the scaling factors.
#if 0
		Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
		if (dot(Row[0], Pdum3) < 0)
		{
			for (length_t i = 0; i < 3; i++)
			{
				scale[i] *= static_cast<T>(-1);
				Row[i] *= static_cast<T>(-1);
			}
#endif

		rotation.Y = asin(-Row[0].Z);
		if (cos(rotation.Y) != 0)
		{
			rotation.X = atan2(Row[1].Z, Row[2].Z);
			rotation.Z = atan2(Row[0].Y, Row[0].X);
		}
		else
		{
			rotation.X = atan2(Row[2].X, Row[1].Y);
			rotation.Z = 0;
		}

		return true;
	}

	bool DecomposeTransformScale(const rtmcpp::Mat4 & transform, rtmcpp::Vec3 & scale)
	{
		//using namespace glm;
		using T = float;

		rtmcpp::Mat4 LocalMatrix(transform);

		// Normalize the matrix.
		if (EpsilonEqual(LocalMatrix.Value.w_axis.m128_f32[3], static_cast<float>(0), Epsilon<T>()))
			return false;

		// First, isolate perspective. This is the messiest.
		if (
			EpsilonNotEqual(LocalMatrix.Value.x_axis.m128_f32[3], static_cast<T>(0), Epsilon<T>()) ||
			EpsilonNotEqual(LocalMatrix.Value.y_axis.m128_f32[3], static_cast<T>(0), Epsilon<T>()) ||
			EpsilonNotEqual(LocalMatrix.Value.z_axis.m128_f32[3], static_cast<T>(0), Epsilon<T>()))
		{
			LocalMatrix.Value.x_axis.m128_f32[3] = LocalMatrix.Value.y_axis.m128_f32[3] = LocalMatrix.Value.z_axis.m128_f32[3] = static_cast<T>(0);
			LocalMatrix.Value.w_axis.m128_f32[3] = static_cast<T>(1);
		}

		rtmcpp::Vec3 Row[3];// , Pdum3;

		// Now get scale and shear.
		for (uint32_t i = 0; i < 3; i++)
		{
			for (uint32_t j = 0; j < 3; j++)
			{
				Row[i].X = LocalMatrix.Value.x_axis.m128_f32[j];
				Row[i].Y = LocalMatrix.Value.y_axis.m128_f32[j];
				Row[i].Z = LocalMatrix.Value.z_axis.m128_f32[j];
			}
		}


		// Compute X scale factor and normalize first row
		scale = rtmcpp::Vec3(Row[0].Length(), Row[1].Length(), Row[2].Length());
		//scale = glm::vec3(length(Row[0]), length(Row[1]), length(Row[2]));

		return true;
	}

	/*bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale)
	{
		// From glm::decompose in matrix_decompose.inl

		using namespace glm;
		using T = float;

		mat4 LocalMatrix(transform);

		// Normalize the matrix.
		if (epsilonEqual(LocalMatrix[3][3], static_cast<float>(0), epsilon<T>()))
			return false;

		// First, isolate perspective. This is the messiest.
		if (
			epsilonNotEqual(LocalMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(LocalMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(LocalMatrix[2][3], static_cast<T>(0), epsilon<T>()))
		{
			LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
			LocalMatrix[3][3] = static_cast<T>(1);
		}

		// Next take care of translation
		translation = vec3(LocalMatrix[3]);
		LocalMatrix[3] = vec4(0, 0, 0, LocalMatrix[3].w);

		vec3 Row[3];// , Pdum3;

		// Now get scale and shear.
		for (length_t i = 0; i < 3; i++)
			for (length_t j = 0; j < 3; j++)
				Row[i][j] = LocalMatrix[i][j];


		// Compute X scale factor and normalize first row
		scale.x = length(Row[0]);
		Row[0] = detail::scale(Row[0], static_cast<T>(1));
		scale.y = length(Row[1]);
		Row[1] = detail::scale(Row[1], static_cast<T>(1));
		scale.z = length(Row[2]);
		Row[2] = detail::scale(Row[2], static_cast<T>(1));

		// At this point, the matrix (in rows[]) is orthonormal.
		// Check for a coordinate system flip. If the determinant
		// is -1, then negate the matrix and the scaling factors.
#if 0
		Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
		if (dot(Row[0], Pdum3) < 0)
		{
			for (length_t i = 0; i < 3; i++)
			{
				scale[i] *= static_cast<T>(-1);
				Row[i] *= static_cast<T>(-1);
			}
#endif

			rotation.y = asin(-Row[0][2]);
			if (cos(rotation.y) != 0) {
				rotation.x = atan2(Row[1][2], Row[2][2]);
				rotation.z = atan2(Row[0][1], Row[0][0]);
			}
			else {
				rotation.x = atan2(Row[2][0], Row[1][1]);
				rotation.z = 0;
			}

			return true;
		}

		bool DecomposeTransformScale(const glm::mat4 & transform, glm::vec3 & scale)
		{
			using namespace glm;
			using T = float;

			mat4 LocalMatrix(transform);

			// Normalize the matrix.
			if (epsilonEqual(LocalMatrix[3][3], static_cast<float>(0), epsilon<T>()))
				return false;

			// First, isolate perspective. This is the messiest.
			if (
				epsilonNotEqual(LocalMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
				epsilonNotEqual(LocalMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
				epsilonNotEqual(LocalMatrix[2][3], static_cast<T>(0), epsilon<T>()))
			{
				LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
				LocalMatrix[3][3] = static_cast<T>(1);
			}

			vec3 Row[3];// , Pdum3;

			// Now get scale and shear.
			for (length_t i = 0; i < 3; i++)
				for (length_t j = 0; j < 3; j++)
					Row[i][j] = LocalMatrix[i][j];


			// Compute X scale factor and normalize first row
			scale = glm::vec3(length(Row[0]), length(Row[1]), length(Row[2]));

			return true;
		}*/

}
