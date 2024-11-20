#include "nzpch.hpp"
#include "ObjModelImporter.hpp"

#include "Nutcrackz/Project/Project.hpp"
#include "Nutcrackz/Scene/SceneSerializer.hpp"

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
// Optional. define TINYOBJLOADER_USE_MAPBOX_EARCUT gives robust triangulation. Requires C++11
//#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "tiny_obj_loader.h"

namespace Nutcrackz {

	RefPtr<ObjModel> ObjModelImporter::ImportObjModel(AssetHandle handle, const AssetMetadata& metadata)
	{
		//NZ_PROFILE_FUNCTION();

		return LoadTinyObjModel(Project::GetActiveAssetDirectory() / metadata.FilePath);
	}

	RefPtr<ObjModel> ObjModelImporter::LoadTinyObjModel(const std::filesystem::path& path)
	{
		RefPtr<ObjModel> model = RefPtr<ObjModel>::Create();

		tinyobj::ObjReaderConfig readerConfig;
		readerConfig.mtl_search_path = "./"; // Path to material files

		tinyobj::ObjReader reader;

		if (!reader.ParseFromFile(path.string(), readerConfig))
		{
			if (!reader.Error().empty())
			{
				NZ_CORE_ERROR("TinyObjReader: {0}", reader.Error());
			}

			return nullptr;
		}

		model->GetMeshPositionIndices().clear();
		model->GetMeshPositionIndices().shrink_to_fit();
		model->GetMeshUVIndices().clear();
		model->GetMeshUVIndices().shrink_to_fit();
		model->GetMeshNormalIndices().clear();
		model->GetMeshNormalIndices().shrink_to_fit();

		model->GetMeshPositionVertices().clear();
		model->GetMeshPositionVertices().shrink_to_fit();
		model->GetMeshUVVertices().clear();
		model->GetMeshUVVertices().shrink_to_fit();
		model->GetMeshNormalVertices().clear();
		model->GetMeshNormalVertices().shrink_to_fit();

		if (!reader.Warning().empty())
		{
			NZ_CORE_WARN("TinyObjReader: {0}", reader.Warning());
		}

		auto& attrib = reader.GetAttrib();
		auto& shapes = reader.GetShapes();
		auto& materials = reader.GetMaterials();

		std::vector<rtmcpp::Vec3> meshPositionVertices;
		std::vector<rtmcpp::Vec2> meshUVVertices;
		std::vector<rtmcpp::Vec3> meshNormalVertices;
		std::vector<uint32_t> meshPositionIndices, meshUVIndices, meshNormalIndices;

		// Loop over shapes
		for (size_t s = 0; s < shapes.size(); s++)
		{
			// Loop over faces(polygon)
			size_t indexOffset = 0;
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
			{
				size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

				// Loop over vertices in the face.
				for (size_t v = 0; v < fv; v++)
				{
					// TODO: Convert idx, vx, vy, vz, nx, ny, nz, tx and ty into variables that can
					// be read from s_Data... like: s_Data.VertexInddex, s_Data.VertexX, etc...

					// access to vertex
					tinyobj::index_t idx = shapes[s].mesh.indices[indexOffset + v];
					tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
					tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
					tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

					meshPositionIndices.push_back(idx.vertex_index);
					meshUVIndices.push_back(idx.texcoord_index);
					meshNormalIndices.push_back(idx.normal_index);

					meshPositionVertices.push_back(rtmcpp::Vec3((float)vx, (float)vy, (float)vz));

					// Check if `normal_index` is zero or positive. negative = no normal data
					if (idx.normal_index >= 0)
					{
						tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
						tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
						tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];

						meshNormalVertices.push_back(rtmcpp::Vec3((float)nx, (float)ny, (float)nz));
					}

					// Check if `texcoord_index` is zero or positive. negative = no texcoord data
					if (idx.texcoord_index >= 0)
					{
						tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
						tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];

						meshUVVertices.push_back(rtmcpp::Vec2((float)tx, (float)ty));
					}

					// Optional: vertex colors
					// tinyobj::real_t red   = attrib.colors[3*size_t(idx.vertex_index)+0];
					// tinyobj::real_t green = attrib.colors[3*size_t(idx.vertex_index)+1];
					// tinyobj::real_t blue  = attrib.colors[3*size_t(idx.vertex_index)+2];
				}
				indexOffset += fv;

				// per-face material
				//shapes[s].mesh.material_ids[f];
			}
		}

		model->SetMeshPositionIndices(meshPositionIndices);
		model->SetMeshUVIndices(meshUVIndices);
		model->SetMeshNormalIndices(meshNormalIndices);

		model->SetMeshPositionVertices(meshPositionVertices);
		model->SetMeshUVVertices(meshUVVertices);
		model->SetMeshNormalVertices(meshNormalVertices);

		NZ_CORE_WARN("Vertex Positions: {0}", model->GetMeshPositionVertices().size());

		return model;
	}

}