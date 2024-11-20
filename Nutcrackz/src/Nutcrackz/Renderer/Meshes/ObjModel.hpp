#pragma once

#include "Nutcrackz/Asset/Asset.hpp"

#include <stdint.h>

//#include <glm/glm.hpp>
#include "rtmcpp/Common.hpp"

namespace Nutcrackz {

	struct Material
	{
		rtmcpp::Vec3 MaterialSpecular = rtmcpp::Vec3(0.5f, 0.5f, 0.5f);
		float MaterialShininess = 64.0f;
	};

	class ObjModel : public Asset
	{
	public:
		ObjModel();
		~ObjModel();

		virtual AssetType GetType() const { return AssetType::ObjModel; }

		AssetHandle GetTextureHandle()
		{
			if (m_TextureHandle != 0)
				return m_TextureHandle;
			else
				return AssetHandle(0);
		}

		rtmcpp::Vec3& GetMaterialSpecular() { return m_MaterialProperties.MaterialSpecular; }
		float GetMaterialShininess() { return m_MaterialProperties.MaterialShininess; }
		bool HasLoadedFile() { return m_HasLoadedFile; }
		std::vector<rtmcpp::Vec3> GetMeshPositionVertices() { return m_MeshPositionVertices; }
		std::vector<rtmcpp::Vec2> GetMeshUVVertices() { return m_MeshUVVertices; }
		std::vector<rtmcpp::Vec3> GetMeshNormalVertices() { return m_MeshNormalVertices; }
		std::vector<uint32_t> GetMeshPositionIndices() { return m_MeshPositionIndices; }
		std::vector<uint32_t> GetMeshUVIndices() { return m_MeshUVIndices; }
		std::vector<uint32_t> GetMeshNormalIndices() { return m_MeshNormalIndices; }

		void SetTextureHandle(AssetHandle handle) { m_TextureHandle = handle; }
		void SetMaterialSpecular(const rtmcpp::Vec3& specular) { m_MaterialProperties.MaterialSpecular = rtmcpp::Vec3{ specular.X, specular.Y, specular.Z }; }
		void SetMaterialShininess(float shininess) { m_MaterialProperties.MaterialShininess = shininess; }

		void SetLoadedFile(bool value) { m_HasLoadedFile = value; }
		void SetMeshPositionVertices(const std::vector<rtmcpp::Vec3>& positionVertices) { m_MeshPositionVertices = positionVertices; }
		void SetMeshUVVertices(const std::vector<rtmcpp::Vec2>& uvVertices) { m_MeshUVVertices = uvVertices; }
		void SetMeshNormalVertices(const std::vector<rtmcpp::Vec3>& normalVertices) { m_MeshNormalVertices = normalVertices; }
		void SetMeshPositionIndices(const std::vector<uint32_t>& positionIndices) { m_MeshPositionIndices = positionIndices; }
		void SetMeshUVIndices(const std::vector<uint32_t>& positionIndices) { m_MeshPositionIndices = positionIndices; }
		void SetMeshNormalIndices(const std::vector<uint32_t>& positionIndices) { m_MeshPositionIndices = positionIndices; }

	public:
		AssetHandle m_TextureHandle = 0;
		Material m_MaterialProperties;

	private:
		bool m_HasLoadedFile = false;

		std::vector<rtmcpp::Vec3> m_MeshPositionVertices;
		std::vector<rtmcpp::Vec2> m_MeshUVVertices;
		std::vector<rtmcpp::Vec3> m_MeshNormalVertices;
		std::vector<uint32_t> m_MeshPositionIndices, m_MeshUVIndices, m_MeshNormalIndices;
	};

}