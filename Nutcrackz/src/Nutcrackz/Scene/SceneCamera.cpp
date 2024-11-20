#include "nzpch.hpp"
#include "SceneCamera.hpp"

#include "rtmcpp/Transforms.hpp"

namespace Nutcrackz {

	SceneCamera::SceneCamera()
	{
		RecalculateProjection();
	}

	void SceneCamera::SetPerspective(float verticalFOV, float nearClip, float farClip)
	{
		m_ProjectionType = ProjectionType::Perspective;
		m_PerspectiveFOV = verticalFOV;
		m_PerspectiveNear = nearClip;
		m_PerspectiveFar = farClip;
		RecalculateProjection();
	}

	void SceneCamera::SetOrthographic(float size, float nearClip, float farClip)
	{
		m_ProjectionType = ProjectionType::Orthographic;
		m_OrthographicSize = size;
		m_OrthographicNear = nearClip;
		m_OrthographicFar = farClip;
		RecalculateProjection();
	}

	void SceneCamera::SetViewportSize(uint32_t width, uint32_t height)
	{
		if (width <= 0 && height <= 0)
			return;

		//NZ_CORE_ASSERT(width > 0 && height > 0);
		m_Width = width;
		m_Height = height;
		m_AspectRatio = (float)width / (float)height;
		RecalculateProjection();
	}

	void SceneCamera::RecalculateProjection()
	{
		if (m_ProjectionType == ProjectionType::Perspective)
		{
			m_Projection = rtmcpp::Mat4::PerspectiveInfReversedZ(m_PerspectiveFOV, m_AspectRatio, m_PerspectiveNear);
		}
		else
		{
			float orthoLeft = -m_OrthographicSize * m_AspectRatio * 0.5f;
			float orthoRight = m_OrthographicSize * m_AspectRatio * 0.5f;
			float orthoBottom = -m_OrthographicSize * 0.5f;
			float orthoTop = m_OrthographicSize * 0.5f;

			//m_Projection = rtmcpp::Mat4::Orthographic(orthoLeft, orthoRight, orthoBottom, orthoTop, m_OrthographicNear, m_OrthographicFar);
			m_Projection = rtmcpp::Mat4::Orthographic(orthoRight - orthoLeft, orthoTop - orthoBottom, m_OrthographicNear, m_OrthographicFar);
		}
	}

	RefPtr<SceneCamera> SceneCamera::Create()
	{
		return RefPtr<SceneCamera>::Create();
	}

}
