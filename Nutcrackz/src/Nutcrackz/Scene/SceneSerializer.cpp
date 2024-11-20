#include "nzpch.hpp"
#include "SceneSerializer.hpp"

#include "Entity.hpp"
#include "Components.hpp"
#include "Nutcrackz/Scripting/ScriptEngine.hpp"
#include "Nutcrackz/Core/UUID.hpp"

#include "Nutcrackz/Project/Project.hpp"
#include "Nutcrackz/Renderer/Renderer3D.hpp"
#include "Nutcrackz/Core/Hash.hpp"

#include "Nutcrackz/Core/Timer.hpp"

#include <Coral/String.hpp>

#include <magic_enum.hpp>

#include <fstream>

#include <yaml-cpp/yaml.h>

#include "rtmcpp/Common.hpp"

#include "yyjson.h"

namespace YAML {

	template<>
	struct convert<rtmcpp::Vec2>
	{
		static Node encode(const rtmcpp::Vec2& rhs)
		{
			Node node;
			node.push_back(rhs.X);
			node.push_back(rhs.Y);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, rtmcpp::Vec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.X = node[0].as<float>();
			rhs.Y = node[1].as<float>();
			return true;
		}
	};

	/*template<>
	struct convert<glm::ivec2>
	{
		static Node encode(const glm::ivec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::ivec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<int>();
			rhs.y = node[1].as<int>();
			return true;
		}
	};*/

	template<>
	struct convert<rtmcpp::Vec3>
	{
		static Node encode(const rtmcpp::Vec3& rhs)
		{
			Node node;
			node.push_back(rhs.X);
			node.push_back(rhs.Y);
			node.push_back(rhs.Z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, rtmcpp::Vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.X = node[0].as<float>();
			rhs.Y = node[1].as<float>();
			rhs.Z = node[2].as<float>();
			return true;
		}
	};

	/*template<>
	struct convert<glm::ivec3>
	{
		static Node encode(const glm::ivec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::ivec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<int>();
			rhs.y = node[1].as<int>();
			rhs.y = node[2].as<int>();
			return true;
		}
	};*/

	template<>
	struct convert<rtmcpp::Vec4>
	{
		static Node encode(const rtmcpp::Vec4& rhs)
		{
			Node node;
			node.push_back(rhs.X);
			node.push_back(rhs.Y);
			node.push_back(rhs.Z);
			node.push_back(rhs.W);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, rtmcpp::Vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.X = node[0].as<float>();
			rhs.Y = node[1].as<float>();
			rhs.Z = node[2].as<float>();
			rhs.W = node[3].as<float>();
			return true;
		}
	};

	template<>
	struct convert<Nutcrackz::UUID>
	{
		static Node encode(const Nutcrackz::UUID& uuid)
		{
			Node node;
			node.push_back((uint64_t)uuid);
			return node;
		}

		static bool decode(const Node& node, Nutcrackz::UUID& uuid)
		{
			uuid = node.as<uint64_t>();
			return true;
		}
	};

	template<>
	struct convert<Coral::String>
	{
		static Node encode(const Coral::String& str)
		{
			Node node;
			std::string inStr = str;
			node.push_back(inStr);
			return node;
		}

		static bool decode(const Node& node, Coral::String& str)
		{
			str = Coral::String::New(node.as<std::string>());
			return true;
		}
	};

	template<>
	struct convert<flecs::entity>
	{
		static Node encode(const flecs::entity& id)
		{
			Node node;
			node.push_back((uint64_t)id);
			return node;
		}

		static bool decode(const Node& node, flecs::entity& id)
		{
			id = node.as<flecs::entity>();
			return true;
		}
	};

}
namespace Nutcrackz {

#define WRITE_SCRIPT_FIELD(FieldType, Type)           \
			case ScriptFieldType::FieldType:          \
				out << scriptField.GetValue<Type>();  \
				break

#define READ_SCRIPT_FIELD(FieldType, Type)            \
	case ScriptFieldType::FieldType:                  \
	{                                                 \
		Type data = scriptField["Data"].as<Type>();   \
		fieldInstance.SetValue(data);                 \
		break;                                        \
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const rtmcpp::Vec2& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.X << v.Y << YAML::EndSeq;
		return out;
	}

	//YAML::Emitter& operator<<(YAML::Emitter& out, const glm::ivec2& v)
	//{
	//	out << YAML::Flow;
	//	out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
	//	return out;
	//}

	YAML::Emitter& operator<<(YAML::Emitter& out, const rtmcpp::Vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.X << v.Y << v.Z << YAML::EndSeq;
		return out;
	}

	//YAML::Emitter& operator<<(YAML::Emitter& out, const glm::ivec3& v)
	//{
	//	out << YAML::Flow;
	//	out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
	//	return out;
	//}

	YAML::Emitter& operator<<(YAML::Emitter& out, const rtmcpp::Vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.X << v.Y << v.Z << v.W << YAML::EndSeq;
		return out;
	}

	template<typename T>
	inline T TrySetEnum(T& value, const YAML::Node& node)
	{
		if (node)
			value = static_cast<T>(node.as<int>(static_cast<int>(value)));
		return value;
	}

	void Vec2ToJson(yyjson_mut_doc* doc, yyjson_mut_val* root, const char* key, const rtmcpp::Vec2& P)
	{
		yyjson_mut_obj_add_str(doc, root, key, key);
		yyjson_mut_obj_add_real(doc, root, "x", P.X);
		yyjson_mut_obj_add_real(doc, root, "y", P.Y);
	};

	void Vec2FromJson(yyjson_val* obj, const char* key, rtmcpp::Vec2& P)
	{
		yyjson_val* str = yyjson_obj_get(obj, key);
		yyjson_val* x = yyjson_obj_get(obj, "x");
		yyjson_val* y = yyjson_obj_get(obj, "y");
		P = rtmcpp::Vec2((float)yyjson_get_real(x), (float)yyjson_get_real(y));
	}

	void Vec3ToJson(yyjson_mut_doc* doc, yyjson_mut_val* root, const char* key, const rtmcpp::Vec3& P)
	{
		yyjson_mut_obj_add_str(doc, root, key, key);
		yyjson_mut_obj_add_real(doc, root, "x", P.X);
		yyjson_mut_obj_add_real(doc, root, "y", P.Y);
		yyjson_mut_obj_add_real(doc, root, "z", P.Z);
	};

	void Vec3FromJson(yyjson_val* obj, const char* key, rtmcpp::Vec3& P)
	{
		yyjson_val* str = yyjson_obj_get(obj, key);
		yyjson_val* x = yyjson_obj_get(obj, "x");
		yyjson_val* y = yyjson_obj_get(obj, "y");
		yyjson_val* z = yyjson_obj_get(obj, "z");
		P = rtmcpp::Vec3((float)yyjson_get_real(x), (float)yyjson_get_real(y), (float)yyjson_get_real(z));
	}

	void Vec4ToJson(yyjson_mut_doc* doc, yyjson_mut_val* root, const char* key, const rtmcpp::Vec4& P)
	{
		yyjson_mut_obj_add_str(doc, root, key, key);
		yyjson_mut_obj_add_real(doc, root, "x", P.X);
		yyjson_mut_obj_add_real(doc, root, "y", P.Y);
		yyjson_mut_obj_add_real(doc, root, "z", P.Z);
		yyjson_mut_obj_add_real(doc, root, "w", P.W);
	};

	void Vec4FromJson(yyjson_val* obj, const char* key, rtmcpp::Vec4& P)
	{
		yyjson_val* str = yyjson_obj_get(obj, key);
		yyjson_val* x = yyjson_obj_get(obj, "x");
		yyjson_val* y = yyjson_obj_get(obj, "y");
		yyjson_val* z = yyjson_obj_get(obj, "z");
		yyjson_val* w = yyjson_obj_get(obj, "w");
		P = rtmcpp::Vec4((float)yyjson_get_real(x), (float)yyjson_get_real(y), (float)yyjson_get_real(z), (float)yyjson_get_real(w));
	}

	//template<typename T>
	//inline T TrySetEnum(T& value, yyjson_val* obj, const char* key)
	//{
	//	if (obj)
	//	{
	//		yyjson_val* val = yyjson_obj_get(obj, key);
	//
	//		value = static_cast<T>(node.as<int>(static_cast<int>(value)));
	//		return value;
	//	}
	//}

#pragma region SceneSerializer
	SceneSerializer::SceneSerializer(const RefPtr<Scene>& scene)
		: m_Scene(scene)
	{
	}

	void SceneSerializer::SerializeEntity(YAML::Emitter& out, Entity entity, RefPtr<Scene> scene)
	{
		//UUID uuid = entity.GetComponent<IDComponent>().ID;
		uint64_t id = entity.GetEntityHandle();
		out << YAML::BeginMap; // Entity
		out << YAML::Key << "Entity" << YAML::Value << id;

		if (entity.HasComponent<TagComponent>())
		{
			out << YAML::Key << "TagComponent";
			out << YAML::BeginMap; // TagComponent

			auto& tag = entity.GetComponent<TagComponent>().Tag;
			out << YAML::Key << "Tag" << YAML::Value << tag;

			out << YAML::EndMap; // TagComponent
		}

		if (entity.HasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap; // TransformComponent

			auto& tc = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "Enabled" << YAML::Value << tc.Enabled;
			out << YAML::Key << "Translation" << YAML::Value << tc.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << tc.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << tc.Scale;

			out << YAML::EndMap; // TransformComponent
		}

		if (entity.HasComponent<CameraComponent>())
		{
			out << YAML::Key << "CameraComponent";
			out << YAML::BeginMap; // CameraComponent

			auto& cameraComponent = entity.GetComponent<CameraComponent>();
			auto& camera = cameraComponent.Camera;

			out << YAML::Key << "Camera" << YAML::Value;
			out << YAML::BeginMap; // Camera
			out << YAML::Key << "ProjectionType" << YAML::Value << (int)camera->GetProjectionType();
			out << YAML::Key << "PerspectiveFOV" << YAML::Value << camera->GetPerspectiveVerticalFOV();
			out << YAML::Key << "PerspectiveNear" << YAML::Value << camera->GetPerspectiveNearClip();
			out << YAML::Key << "PerspectiveFar" << YAML::Value << camera->GetPerspectiveFarClip();
			out << YAML::Key << "OrthographicSize" << YAML::Value << camera->GetOrthographicSize();
			out << YAML::Key << "OrthographicNear" << YAML::Value << camera->GetOrthographicNearClip();
			out << YAML::Key << "OrthographicFar" << YAML::Value << camera->GetOrthographicFarClip();
			out << YAML::EndMap; // Camera

			out << YAML::Key << "Primary" << YAML::Value << cameraComponent.Primary;
			out << YAML::Key << "FixedAspectRatio" << YAML::Value << cameraComponent.FixedAspectRatio;

			out << YAML::EndMap; // CameraComponent
		}

		if (entity.HasComponent<ScriptComponent>())
		{
			auto& scriptComponent = entity.GetComponent<ScriptComponent>();
		
			out << YAML::Key << "ScriptComponent";
			out << YAML::BeginMap; // ScriptComponent
		
			const auto& scriptEngine = ScriptEngine::GetInstance();
			const auto& sc = entity.GetComponent<ScriptComponent>();

			if (scriptEngine.IsValidScript(sc.ScriptHandle))
			{
				const auto& scriptMetadata = scriptEngine.GetScriptMetadata(sc.ScriptHandle);
				const auto& entityStorage = scene->m_ScriptStorage.EntityStorage.at(entity.GetEntityHandle());

				out << YAML::Key << "ScriptHandle" << YAML::Value << sc.ScriptHandle;
				out << YAML::Key << "ScriptName" << YAML::Value << scriptMetadata.FullName;

				out << YAML::Key << "Fields" << YAML::Value << YAML::BeginSeq;
				for (const auto& [fieldID, fieldStorage] : entityStorage.Fields)
				{
					const auto& fieldMetadata = scriptMetadata.Fields.at(fieldID);

					out << YAML::BeginMap;
					out << YAML::Key << "ID" << YAML::Value << fieldID;
					out << YAML::Key << "Name" << YAML::Value << fieldMetadata.Name;
					out << YAML::Key << "Type" << YAML::Value << std::string(magic_enum::enum_name(fieldMetadata.Type));
					out << YAML::Key << "Value" << YAML::Value;

					switch (fieldMetadata.Type)
					{
					case DataType::SByte:
						out << fieldStorage.GetValue<int8_t>();
						break;
					case DataType::Byte:
						out << fieldStorage.GetValue<uint8_t>();
						break;
					case DataType::Short:
						out << fieldStorage.GetValue<int16_t>();
						break;
					case DataType::UShort:
						out << fieldStorage.GetValue<uint16_t>();
						break;
					case DataType::Int:
						out << fieldStorage.GetValue<int32_t>();
						break;
					case DataType::UInt:
						out << fieldStorage.GetValue<uint32_t>();
						break;
					case DataType::Long:
						out << fieldStorage.GetValue<int64_t>();
						break;
					case DataType::ULong:
						out << fieldStorage.GetValue<uint64_t>();
						break;
					case DataType::Float:
						out << fieldStorage.GetValue<float>();
						break;
					case DataType::Double:
						out << fieldStorage.GetValue<double>();
						break;
					case DataType::Bool:
						//out << fieldStorage.GetValue<int16_t>();
						out << fieldStorage.GetValue<bool>();
						break;
					case DataType::String:
						out << fieldStorage.GetValue<Coral::String>();
						break;
					case DataType::Bool32:
						out << fieldStorage.GetValue<bool>();
						break;
					case DataType::AssetHandle:
						out << fieldStorage.GetValue<uint64_t>();
						break;
					case DataType::Vector2:
						out << fieldStorage.GetValue<rtmcpp::Vec2>();
						break;
					case DataType::Vector3:
						out << fieldStorage.GetValue<rtmcpp::Vec3>();
						break;
					case DataType::Vector4:
						out << fieldStorage.GetValue<rtmcpp::Vec4>();
						break;
					//case DataType::Entity:
					//	out << fieldStorage.GetValue<uint64_t>();
					//	break;
					//case DataType::Prefab:
					//	out << fieldStorage.GetValue<uint64_t>();
					//	break;
					//case DataType::Mesh:
					//	out << fieldStorage.GetValue<uint64_t>();
					//	break;
					//case DataType::StaticMesh:
					//	out << fieldStorage.GetValue<uint64_t>();
					//	break;
					//case DataType::Material:
					//	out << fieldStorage.GetValue<uint64_t>();
					//	break;
					//case DataType::Texture2D:
					//	out << fieldStorage.GetValue<uint64_t>();
					//	break;
					//case DataType::Scene:
					//	out << fieldStorage.GetValue<uint64_t>();
					//	break;
					default:
						break;
					}

					out << YAML::EndMap;
				}
				out << YAML::EndSeq;
			}
		//
		//	out << YAML::Key << "ClassName" << YAML::Value << scriptComponent.ClassName;
		//
		//	// Fields
		//	Ref<ScriptClass> entityClass = ScriptEngine::GetEntityClass(scriptComponent.ClassName);
		//	const auto& fields = entityClass->GetFields();
		//	if (fields.size() > 0)
		//	{
		//		out << YAML::Key << "ScriptFields" << YAML::Value;
		//		auto& entityFields = ScriptEngine::GetScriptFieldMap(entity);
		//		out << YAML::BeginSeq;
		//		for (const auto& [name, field] : fields)
		//		{
		//			if (entityFields.find(name) == entityFields.end())
		//				continue;
		//
		//			out << YAML::BeginMap; // ScriptField
		//			out << YAML::Key << "Name" << YAML::Value << name;
		//			out << YAML::Key << "Type" << YAML::Value << Utils::ScriptFieldTypeToString(field.Type);
		//
		//			out << YAML::Key << "Data" << YAML::Value;
		//			ScriptFieldInstance& scriptField = entityFields.at(name);
		//
		//			switch (field.Type)
		//			{
		//			WRITE_SCRIPT_FIELD(Float, float);
		//			WRITE_SCRIPT_FIELD(Double, double);
		//			WRITE_SCRIPT_FIELD(Bool, bool);
		//			WRITE_SCRIPT_FIELD(SByte, int8_t);
		//			WRITE_SCRIPT_FIELD(Short, int16_t);
		//			WRITE_SCRIPT_FIELD(Int, int32_t);
		//			WRITE_SCRIPT_FIELD(Long, int64_t);
		//			WRITE_SCRIPT_FIELD(Byte, uint8_t);
		//			WRITE_SCRIPT_FIELD(UShort, uint16_t);
		//			WRITE_SCRIPT_FIELD(UInt, uint32_t);
		//			WRITE_SCRIPT_FIELD(ULong, uint64_t);
		//			WRITE_SCRIPT_FIELD(String, std::string);
		//			WRITE_SCRIPT_FIELD(Vector2, glm::vec2);
		//			WRITE_SCRIPT_FIELD(Vector3, glm::vec3);
		//			WRITE_SCRIPT_FIELD(Vector4, glm::vec4);
		//			WRITE_SCRIPT_FIELD(AssetHandle, AssetHandle);
		//			WRITE_SCRIPT_FIELD(Entity, UUID);
		//			}
		//			out << YAML::EndMap; // ScriptFields
		//		}
		//		out << YAML::EndSeq;
		//	}
		//
			out << YAML::EndMap; // ScriptComponent
		}

		if (entity.HasComponent<SpriteRendererComponent>())
		{
			out << YAML::Key << "SpriteRendererComponent";
			out << YAML::BeginMap; // SpriteRendererComponent

			auto& spriteRendererComponent = entity.GetComponent<SpriteRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << spriteRendererComponent.Color;
			out << YAML::Key << "UV" << YAML::Value << spriteRendererComponent.UV;
			out << YAML::Key << "TextureHandle" << YAML::Value << spriteRendererComponent.TextureHandle;
			out << YAML::Key << "UseParallaxScrolling" << YAML::Value << spriteRendererComponent.m_AnimationData.UseParallaxScrolling;
			
			if (spriteRendererComponent.m_AnimationData.UseParallaxScrolling)
			{
				out << YAML::Key << "UseCameraParallaxX" << YAML::Value << spriteRendererComponent.m_AnimationData.UseCameraParallaxX;
				out << YAML::Key << "UseCameraParallaxY" << YAML::Value << spriteRendererComponent.m_AnimationData.UseCameraParallaxY;
				out << YAML::Key << "ParallaxSpeed" << YAML::Value << spriteRendererComponent.m_AnimationData.ParallaxSpeed;
				out << YAML::Key << "ParallaxDivision" << YAML::Value << spriteRendererComponent.m_AnimationData.ParallaxDivision;
			}

			out << YAML::Key << "UseTextureAtlasAnimation" << YAML::Value << spriteRendererComponent.m_AnimationData.UseTextureAtlasAnimation;

			if (spriteRendererComponent.m_AnimationData.UseTextureAtlasAnimation)
			{
				out << YAML::Key << "AnimationSpeed" << YAML::Value << spriteRendererComponent.m_AnimationData.AnimationSpeed;
			}

			out << YAML::Key << "NumberOfTiles" << YAML::Value << spriteRendererComponent.m_AnimationData.NumberOfTiles;
			out << YAML::Key << "StartIndexX" << YAML::Value << spriteRendererComponent.m_AnimationData.StartIndexX;
			out << YAML::Key << "StartIndexY" << YAML::Value << spriteRendererComponent.m_AnimationData.StartIndexY;
			out << YAML::Key << "Rows" << YAML::Value << spriteRendererComponent.m_AnimationData.Rows;
			out << YAML::Key << "Columns" << YAML::Value << spriteRendererComponent.m_AnimationData.Columns;
			out << YAML::Key << "UseLinear" << YAML::Value << spriteRendererComponent.m_AnimationData.UseLinear;

			if (spriteRendererComponent.m_AnimationData.UsePerTextureAnimation)
			{
				out << YAML::Key << "UsePerTextureAnimation" << YAML::Value << spriteRendererComponent.m_AnimationData.UsePerTextureAnimation;
				out << YAML::Key << "StartIndex" << YAML::Value << spriteRendererComponent.m_AnimationData.StartIndex;
				out << YAML::Key << "TexturesSize" << YAML::Value << spriteRendererComponent.m_AnimationData.NumberOfTextures;

				for (uint32_t i = 0; i < spriteRendererComponent.m_AnimationData.Textures.size(); i++)
				{
					if (spriteRendererComponent.m_AnimationData.Textures[i])
					{
						std::string texturePath = "TextureHandle" + std::to_string(i);
						out << YAML::Key << texturePath.c_str() << YAML::Value << spriteRendererComponent.m_AnimationData.Textures[i];
					}
				}
			}

			out << YAML::EndMap; // SpriteRendererComponent
		}

		if (entity.HasComponent<CircleRendererComponent>())
		{
			out << YAML::Key << "CircleRendererComponent";
			out << YAML::BeginMap; // CircleRendererComponent

			auto& circleRendererComponent = entity.GetComponent<CircleRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << circleRendererComponent.Color;
			out << YAML::Key << "UV" << YAML::Value << circleRendererComponent.UV;
			out << YAML::Key << "Thickness" << YAML::Value << circleRendererComponent.Thickness;
			out << YAML::Key << "Fade" << YAML::Value << circleRendererComponent.Fade;
			out << YAML::Key << "MakeSolid" << YAML::Value << circleRendererComponent.MakeSolid;
			out << YAML::Key << "TextureHandle" << YAML::Value << circleRendererComponent.TextureHandle;
			out << YAML::Key << "UseParallaxScrolling" << YAML::Value << circleRendererComponent.m_AnimationData.UseParallaxScrolling;

			if (circleRendererComponent.m_AnimationData.UseParallaxScrolling)
			{
				out << YAML::Key << "UseCameraParallaxX" << YAML::Value << circleRendererComponent.m_AnimationData.UseCameraParallaxX;
				out << YAML::Key << "UseCameraParallaxY" << YAML::Value << circleRendererComponent.m_AnimationData.UseCameraParallaxY;
				out << YAML::Key << "ParallaxSpeed" << YAML::Value << circleRendererComponent.m_AnimationData.ParallaxSpeed;
				out << YAML::Key << "ParallaxDivision" << YAML::Value << circleRendererComponent.m_AnimationData.ParallaxDivision;
			}

			out << YAML::Key << "UseTextureAtlasAnimation" << YAML::Value << circleRendererComponent.m_AnimationData.UseTextureAtlasAnimation;

			if (circleRendererComponent.m_AnimationData.UseTextureAtlasAnimation)
			{
				out << YAML::Key << "AnimationSpeed" << YAML::Value << circleRendererComponent.m_AnimationData.AnimationSpeed;
			}

			out << YAML::Key << "NumberOfTiles" << YAML::Value << circleRendererComponent.m_AnimationData.NumberOfTiles;
			out << YAML::Key << "StartIndexX" << YAML::Value << circleRendererComponent.m_AnimationData.StartIndexX;
			out << YAML::Key << "StartIndexY" << YAML::Value << circleRendererComponent.m_AnimationData.StartIndexY;
			out << YAML::Key << "Rows" << YAML::Value << circleRendererComponent.m_AnimationData.Rows;
			out << YAML::Key << "Columns" << YAML::Value << circleRendererComponent.m_AnimationData.Columns;
			out << YAML::Key << "UseLinear" << YAML::Value << circleRendererComponent.m_AnimationData.UseLinear;

			out << YAML::EndMap; // CircleRendererComponent
		}

		if (entity.HasComponent<TriangleRendererComponent>())
		{
			out << YAML::Key << "TriangleRendererComponent";
			out << YAML::BeginMap; // TriangleRendererComponent

			auto& triangleRendererComponent = entity.GetComponent<TriangleRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << triangleRendererComponent.Color;
			out << YAML::Key << "UV" << YAML::Value << triangleRendererComponent.UV;
			out << YAML::Key << "TextureHandle" << YAML::Value << triangleRendererComponent.TextureHandle;
			
			out << YAML::EndMap; // TriangleRendererComponent
		}

		if (entity.HasComponent<LineRendererComponent>())
		{
			out << YAML::Key << "LineRendererComponent";
			out << YAML::BeginMap; // LineRendererComponent

			auto& lineRendererComponent = entity.GetComponent<LineRendererComponent>();
			out << YAML::Key << "LineThickness" << YAML::Value << lineRendererComponent.LineThickness;

			uint32_t numberOfColors = lineRendererComponent.NumberOfColors - 1;
			out << YAML::Key << "NumberOfColors" << YAML::Value << numberOfColors;
			out << YAML::Key << "NumberOfTranslations" << YAML::Value << lineRendererComponent.NumberOfTranslations;

			for (uint32_t i = 0; lineRendererComponent.Translations.size(); i++)
			{
				if (i < lineRendererComponent.Translations.size())
				{
					if (i >= 1)
					{
						std::string keyColorName = "Color" + std::to_string(i);
						out << YAML::Key << keyColorName.c_str() << YAML::Value << lineRendererComponent.Colors[i];
					}

					std::string keyTranslationName = "Translation" + std::to_string(i);
					out << YAML::Key << keyTranslationName.c_str() << YAML::Value << lineRendererComponent.Translations[i];
				}
				else
					break;
			}

			out << YAML::EndMap; // LineRendererComponent
		}

		if (entity.HasComponent<TextComponent>())
		{
			out << YAML::Key << "TextComponent";
			out << YAML::BeginMap; // TextComponent

			auto& textComponent = entity.GetComponent<TextComponent>();
			out << YAML::Key << "FontHandle" << YAML::Value << textComponent.FontHandle;
			out << YAML::Key << "TextString" << YAML::Value << textComponent.TextString;
			out << YAML::Key << "Color" << YAML::Value << textComponent.Color;
			out << YAML::Key << "LinearFiltering" << YAML::Value << textComponent.UseLinear;
			out << YAML::Key << "LineSpacing" << YAML::Value << textComponent.LineSpacing;
			out << YAML::Key << "Kerning" << YAML::Value << textComponent.Kerning;

			out << YAML::EndMap; // TextComponent
		}

		if (entity.HasComponent<ParticleSystemComponent>())
		{
			out << YAML::Key << "ParticleSystemComponent";
			out << YAML::BeginMap; // ParticleSystemComponent

			auto& particleSystemComponent = entity.GetComponent<ParticleSystemComponent>();
			out << YAML::Key << "Velocity" << YAML::Value << particleSystemComponent.Velocity;
			out << YAML::Key << "VelocityVariation" << YAML::Value << particleSystemComponent.VelocityVariation;
			out << YAML::Key << "ColorBegin" << YAML::Value << particleSystemComponent.ColorBegin;
			out << YAML::Key << "ColorEnd" << YAML::Value << particleSystemComponent.ColorEnd;
			out << YAML::Key << "TextureHandle" << YAML::Value << particleSystemComponent.TextureHandle;
			out << YAML::Key << "SizeBegin" << YAML::Value << particleSystemComponent.SizeBegin;
			out << YAML::Key << "SizeEnd" << YAML::Value << particleSystemComponent.SizeEnd;
			out << YAML::Key << "SizeVariation" << YAML::Value << particleSystemComponent.SizeVariation;
			out << YAML::Key << "LifeTime" << YAML::Value << particleSystemComponent.LifeTime;
			out << YAML::Key << "ParticleSize" << YAML::Value << particleSystemComponent.ParticleSize;
			out << YAML::Key << "UseLinear" << YAML::Value << particleSystemComponent.UseLinear;
			out << YAML::Key << "UseBillboard" << YAML::Value << particleSystemComponent.UseBillboard;
			
			out << YAML::EndMap; // ParticleSystemComponent
		}

		if (entity.HasComponent<CubeRendererComponent>())
		{
			out << YAML::Key << "CubeRendererComponent";
			out << YAML::BeginMap; // CubeRendererComponent

			auto& cubeRendererComponent = entity.GetComponent<CubeRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << cubeRendererComponent.Color;
			out << YAML::Key << "UV" << YAML::Value << cubeRendererComponent.UV;
			out << YAML::Key << "TextureHandle" << YAML::Value << cubeRendererComponent.TextureHandle;
			out << YAML::Key << "UseParallaxScrolling" << YAML::Value << cubeRendererComponent.m_AnimationData.UseParallaxScrolling;

			if (cubeRendererComponent.m_AnimationData.UseParallaxScrolling)
			{
				out << YAML::Key << "UseCameraParallaxX" << YAML::Value << cubeRendererComponent.m_AnimationData.UseCameraParallaxX;
				out << YAML::Key << "UseCameraParallaxY" << YAML::Value << cubeRendererComponent.m_AnimationData.UseCameraParallaxY;
				out << YAML::Key << "ParallaxSpeed" << YAML::Value << cubeRendererComponent.m_AnimationData.ParallaxSpeed;
				out << YAML::Key << "ParallaxDivision" << YAML::Value << cubeRendererComponent.m_AnimationData.ParallaxDivision;
			}

			out << YAML::Key << "UseTextureAtlasAnimation" << YAML::Value << cubeRendererComponent.m_AnimationData.UseTextureAtlasAnimation;

			if (cubeRendererComponent.m_AnimationData.UseTextureAtlasAnimation)
			{
				out << YAML::Key << "AnimationSpeed" << YAML::Value << cubeRendererComponent.m_AnimationData.AnimationSpeed;
			}

			out << YAML::Key << "NumberOfTiles" << YAML::Value << cubeRendererComponent.m_AnimationData.NumberOfTiles;
			out << YAML::Key << "StartIndexX" << YAML::Value << cubeRendererComponent.m_AnimationData.StartIndexX;
			out << YAML::Key << "StartIndexY" << YAML::Value << cubeRendererComponent.m_AnimationData.StartIndexY;
			out << YAML::Key << "Rows" << YAML::Value << cubeRendererComponent.m_AnimationData.Rows;
			out << YAML::Key << "Columns" << YAML::Value << cubeRendererComponent.m_AnimationData.Columns;
			out << YAML::Key << "UseLinear" << YAML::Value << cubeRendererComponent.m_AnimationData.UseLinear;

			if (cubeRendererComponent.m_AnimationData.UsePerTextureAnimation)
			{
				out << YAML::Key << "UsePerTextureAnimation" << YAML::Value << cubeRendererComponent.m_AnimationData.UsePerTextureAnimation;
				out << YAML::Key << "StartIndex" << YAML::Value << cubeRendererComponent.m_AnimationData.StartIndex;
				out << YAML::Key << "TexturesSize" << YAML::Value << cubeRendererComponent.m_AnimationData.NumberOfTextures;

				for (uint32_t i = 0; i < cubeRendererComponent.m_AnimationData.Textures.size(); i++)
				{
					if (cubeRendererComponent.m_AnimationData.Textures[i])
					{
						std::string texturePath = "TextureHandle" + std::to_string(i);
						out << YAML::Key << texturePath.c_str() << YAML::Value << cubeRendererComponent.m_AnimationData.Textures[i];
					}
				}
			}

			out << YAML::EndMap; // CubeRendererComponent
		}

		if (entity.HasComponent<PyramidRendererComponent>())
		{
			out << YAML::Key << "PyramidRendererComponent";
			out << YAML::BeginMap; // PyramidRendererComponent

			auto& pyramidRendererComponent = entity.GetComponent<PyramidRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << pyramidRendererComponent.Color;
			out << YAML::Key << "UV" << YAML::Value << pyramidRendererComponent.UV;
			out << YAML::Key << "TextureHandle" << YAML::Value << pyramidRendererComponent.TextureHandle;

			out << YAML::EndMap; // PyramidRendererComponent
		}

		if (entity.HasComponent<TriangularPrismRendererComponent>())
		{
			out << YAML::Key << "TriangularPrismRendererComponent";
			out << YAML::BeginMap; // TriangularPrismRendererComponent

			auto& triangularPrismRendererComponent = entity.GetComponent<TriangularPrismRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << triangularPrismRendererComponent.Color;
			out << YAML::Key << "UV" << YAML::Value << triangularPrismRendererComponent.UV;
			out << YAML::Key << "TexturePath" << YAML::Value << triangularPrismRendererComponent.TextureHandle;

			out << YAML::EndMap; // TriangularPrismRendererComponent
		}

		if (entity.HasComponent<PlaneRendererComponent>())
		{
			out << YAML::Key << "PlaneRendererComponent";
			out << YAML::BeginMap; // PlaneRendererComponent

			auto& planeRendererComponent = entity.GetComponent<PlaneRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << planeRendererComponent.Color;
			out << YAML::Key << "UV" << YAML::Value << planeRendererComponent.UV;
			out << YAML::Key << "TexturePath" << YAML::Value << planeRendererComponent.TextureHandle;

			out << YAML::EndMap; // PlaneRendererComponent
		}

		if (entity.HasComponent<OBJRendererComponent>())
		{
			out << YAML::Key << "OBJRendererComponent";
			out << YAML::BeginMap; // OBJRendererComponent

			auto& objRendererComponent = entity.GetComponent<OBJRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << objRendererComponent.Color;
			out << YAML::Key << "UV" << YAML::Value << objRendererComponent.UV;
			out << YAML::Key << "ModelHandle" << YAML::Value << objRendererComponent.ModelHandle;
			//if (objRendererComponent.ModelHandle.Texture)
			//{
			//	if (!objRendererComponent.Texture->GetPath().empty())
			//		out << YAML::Key << "TexturePath" << YAML::Value << objRendererComponent.Texture->GetPath();
			//}

			out << YAML::EndMap; // OBJRendererComponent
		}

		if (entity.HasComponent<ButtonWidgetComponent>())
		{
			out << YAML::Key << "ButtonWidgetComponent";
			out << YAML::BeginMap; // ButtonWidgetComponent

			auto& buttonWidgetComponent = entity.GetComponent<ButtonWidgetComponent>();
			out << YAML::Key << "Color" << YAML::Value << buttonWidgetComponent.Color;
			out << YAML::Key << "UV" << YAML::Value << buttonWidgetComponent.UV;
			out << YAML::Key << "TextureHandle" << YAML::Value << buttonWidgetComponent.TextureHandle;

			out << YAML::Key << "Radius" << YAML::Value << buttonWidgetComponent.Radius;
			out << YAML::Key << "Dimensions" << YAML::Value << buttonWidgetComponent.Dimensions;
			out << YAML::Key << "InvertCorners" << YAML::Value << buttonWidgetComponent.InvertCorners;

			out << YAML::Key << "UseLinear" << YAML::Value << buttonWidgetComponent.UseLinear;

			out << YAML::EndMap; // ButtonWidgetComponent
		}

		if (entity.HasComponent<CircleWidgetComponent>())
		{
			out << YAML::Key << "CircleWidgetComponent";
			out << YAML::BeginMap; // CircleWidgetComponent

			auto& circleWidgetComponent = entity.GetComponent<CircleWidgetComponent>();
			out << YAML::Key << "Color" << YAML::Value << circleWidgetComponent.Color;
			out << YAML::Key << "UV" << YAML::Value << circleWidgetComponent.UV;
			out << YAML::Key << "TextureHandle" << YAML::Value << circleWidgetComponent.TextureHandle;

			out << YAML::Key << "Fade" << YAML::Value << circleWidgetComponent.Fade;
			out << YAML::Key << "UseLinear" << YAML::Value << circleWidgetComponent.UseLinear;

			out << YAML::EndMap; // CircleWidgetComponent
		}

		if (entity.HasComponent<Rigidbody2DComponent>())
		{
			out << YAML::Key << "RigidBody2DComponent";
			out << YAML::BeginMap; // RigidBody2DComponent

			auto& rigidbody2DComponent = entity.GetComponent<Rigidbody2DComponent>();
			out << YAML::Key << "BodyType" << YAML::Value << (int)rigidbody2DComponent.Type;
			out << YAML::Key << "FixedRotation" << YAML::Value << rigidbody2DComponent.FixedRotation;
			out << YAML::Key << "SetEnabled" << YAML::Value << rigidbody2DComponent.SetEnabled;

			out << YAML::EndMap; // RigidBody2DComponent
		}

		if (entity.HasComponent<BoxCollider2DComponent>())
		{
			out << YAML::Key << "BoxCollider2DComponent";
			out << YAML::BeginMap; // BoxCollider2DComponent

			auto& boxCollider2DComponent = entity.GetComponent<BoxCollider2DComponent>();
			out << YAML::Key << "Offset" << YAML::Value << boxCollider2DComponent.Offset;
			out << YAML::Key << "Size" << YAML::Value << boxCollider2DComponent.Size;
			out << YAML::Key << "Density" << YAML::Value << boxCollider2DComponent.Density;
			out << YAML::Key << "Friction" << YAML::Value << boxCollider2DComponent.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << boxCollider2DComponent.Restitution;
			out << YAML::Key << "Threshold" << YAML::Value << boxCollider2DComponent.RestitutionThreshold;

			out << YAML::EndMap; // BoxCollider2DComponent
		}

		if (entity.HasComponent<CircleCollider2DComponent>())
		{
			out << YAML::Key << "CircleCollider2DComponent";
			out << YAML::BeginMap; // CircleCollider2DComponent

			auto& circleCollider2DComponent = entity.GetComponent<CircleCollider2DComponent>();
			out << YAML::Key << "Offset" << YAML::Value << circleCollider2DComponent.Offset;
			out << YAML::Key << "Radius" << YAML::Value << circleCollider2DComponent.Radius;
			out << YAML::Key << "Density" << YAML::Value << circleCollider2DComponent.Density;
			out << YAML::Key << "Friction" << YAML::Value << circleCollider2DComponent.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << circleCollider2DComponent.Restitution;
			out << YAML::Key << "Threshold" << YAML::Value << circleCollider2DComponent.RestitutionThreshold;

			out << YAML::EndMap; // CircleCollider2DComponent
		}

		if (entity.HasComponent<TriangleCollider2DComponent>())
		{
			out << YAML::Key << "TriangleCollider2DComponent";
			out << YAML::BeginMap; // TriangleCollider2DComponent

			auto& triangleCollider2DComponent = entity.GetComponent<TriangleCollider2DComponent>();
			out << YAML::Key << "Offset" << YAML::Value << triangleCollider2DComponent.Offset;
			out << YAML::Key << "Size" << YAML::Value << triangleCollider2DComponent.Size;
			out << YAML::Key << "Density" << YAML::Value << triangleCollider2DComponent.Density;
			out << YAML::Key << "Friction" << YAML::Value << triangleCollider2DComponent.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << triangleCollider2DComponent.Restitution;
			out << YAML::Key << "Threshold" << YAML::Value << triangleCollider2DComponent.RestitutionThreshold;

			out << YAML::EndMap; // TriangleCollider2DComponent
		}

		if (entity.HasComponent<CapsuleCollider2DComponent>())
		{
			out << YAML::Key << "CapsuleCollider2DComponent";
			out << YAML::BeginMap; // CapsuleCollider2DComponent

			auto& capsuleCollider2DComponent = entity.GetComponent<CapsuleCollider2DComponent>();
			out << YAML::Key << "Offset" << YAML::Value << capsuleCollider2DComponent.Offset;
			out << YAML::Key << "Size" << YAML::Value << capsuleCollider2DComponent.Size;
			out << YAML::Key << "Density" << YAML::Value << capsuleCollider2DComponent.Density;
			out << YAML::Key << "Friction" << YAML::Value << capsuleCollider2DComponent.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << capsuleCollider2DComponent.Restitution;
			out << YAML::Key << "Threshold" << YAML::Value << capsuleCollider2DComponent.RestitutionThreshold;

			out << YAML::EndMap; // CapsuleCollider2DComponent
		}

		if (entity.HasComponent<MeshCollider2DComponent>())
		{
			out << YAML::Key << "MeshCollider2DComponent";
			out << YAML::BeginMap; // MeshCollider2DComponent

			auto& meshCollider2DComponent = entity.GetComponent<MeshCollider2DComponent>();
			out << YAML::Key << "Offset" << YAML::Value << meshCollider2DComponent.Offset;
			out << YAML::Key << "Size" << YAML::Value << meshCollider2DComponent.Size;
			out << YAML::Key << "Density" << YAML::Value << meshCollider2DComponent.Density;
			out << YAML::Key << "Friction" << YAML::Value << meshCollider2DComponent.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << meshCollider2DComponent.Restitution;
			out << YAML::Key << "Threshold" << YAML::Value << meshCollider2DComponent.RestitutionThreshold;
			out << YAML::Key << "NumberOfPositions" << YAML::Value << meshCollider2DComponent.NumberOfPositions;

			if (meshCollider2DComponent.Positions.size() > 0)
			{
				for (uint32_t i = 0; i < meshCollider2DComponent.Positions.size(); i++)
				{
					std::string positionName = "Position" + std::to_string(i);
					out << YAML::Key << positionName.c_str() << YAML::Value << meshCollider2DComponent.Positions[i];
				}
			}

			out << YAML::EndMap; // MeshCollider2DComponent
		}
		
		if (entity.HasComponent<AudioSourceComponent>())
		{
			out << YAML::Key << "AudioSourceComponent";
			out << YAML::BeginMap;

			const auto& audioSourceComponent = entity.GetComponent<AudioSourceComponent>();
			out << YAML::Key << "AudioHandle" << YAML::Value << audioSourceComponent.Audio;
			out << YAML::Key << "VolumeMultiplier" << YAML::Value << audioSourceComponent.Config.VolumeMultiplier;
			out << YAML::Key << "PitchMultiplier" << YAML::Value << audioSourceComponent.Config.PitchMultiplier;
			out << YAML::Key << "PlayOnAwake" << YAML::Value << audioSourceComponent.Config.PlayOnAwake;
			out << YAML::Key << "Looping" << YAML::Value << audioSourceComponent.Config.Looping;
			out << YAML::Key << "Spatialization" << YAML::Value << audioSourceComponent.Config.Spatialization;
			out << YAML::Key << "AttenuationModel" << YAML::Value << static_cast<int>(audioSourceComponent.Config.AttenuationModel);
			out << YAML::Key << "RollOff" << YAML::Value << audioSourceComponent.Config.RollOff;
			out << YAML::Key << "MinGain" << YAML::Value << audioSourceComponent.Config.MinGain;
			out << YAML::Key << "MaxGain" << YAML::Value << audioSourceComponent.Config.MaxGain;
			out << YAML::Key << "MinDistance" << YAML::Value << audioSourceComponent.Config.MinDistance;
			out << YAML::Key << "MaxDistance" << YAML::Value << audioSourceComponent.Config.MaxDistance;
			out << YAML::Key << "ConeInnerAngle" << YAML::Value << audioSourceComponent.Config.ConeInnerAngle;
			out << YAML::Key << "ConeOuterAngle" << YAML::Value << audioSourceComponent.Config.ConeOuterAngle;
			out << YAML::Key << "ConeOuterGain" << YAML::Value << audioSourceComponent.Config.ConeOuterGain;
			out << YAML::Key << "DopplerFactor" << YAML::Value << audioSourceComponent.Config.DopplerFactor;

			out << YAML::Key << "UsePlaylist" << YAML::Value << audioSourceComponent.AudioSourceData.UsePlaylist;

			if (audioSourceComponent.AudioSourceData.UsePlaylist)
			{
				out << YAML::Key << "AudioSourcesSize" << YAML::Value << audioSourceComponent.AudioSourceData.NumberOfAudioSources;
				out << YAML::Key << "StartIndex" << YAML::Value << audioSourceComponent.AudioSourceData.StartIndex;
				out << YAML::Key << "RepeatPlaylist" << YAML::Value << audioSourceComponent.AudioSourceData.RepeatPlaylist;
				out << YAML::Key << "RepeatSpecificTrack" << YAML::Value << audioSourceComponent.AudioSourceData.RepeatAfterSpecificTrackPlays;

				for (uint32_t i = 0; i < audioSourceComponent.AudioSourceData.Playlist.size(); i++)
				{
					if (audioSourceComponent.AudioSourceData.Playlist[i])
					{
						std::string audioName = "AudioHandle" + std::to_string(i);
						out << YAML::Key << audioName.c_str() << YAML::Value << audioSourceComponent.AudioSourceData.Playlist[i];
					}
				}
			}

			out << YAML::EndMap;
		}

		if (entity.HasComponent<AudioListenerComponent>())
		{
			out << YAML::Key << "AudioListenerComponent";
			out << YAML::BeginMap;

			const auto& audioListenerComponent = entity.GetComponent<AudioListenerComponent>();
			out << YAML::Key << "Active" << YAML::Value << audioListenerComponent.Active;
			out << YAML::Key << "ConeInnerAngle" << YAML::Value << audioListenerComponent.Config.ConeInnerAngle;
			out << YAML::Key << "ConeOuterAngle" << YAML::Value << audioListenerComponent.Config.ConeOuterAngle;
			out << YAML::Key << "ConeOuterGain" << YAML::Value << audioListenerComponent.Config.ConeOuterGain;

			out << YAML::EndMap;
		}
 
		if (entity.HasComponent<VideoRendererComponent>())
		{
			out << YAML::Key << "VideoRendererComponent";
			out << YAML::BeginMap; // VideoRendererComponent

			auto& videoRendererComponent = entity.GetComponent<VideoRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << videoRendererComponent.Color;
			out << YAML::Key << "RepeatVideo" << YAML::Value << videoRendererComponent.m_VideoData.RepeatVideo;
			out << YAML::Key << "VideoHandle" << YAML::Value << videoRendererComponent.Video;
			out << YAML::Key << "UseExternalAudio" << YAML::Value << videoRendererComponent.m_VideoData.UseExternalAudio;
			
			if (videoRendererComponent.Video)
				out << YAML::Key << "Volume" << YAML::Value << videoRendererComponent.m_VideoData.Volume;

			out << YAML::EndMap; // VideoRendererComponent
		}

		out << YAML::EndMap; // Entity
	}

	void SceneSerializer::SerializeEntityJSON(yyjson_mut_doc* doc, yyjson_mut_val* root, Entity entity, RefPtr<Scene> scene)
	{
		yyjson_mut_obj_add_str(doc, root, "Component", "ScriptComponent");
		const auto& scriptEngine = ScriptEngine::GetInstance();
		const auto& sc = entity.GetComponent<ScriptComponent>();

		if (scriptEngine.IsValidScript(sc.ScriptHandle))
		{
			const auto& scriptMetadata = scriptEngine.GetScriptMetadata(sc.ScriptHandle);
			const auto& entityStorage = scene->m_ScriptStorage.EntityStorage.at(entity.GetEntityHandle());

			yyjson_mut_obj_add_uint(doc, root, "ScriptHandle", (AssetHandle)sc.ScriptHandle);
			yyjson_mut_obj_add_str(doc, root, "ScriptName", scriptMetadata.FullName.c_str());

			yyjson_mut_obj_add_uint(doc, root, "Fields", entityStorage.Fields.size());

			if (entityStorage.Fields.size() > 0)
			{
				for (const auto& [fieldID, fieldStorage] : entityStorage.Fields)
				{
					const auto& fieldMetadata = scriptMetadata.Fields.at(fieldID);

					yyjson_mut_obj_add_uint(doc, root, "ID", fieldID); // This might cause a bug, when serializing a scene after saving scene, closing scene and reopen scene!
					yyjson_mut_obj_add_str(doc, root, "Name", fieldMetadata.Name.c_str());

					std::string type = GetDataTypeName(fieldMetadata.Type) + '\0';
					char* endType = new char[type.length() + 1];
					std::copy(type.begin(), type.end(), endType);

					yyjson_mut_obj_add_str(doc, root, "Type", endType);

					switch (fieldMetadata.Type)
					{
					case DataType::SByte:
						yyjson_mut_obj_add_int(doc, root, "Value", fieldStorage.GetValue<int8_t>());
						break;
					case DataType::Byte:
						yyjson_mut_obj_add_uint(doc, root, "Value", fieldStorage.GetValue<uint8_t>());
						break;
					case DataType::Short:
						yyjson_mut_obj_add_int(doc, root, "Value", fieldStorage.GetValue<int16_t>());
						break;
					case DataType::UShort:
						yyjson_mut_obj_add_uint(doc, root, "Value", fieldStorage.GetValue<uint16_t>());
						break;
					case DataType::Int:
						yyjson_mut_obj_add_int(doc, root, "Value", fieldStorage.GetValue<int32_t>());
						break;
					case DataType::UInt:
						yyjson_mut_obj_add_uint(doc, root, "Value", fieldStorage.GetValue<uint32_t>());
						break;
					case DataType::Long:
						yyjson_mut_obj_add_int(doc, root, "Value", fieldStorage.GetValue<int64_t>());
						break;
					case DataType::ULong:
						yyjson_mut_obj_add_uint(doc, root, "Value", fieldStorage.GetValue<uint64_t>());
						break;
					case DataType::Float:
						yyjson_mut_obj_add_real(doc, root, "Value", (double)fieldStorage.GetValue<float>());
						break;
					case DataType::Double:
						yyjson_mut_obj_add_real(doc, root, "Value", fieldStorage.GetValue<double>());
						break;
					case DataType::Bool:
						yyjson_mut_obj_add_bool(doc, root, "Value", fieldStorage.GetValue<bool>());
						break;
					case DataType::String:
					{
						char* strData;
						std::string str = fieldStorage.GetValue<Coral::String>();
						strData = (char*)malloc(str.size() + 1);
						strcpy(strData, str.c_str());

						const char* result = strData;
						NZ_CORE_WARN("Script string is: {0}", result);
						yyjson_mut_obj_add_str(doc, root, "Value", result);
						break;
					}
					case DataType::Bool32:
						yyjson_mut_obj_add_bool(doc, root, "Value", fieldStorage.GetValue<bool>());
						break;
					case DataType::AssetHandle:
						yyjson_mut_obj_add_uint(doc, root, "Value", fieldStorage.GetValue<AssetHandle>());
						break;
					case DataType::Vector2:
					{
						rtmcpp::Vec2 value = fieldStorage.GetValue<rtmcpp::Vec2>();
						yyjson_mut_obj_add_real(doc, root, "ValueX", value.X);
						yyjson_mut_obj_add_real(doc, root, "ValueY", value.Y);
						break;
					}
					case DataType::Vector3:
					{
						rtmcpp::Vec3 value = fieldStorage.GetValue<rtmcpp::Vec3>();
						yyjson_mut_obj_add_real(doc, root, "ValueX", value.X);
						yyjson_mut_obj_add_real(doc, root, "ValueY", value.Y);
						yyjson_mut_obj_add_real(doc, root, "ValueZ", value.Z);
						break;
					}
					case DataType::Vector4:
					{
						rtmcpp::Vec4 value = fieldStorage.GetValue<rtmcpp::Vec4>();
						yyjson_mut_obj_add_real(doc, root, "ValueX", value.X);
						yyjson_mut_obj_add_real(doc, root, "ValueY", value.Y);
						yyjson_mut_obj_add_real(doc, root, "ValueZ", value.Z);
						yyjson_mut_obj_add_real(doc, root, "ValueW", value.W);
						break;
					}
					//case DataType::Entity:
					//	out << fieldStorage.GetValue<uint64_t>();
					//	break;
					//case DataType::Prefab:
					//	out << fieldStorage.GetValue<uint64_t>();
					//	break;
					//case DataType::Mesh:
					//	out << fieldStorage.GetValue<uint64_t>();
					//	break;
					//case DataType::StaticMesh:
					//	out << fieldStorage.GetValue<uint64_t>();
					//	break;
					//case DataType::Material:
					//	out << fieldStorage.GetValue<uint64_t>();
					//	break;
					//case DataType::Texture2D:
					//	out << fieldStorage.GetValue<uint64_t>();
					//	break;
					//case DataType::Scene:
					//	out << fieldStorage.GetValue<uint64_t>();
					//	break;
					default:
						break;
					}
				}
			}
		}

		if (entity.HasComponent<SpriteRendererComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "SpriteRendererComponent");
			auto& spriteRendererComponent = entity.GetComponent<SpriteRendererComponent>();

			yyjson_mut_obj_add_real(doc, root, "ColorValueX", spriteRendererComponent.Color.X);
			yyjson_mut_obj_add_real(doc, root, "ColorValueY", spriteRendererComponent.Color.Y);
			yyjson_mut_obj_add_real(doc, root, "ColorValueZ", spriteRendererComponent.Color.Z);
			yyjson_mut_obj_add_real(doc, root, "ColorValueW", spriteRendererComponent.Color.W);

			yyjson_mut_obj_add_real(doc, root, "UVValueX", spriteRendererComponent.UV.X);
			yyjson_mut_obj_add_real(doc, root, "UVValueY", spriteRendererComponent.UV.Y);

			yyjson_mut_obj_add_uint(doc, root, "TextureHandle", (uint64_t)spriteRendererComponent.TextureHandle);
			yyjson_mut_obj_add_bool(doc, root, "UseParallaxScrolling", spriteRendererComponent.m_AnimationData.UseParallaxScrolling);

			if (spriteRendererComponent.m_AnimationData.UseParallaxScrolling)
			{
				yyjson_mut_obj_add_bool(doc, root, "UseCameraParallaxX", spriteRendererComponent.m_AnimationData.UseCameraParallaxX);
				yyjson_mut_obj_add_bool(doc, root, "UseCameraParallaxY", spriteRendererComponent.m_AnimationData.UseCameraParallaxY);

				yyjson_mut_obj_add_real(doc, root, "ParallaxSpeedX", spriteRendererComponent.m_AnimationData.ParallaxSpeed.X);
				yyjson_mut_obj_add_real(doc, root, "ParallaxSpeedY", spriteRendererComponent.m_AnimationData.ParallaxSpeed.Y);

				yyjson_mut_obj_add_real(doc, root, "ParallaxDivision", spriteRendererComponent.m_AnimationData.ParallaxDivision);
			}

			yyjson_mut_obj_add_bool(doc, root, "UseTextureAtlasAnimation", spriteRendererComponent.m_AnimationData.UseTextureAtlasAnimation);

			if (spriteRendererComponent.m_AnimationData.UseTextureAtlasAnimation)
			{
				yyjson_mut_obj_add_real(doc, root, "AnimationSpeed", spriteRendererComponent.m_AnimationData.AnimationSpeed);
			}

			yyjson_mut_obj_add_int(doc, root, "NumberOfTiles", spriteRendererComponent.m_AnimationData.NumberOfTiles);
			yyjson_mut_obj_add_int(doc, root, "StartIndexesX", spriteRendererComponent.m_AnimationData.StartIndexX);
			yyjson_mut_obj_add_int(doc, root, "StartIndexesY", spriteRendererComponent.m_AnimationData.StartIndexY);
			yyjson_mut_obj_add_int(doc, root, "RowsAndColumnsX", spriteRendererComponent.m_AnimationData.Rows);
			yyjson_mut_obj_add_int(doc, root, "RowsAndColumnsY", spriteRendererComponent.m_AnimationData.Columns);
			yyjson_mut_obj_add_bool(doc, root, "UseLinear", spriteRendererComponent.m_AnimationData.UseLinear);
			yyjson_mut_obj_add_bool(doc, root, "UsePerTextureAnimation", spriteRendererComponent.m_AnimationData.UsePerTextureAnimation);

			if (spriteRendererComponent.m_AnimationData.UsePerTextureAnimation)
			{
				yyjson_mut_obj_add_int(doc, root, "StartIndex", spriteRendererComponent.m_AnimationData.StartIndex);
				yyjson_mut_obj_add_uint(doc, root, "TexturesSize", spriteRendererComponent.m_AnimationData.NumberOfTextures);

				for (uint32_t i = 0; i < spriteRendererComponent.m_AnimationData.Textures.size(); i++)
				{
					if (spriteRendererComponent.m_AnimationData.Textures[i])
					{
						std::string handle = "TextureHandle" + std::to_string(i) + '\0';
						char* endHandle = new char[handle.length() + 1];
						std::copy(handle.begin(), handle.end(), endHandle);

						yyjson_mut_obj_add_uint(doc, root, endHandle, (uint64_t)spriteRendererComponent.m_AnimationData.Textures[i]);
					}
				}
			}
		}

		if (entity.HasComponent<CircleRendererComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "CircleRendererComponent");
			auto& circleRendererComponent = entity.GetComponent<CircleRendererComponent>();

			yyjson_mut_obj_add_real(doc, root, "ColorValueX", circleRendererComponent.Color.X);
			yyjson_mut_obj_add_real(doc, root, "ColorValueY", circleRendererComponent.Color.Y);
			yyjson_mut_obj_add_real(doc, root, "ColorValueZ", circleRendererComponent.Color.Z);
			yyjson_mut_obj_add_real(doc, root, "ColorValueW", circleRendererComponent.Color.W);

			yyjson_mut_obj_add_real(doc, root, "UVValueX", circleRendererComponent.UV.X);
			yyjson_mut_obj_add_real(doc, root, "UVValueY", circleRendererComponent.UV.Y);

			yyjson_mut_obj_add_real(doc, root, "Thickness", circleRendererComponent.Thickness);
			yyjson_mut_obj_add_real(doc, root, "Fade", circleRendererComponent.Fade);
			yyjson_mut_obj_add_bool(doc, root, "MakeSolid", circleRendererComponent.MakeSolid);
			yyjson_mut_obj_add_uint(doc, root, "TextureHandle", circleRendererComponent.TextureHandle);
			yyjson_mut_obj_add_bool(doc, root, "UseParallaxScrolling", circleRendererComponent.m_AnimationData.UseParallaxScrolling);

			if (circleRendererComponent.m_AnimationData.UseParallaxScrolling)
			{
				yyjson_mut_obj_add_bool(doc, root, "UseCameraParallaxX", circleRendererComponent.m_AnimationData.UseCameraParallaxX);
				yyjson_mut_obj_add_bool(doc, root, "UseCameraParallaxY", circleRendererComponent.m_AnimationData.UseCameraParallaxY);
				yyjson_mut_obj_add_real(doc, root, "ParallaxSpeedX", circleRendererComponent.m_AnimationData.ParallaxSpeed.X);
				yyjson_mut_obj_add_real(doc, root, "ParallaxSpeedY", circleRendererComponent.m_AnimationData.ParallaxSpeed.Y);
				yyjson_mut_obj_add_real(doc, root, "ParallaxDivision", circleRendererComponent.m_AnimationData.ParallaxDivision);
			}

			yyjson_mut_obj_add_bool(doc, root, "UseTextureAtlasAnimation", circleRendererComponent.m_AnimationData.UseTextureAtlasAnimation);

			if (circleRendererComponent.m_AnimationData.UseTextureAtlasAnimation)
			{
				yyjson_mut_obj_add_real(doc, root, "AnimationSpeed", circleRendererComponent.m_AnimationData.AnimationSpeed);
			}
			
			yyjson_mut_obj_add_int(doc, root, "NumberOfTiles", circleRendererComponent.m_AnimationData.NumberOfTiles);
			yyjson_mut_obj_add_int(doc, root, "StartIndexesX", circleRendererComponent.m_AnimationData.StartIndexX);
			yyjson_mut_obj_add_int(doc, root, "StartIndexesY", circleRendererComponent.m_AnimationData.StartIndexY);
			yyjson_mut_obj_add_int(doc, root, "RowsAndColumnsX", circleRendererComponent.m_AnimationData.Rows);
			yyjson_mut_obj_add_int(doc, root, "RowsAndColumnsY", circleRendererComponent.m_AnimationData.Columns);
			yyjson_mut_obj_add_bool(doc, root, "UseLinear", circleRendererComponent.m_AnimationData.UseLinear);
		}

		if (entity.HasComponent<TriangleRendererComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "TriangleRendererComponent");
			auto& triangleRendererComponent = entity.GetComponent<TriangleRendererComponent>();

			yyjson_mut_obj_add_real(doc, root, "ColorValueX", triangleRendererComponent.Color.X);
			yyjson_mut_obj_add_real(doc, root, "ColorValueY", triangleRendererComponent.Color.Y);
			yyjson_mut_obj_add_real(doc, root, "ColorValueZ", triangleRendererComponent.Color.Z);
			yyjson_mut_obj_add_real(doc, root, "ColorValueW", triangleRendererComponent.Color.W);

			yyjson_mut_obj_add_real(doc, root, "UVValueX", triangleRendererComponent.UV.X);
			yyjson_mut_obj_add_real(doc, root, "UVValueY", triangleRendererComponent.UV.Y);
			yyjson_mut_obj_add_uint(doc, root, "TextureHandle", triangleRendererComponent.TextureHandle);
		}

		if (entity.HasComponent<LineRendererComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "LineRendererComponent");
			auto& lineRendererComponent = entity.GetComponent<LineRendererComponent>();

			yyjson_mut_obj_add_real(doc, root, "LineThickness", lineRendererComponent.LineThickness);

			uint32_t numberOfColors = lineRendererComponent.NumberOfColors - 1;
			yyjson_mut_obj_add_uint(doc, root, "NumberOfColors", numberOfColors);
			yyjson_mut_obj_add_uint(doc, root, "NumberOfTranslations", lineRendererComponent.NumberOfTranslations);

			for (uint32_t i = 0; lineRendererComponent.Translations.size(); i++)
			{
				if (i < lineRendererComponent.Translations.size())
				{
					if (i >= 1)
					{
						std::string handle1 = "ColorValueX" + std::to_string(i) + '\0';
						char* endHandle1 = new char[handle1.length() + 1];
						std::copy(handle1.begin(), handle1.end(), endHandle1);
						yyjson_mut_obj_add_real(doc, root, endHandle1, lineRendererComponent.Colors[i].X);

						std::string handle2 = "ColorValueY" + std::to_string(i) + '\0';
						char* endHandle2 = new char[handle2.length() + 1];
						std::copy(handle2.begin(), handle2.end(), endHandle2);
						yyjson_mut_obj_add_real(doc, root, endHandle2, lineRendererComponent.Colors[i].Y);

						std::string handle3 = "ColorValueZ" + std::to_string(i) + '\0';
						char* endHandle3 = new char[handle3.length() + 1];
						std::copy(handle3.begin(), handle3.end(), endHandle3);
						yyjson_mut_obj_add_real(doc, root, endHandle3, lineRendererComponent.Colors[i].Z);

						std::string handle4 = "ColorValueW" + std::to_string(i) + '\0';
						char* endHandle4 = new char[handle4.length() + 1];
						std::copy(handle4.begin(), handle4.end(), endHandle4);
						yyjson_mut_obj_add_real(doc, root, endHandle4, lineRendererComponent.Colors[i].W);
					}

					std::string handle1 = "TranslationX" + std::to_string(i) + '\0';
					char* endHandle1 = new char[handle1.length() + 1];
					std::copy(handle1.begin(), handle1.end(), endHandle1);
					yyjson_mut_obj_add_real(doc, root, endHandle1, lineRendererComponent.Translations[i].X);

					std::string handle2 = "TranslationY" + std::to_string(i) + '\0';
					char* endHandle2 = new char[handle2.length() + 1];
					std::copy(handle2.begin(), handle2.end(), endHandle2);
					yyjson_mut_obj_add_real(doc, root, endHandle2, lineRendererComponent.Translations[i].Y);

					std::string handle3 = "TranslationZ" + std::to_string(i) + '\0';
					char* endHandle3 = new char[handle3.length() + 1];
					std::copy(handle3.begin(), handle3.end(), endHandle3);
					yyjson_mut_obj_add_real(doc, root, endHandle3, lineRendererComponent.Translations[i].Z);
				}
				else
					break;
			}
		}

		if (entity.HasComponent<TextComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "TextComponent");
			auto& textComponent = entity.GetComponent<TextComponent>();

			yyjson_mut_obj_add_uint(doc, root, "FontHandle", textComponent.FontHandle);
			yyjson_mut_obj_add_str(doc, root, "TextString", textComponent.TextString.c_str());

			yyjson_mut_obj_add_real(doc, root, "ColorValueX", textComponent.Color.X);
			yyjson_mut_obj_add_real(doc, root, "ColorValueY", textComponent.Color.Y);
			yyjson_mut_obj_add_real(doc, root, "ColorValueZ", textComponent.Color.Z);
			yyjson_mut_obj_add_real(doc, root, "ColorValueW", textComponent.Color.W);

			yyjson_mut_obj_add_bool(doc, root, "LinearFiltering", textComponent.UseLinear);
			yyjson_mut_obj_add_real(doc, root, "LineSpacing", textComponent.LineSpacing);
			yyjson_mut_obj_add_real(doc, root, "Kerning", textComponent.Kerning);
		}

		if (entity.HasComponent<ParticleSystemComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "ParticleSystemComponent");
			auto& particleSystemComponent = entity.GetComponent<ParticleSystemComponent>();

			yyjson_mut_obj_add_real(doc, root, "VelocityX", particleSystemComponent.Velocity.X);
			yyjson_mut_obj_add_real(doc, root, "VelocityY", particleSystemComponent.Velocity.Y);
			yyjson_mut_obj_add_real(doc, root, "VelocityZ", particleSystemComponent.Velocity.Z);
			yyjson_mut_obj_add_real(doc, root, "VelocityVariationX", particleSystemComponent.VelocityVariation.X);
			yyjson_mut_obj_add_real(doc, root, "VelocityVariationY", particleSystemComponent.VelocityVariation.Y);
			yyjson_mut_obj_add_real(doc, root, "VelocityVariationZ", particleSystemComponent.VelocityVariation.Z);
			yyjson_mut_obj_add_real(doc, root, "ColorBeginX", particleSystemComponent.ColorBegin.X);
			yyjson_mut_obj_add_real(doc, root, "ColorBeginY", particleSystemComponent.ColorBegin.Y);
			yyjson_mut_obj_add_real(doc, root, "ColorBeginZ", particleSystemComponent.ColorBegin.Z);
			yyjson_mut_obj_add_real(doc, root, "ColorBeginW", particleSystemComponent.ColorBegin.W);
			yyjson_mut_obj_add_real(doc, root, "ColorEndX", particleSystemComponent.ColorEnd.X);
			yyjson_mut_obj_add_real(doc, root, "ColorEndY", particleSystemComponent.ColorEnd.Y);
			yyjson_mut_obj_add_real(doc, root, "ColorEndZ", particleSystemComponent.ColorEnd.Z);
			yyjson_mut_obj_add_real(doc, root, "ColorEndW", particleSystemComponent.ColorEnd.W);

			yyjson_mut_obj_add_uint(doc, root, "TextureHandle", particleSystemComponent.TextureHandle);

			yyjson_mut_obj_add_real(doc, root, "SizeBegin", particleSystemComponent.SizeBegin);
			yyjson_mut_obj_add_real(doc, root, "SizeEnd", particleSystemComponent.SizeEnd);
			yyjson_mut_obj_add_real(doc, root, "SizeVariation", particleSystemComponent.SizeVariation);
			yyjson_mut_obj_add_real(doc, root, "LifeTime", particleSystemComponent.LifeTime);
			yyjson_mut_obj_add_int(doc, root, "ParticleSize", particleSystemComponent.ParticleSize);
			yyjson_mut_obj_add_bool(doc, root, "UseLinear", particleSystemComponent.UseLinear);
			yyjson_mut_obj_add_bool(doc, root, "UseBillboard", particleSystemComponent.UseBillboard);
		}

		if (entity.HasComponent<CubeRendererComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "CubeRendererComponent");
			auto& cubeRendererComponent = entity.GetComponent<CubeRendererComponent>();

			yyjson_mut_obj_add_real(doc, root, "ColorValueX", cubeRendererComponent.Color.X);
			yyjson_mut_obj_add_real(doc, root, "ColorValueY", cubeRendererComponent.Color.Y);
			yyjson_mut_obj_add_real(doc, root, "ColorValueZ", cubeRendererComponent.Color.Z);
			yyjson_mut_obj_add_real(doc, root, "ColorValueW", cubeRendererComponent.Color.W);

			yyjson_mut_obj_add_real(doc, root, "UVValueX", cubeRendererComponent.UV.X);
			yyjson_mut_obj_add_real(doc, root, "UVValueY", cubeRendererComponent.UV.Y);

			yyjson_mut_obj_add_uint(doc, root, "TextureHandle", cubeRendererComponent.TextureHandle);

			yyjson_mut_obj_add_bool(doc, root, "UseParallaxScrolling", cubeRendererComponent.m_AnimationData.UseParallaxScrolling);

			if (cubeRendererComponent.m_AnimationData.UseParallaxScrolling)
			{
				yyjson_mut_obj_add_bool(doc, root, "UseCameraParallaxX", cubeRendererComponent.m_AnimationData.UseCameraParallaxX);
				yyjson_mut_obj_add_bool(doc, root, "UseCameraParallaxY", cubeRendererComponent.m_AnimationData.UseCameraParallaxY);
				yyjson_mut_obj_add_real(doc, root, "ParallaxSpeedX", cubeRendererComponent.m_AnimationData.ParallaxSpeed.X);
				yyjson_mut_obj_add_real(doc, root, "ParallaxSpeedY", cubeRendererComponent.m_AnimationData.ParallaxSpeed.Y);
				yyjson_mut_obj_add_real(doc, root, "ParallaxDivision", cubeRendererComponent.m_AnimationData.ParallaxDivision);
			}

			yyjson_mut_obj_add_bool(doc, root, "UseTextureAtlasAnimation", cubeRendererComponent.m_AnimationData.UseTextureAtlasAnimation);

			if (cubeRendererComponent.m_AnimationData.UseTextureAtlasAnimation)
			{
				yyjson_mut_obj_add_real(doc, root, "AnimationSpeed", cubeRendererComponent.m_AnimationData.AnimationSpeed);
			}

			yyjson_mut_obj_add_int(doc, root, "NumberOfTiles", cubeRendererComponent.m_AnimationData.NumberOfTiles);
			yyjson_mut_obj_add_int(doc, root, "StartIndexesX", cubeRendererComponent.m_AnimationData.StartIndexX);
			yyjson_mut_obj_add_int(doc, root, "StartIndexesY", cubeRendererComponent.m_AnimationData.StartIndexY);
			yyjson_mut_obj_add_int(doc, root, "RowsAndColumnsX", cubeRendererComponent.m_AnimationData.Rows);
			yyjson_mut_obj_add_int(doc, root, "RowsAndColumnsY", cubeRendererComponent.m_AnimationData.Columns);
			yyjson_mut_obj_add_bool(doc, root, "UseLinear", cubeRendererComponent.m_AnimationData.UseLinear);

			if (cubeRendererComponent.m_AnimationData.UsePerTextureAnimation)
			{
				yyjson_mut_obj_add_int(doc, root, "StartIndex", cubeRendererComponent.m_AnimationData.StartIndex);
				yyjson_mut_obj_add_uint(doc, root, "TexturesSize", cubeRendererComponent.m_AnimationData.NumberOfTextures);

				for (uint32_t i = 0; i < cubeRendererComponent.m_AnimationData.Textures.size(); i++)
				{
					if (cubeRendererComponent.m_AnimationData.Textures[i])
					{
						std::string handle = "TextureHandle" + std::to_string(i) + '\0';
						char* endHandle = new char[handle.length() + 1];
						std::copy(handle.begin(), handle.end(), endHandle);

						yyjson_mut_obj_add_uint(doc, root, endHandle, (uint64_t)cubeRendererComponent.m_AnimationData.Textures[i]);
					}
				}
			}
		}

		if (entity.HasComponent<PyramidRendererComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "PyramidRendererComponent");
			auto& pyramidRendererComponent = entity.GetComponent<PyramidRendererComponent>();

			yyjson_mut_obj_add_real(doc, root, "ColorValueX", pyramidRendererComponent.Color.X);
			yyjson_mut_obj_add_real(doc, root, "ColorValueY", pyramidRendererComponent.Color.Y);
			yyjson_mut_obj_add_real(doc, root, "ColorValueZ", pyramidRendererComponent.Color.Z);
			yyjson_mut_obj_add_real(doc, root, "ColorValueW", pyramidRendererComponent.Color.W);

			yyjson_mut_obj_add_real(doc, root, "UVValueX", pyramidRendererComponent.UV.X);
			yyjson_mut_obj_add_real(doc, root, "UVValueY", pyramidRendererComponent.UV.Y);
			yyjson_mut_obj_add_uint(doc, root, "TextureHandle", pyramidRendererComponent.TextureHandle);
		}

		if (entity.HasComponent<TriangularPrismRendererComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "TriangularPrismRendererComponent");
			auto& triangularPrismRendererComponent = entity.GetComponent<TriangularPrismRendererComponent>();

			yyjson_mut_obj_add_real(doc, root, "ColorValueX", triangularPrismRendererComponent.Color.X);
			yyjson_mut_obj_add_real(doc, root, "ColorValueY", triangularPrismRendererComponent.Color.Y);
			yyjson_mut_obj_add_real(doc, root, "ColorValueZ", triangularPrismRendererComponent.Color.Z);
			yyjson_mut_obj_add_real(doc, root, "ColorValueW", triangularPrismRendererComponent.Color.W);

			yyjson_mut_obj_add_real(doc, root, "UVValueX", triangularPrismRendererComponent.UV.X);
			yyjson_mut_obj_add_real(doc, root, "UVValueY", triangularPrismRendererComponent.UV.Y);
			yyjson_mut_obj_add_uint(doc, root, "TextureHandle", triangularPrismRendererComponent.TextureHandle);
		}

		if (entity.HasComponent<PlaneRendererComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "PlaneRendererComponent");
			auto& planeRendererComponent = entity.GetComponent<PlaneRendererComponent>();

			yyjson_mut_obj_add_real(doc, root, "ColorValueX", planeRendererComponent.Color.X);
			yyjson_mut_obj_add_real(doc, root, "ColorValueY", planeRendererComponent.Color.Y);
			yyjson_mut_obj_add_real(doc, root, "ColorValueZ", planeRendererComponent.Color.Z);
			yyjson_mut_obj_add_real(doc, root, "ColorValueW", planeRendererComponent.Color.W);

			yyjson_mut_obj_add_real(doc, root, "UVValueX", planeRendererComponent.UV.X);
			yyjson_mut_obj_add_real(doc, root, "UVValueY", planeRendererComponent.UV.Y);
			yyjson_mut_obj_add_uint(doc, root, "TextureHandle", planeRendererComponent.TextureHandle);
		}

		if (entity.HasComponent<OBJRendererComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "OBJRendererComponent");
			auto& objRendererComponent = entity.GetComponent<OBJRendererComponent>();

			yyjson_mut_obj_add_real(doc, root, "ColorValueX", objRendererComponent.Color.X);
			yyjson_mut_obj_add_real(doc, root, "ColorValueY", objRendererComponent.Color.Y);
			yyjson_mut_obj_add_real(doc, root, "ColorValueZ", objRendererComponent.Color.Z);
			yyjson_mut_obj_add_real(doc, root, "ColorValueW", objRendererComponent.Color.W);

			yyjson_mut_obj_add_real(doc, root, "UVValueX", objRendererComponent.UV.X);
			yyjson_mut_obj_add_real(doc, root, "UVValueY", objRendererComponent.UV.Y);
			yyjson_mut_obj_add_uint(doc, root, "ModelHandle", objRendererComponent.ModelHandle);
		}

		if (entity.HasComponent<ButtonWidgetComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "ButtonWidgetComponent");
			auto& buttonWidgetComponent = entity.GetComponent<ButtonWidgetComponent>();

			yyjson_mut_obj_add_real(doc, root, "ColorValueX", buttonWidgetComponent.Color.X);
			yyjson_mut_obj_add_real(doc, root, "ColorValueY", buttonWidgetComponent.Color.Y);
			yyjson_mut_obj_add_real(doc, root, "ColorValueZ", buttonWidgetComponent.Color.Z);
			yyjson_mut_obj_add_real(doc, root, "ColorValueW", buttonWidgetComponent.Color.W);

			yyjson_mut_obj_add_real(doc, root, "UVValueX", buttonWidgetComponent.UV.X);
			yyjson_mut_obj_add_real(doc, root, "UVValueY", buttonWidgetComponent.UV.Y);
			yyjson_mut_obj_add_uint(doc, root, "TextureHandle", buttonWidgetComponent.TextureHandle);

			yyjson_mut_obj_add_real(doc, root, "Radius", buttonWidgetComponent.Radius);
			yyjson_mut_obj_add_real(doc, root, "DimensionsX", buttonWidgetComponent.Dimensions.X);
			yyjson_mut_obj_add_real(doc, root, "DimensionsY", buttonWidgetComponent.Dimensions.Y);
			yyjson_mut_obj_add_bool(doc, root, "InvertCorners", buttonWidgetComponent.InvertCorners);
			yyjson_mut_obj_add_bool(doc, root, "UseLinear", buttonWidgetComponent.UseLinear);
		}

		if (entity.HasComponent<CircleWidgetComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "CircleWidgetComponent");
			auto& circleWidgetComponent = entity.GetComponent<CircleWidgetComponent>();

			yyjson_mut_obj_add_real(doc, root, "ColorValueX", circleWidgetComponent.Color.X);
			yyjson_mut_obj_add_real(doc, root, "ColorValueY", circleWidgetComponent.Color.Y);
			yyjson_mut_obj_add_real(doc, root, "ColorValueZ", circleWidgetComponent.Color.Z);
			yyjson_mut_obj_add_real(doc, root, "ColorValueW", circleWidgetComponent.Color.W);

			yyjson_mut_obj_add_real(doc, root, "UVValueX", circleWidgetComponent.UV.X);
			yyjson_mut_obj_add_real(doc, root, "UVValueY", circleWidgetComponent.UV.Y);
			yyjson_mut_obj_add_uint(doc, root, "TextureHandle", circleWidgetComponent.TextureHandle);

			yyjson_mut_obj_add_real(doc, root, "Fade", circleWidgetComponent.Fade);
			yyjson_mut_obj_add_bool(doc, root, "UseLinear", circleWidgetComponent.UseLinear);
		}

		if (entity.HasComponent<Rigidbody2DComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "Rigidbody2DComponent");

			auto& rigidbody2DComponent = entity.GetComponent<Rigidbody2DComponent>();

			yyjson_mut_obj_add_int(doc, root, "BodyType", (int)rigidbody2DComponent.Type);
			yyjson_mut_obj_add_bool(doc, root, "FixedRotation", rigidbody2DComponent.FixedRotation);
			yyjson_mut_obj_add_bool(doc, root, "SetEnabled", rigidbody2DComponent.SetEnabled);
		}

		if (entity.HasComponent<BoxCollider2DComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "BoxCollider2DComponent");

			auto& boxCollider2DComponent = entity.GetComponent<BoxCollider2DComponent>();

			yyjson_mut_obj_add_real(doc, root, "OffsetX", boxCollider2DComponent.Offset.X);
			yyjson_mut_obj_add_real(doc, root, "OffsetY", boxCollider2DComponent.Offset.Y);
			yyjson_mut_obj_add_real(doc, root, "SizeX", boxCollider2DComponent.Size.X);
			yyjson_mut_obj_add_real(doc, root, "SizeY", boxCollider2DComponent.Size.Y);
			yyjson_mut_obj_add_real(doc, root, "Density", boxCollider2DComponent.Density);
			yyjson_mut_obj_add_real(doc, root, "Friction", boxCollider2DComponent.Friction);
			yyjson_mut_obj_add_real(doc, root, "Restitution", boxCollider2DComponent.Restitution);
			yyjson_mut_obj_add_real(doc, root, "RestitutionThreshold", boxCollider2DComponent.RestitutionThreshold);
		}

		if (entity.HasComponent<CircleCollider2DComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "CircleCollider2DComponent");
			auto& circleCollider2DComponent = entity.GetComponent<CircleCollider2DComponent>();

			yyjson_mut_obj_add_real(doc, root, "OffsetX", circleCollider2DComponent.Offset.X);
			yyjson_mut_obj_add_real(doc, root, "OffsetY", circleCollider2DComponent.Offset.Y);
			yyjson_mut_obj_add_real(doc, root, "Radius", circleCollider2DComponent.Radius);
			yyjson_mut_obj_add_real(doc, root, "Density", circleCollider2DComponent.Density);
			yyjson_mut_obj_add_real(doc, root, "Friction", circleCollider2DComponent.Friction);
			yyjson_mut_obj_add_real(doc, root, "Restitution", circleCollider2DComponent.Restitution);
			yyjson_mut_obj_add_real(doc, root, "RestitutionThreshold", circleCollider2DComponent.RestitutionThreshold);
		}

		if (entity.HasComponent<TriangleCollider2DComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "TriangleCollider2DComponent");
			auto& triangleCollider2DComponent = entity.GetComponent<TriangleCollider2DComponent>();

			yyjson_mut_obj_add_real(doc, root, "OffsetX", triangleCollider2DComponent.Offset.X);
			yyjson_mut_obj_add_real(doc, root, "OffsetY", triangleCollider2DComponent.Offset.Y);
			yyjson_mut_obj_add_real(doc, root, "SizeX", triangleCollider2DComponent.Size.X);
			yyjson_mut_obj_add_real(doc, root, "SizeY", triangleCollider2DComponent.Size.Y);
			yyjson_mut_obj_add_real(doc, root, "Density", triangleCollider2DComponent.Density);
			yyjson_mut_obj_add_real(doc, root, "Friction", triangleCollider2DComponent.Friction);
			yyjson_mut_obj_add_real(doc, root, "Restitution", triangleCollider2DComponent.Restitution);
			yyjson_mut_obj_add_real(doc, root, "RestitutionThreshold", triangleCollider2DComponent.RestitutionThreshold);
		}

		if (entity.HasComponent<CapsuleCollider2DComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "CapsuleCollider2DComponent");
			auto& capsuleCollider2DComponent = entity.GetComponent<CapsuleCollider2DComponent>();

			yyjson_mut_obj_add_real(doc, root, "OffsetX", capsuleCollider2DComponent.Offset.X);
			yyjson_mut_obj_add_real(doc, root, "OffsetY", capsuleCollider2DComponent.Offset.Y);
			yyjson_mut_obj_add_real(doc, root, "SizeX", capsuleCollider2DComponent.Size.X);
			yyjson_mut_obj_add_real(doc, root, "SizeY", capsuleCollider2DComponent.Size.Y);
			yyjson_mut_obj_add_real(doc, root, "Density", capsuleCollider2DComponent.Density);
			yyjson_mut_obj_add_real(doc, root, "Friction", capsuleCollider2DComponent.Friction);
			yyjson_mut_obj_add_real(doc, root, "Restitution", capsuleCollider2DComponent.Restitution);
			yyjson_mut_obj_add_real(doc, root, "RestitutionThreshold", capsuleCollider2DComponent.RestitutionThreshold);
		}

		if (entity.HasComponent<MeshCollider2DComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "MeshCollider2DComponent");
			auto& meshCollider2DComponent = entity.GetComponent<MeshCollider2DComponent>();

			yyjson_mut_obj_add_real(doc, root, "OffsetX", meshCollider2DComponent.Offset.X);
			yyjson_mut_obj_add_real(doc, root, "OffsetY", meshCollider2DComponent.Offset.Y);
			yyjson_mut_obj_add_real(doc, root, "SizeX", meshCollider2DComponent.Size.X);
			yyjson_mut_obj_add_real(doc, root, "SizeY", meshCollider2DComponent.Size.Y);
			yyjson_mut_obj_add_real(doc, root, "Density", meshCollider2DComponent.Density);
			yyjson_mut_obj_add_real(doc, root, "Friction", meshCollider2DComponent.Friction);
			yyjson_mut_obj_add_real(doc, root, "Restitution", meshCollider2DComponent.Restitution);
			yyjson_mut_obj_add_real(doc, root, "RestitutionThreshold", meshCollider2DComponent.RestitutionThreshold);
			yyjson_mut_obj_add_uint(doc, root, "NumberOfPositions", meshCollider2DComponent.NumberOfPositions);

			if (meshCollider2DComponent.Positions.size() > 0)
			{
				for (uint32_t i = 0; i < meshCollider2DComponent.Positions.size(); i++)
				{
					std::string handle1 = "PositionX" + std::to_string(i) + '\0';
					char* endHandle1 = new char[handle1.length() + 1];
					std::copy(handle1.begin(), handle1.end(), endHandle1);
					yyjson_mut_obj_add_real(doc, root, endHandle1, meshCollider2DComponent.Positions[i].X);
					std::string handle2 = "PositionY" + std::to_string(i) + '\0';
					char* endHandle2 = new char[handle2.length() + 1];
					std::copy(handle2.begin(), handle2.end(), endHandle2);
					yyjson_mut_obj_add_real(doc, root, endHandle2, meshCollider2DComponent.Positions[i].Y);
				}
			}
		}

		if (entity.HasComponent<AudioSourceComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "AudioSourceComponent");

			const auto& audioSourceComponent = entity.GetComponent<AudioSourceComponent>();

			yyjson_mut_obj_add_uint(doc, root, "AudioHandle", audioSourceComponent.Audio);
			yyjson_mut_obj_add_real(doc, root, "VolumeMultiplier", audioSourceComponent.Config.VolumeMultiplier);
			yyjson_mut_obj_add_real(doc, root, "PitchMultiplier", audioSourceComponent.Config.PitchMultiplier);
			yyjson_mut_obj_add_bool(doc, root, "PlayOnAwake", audioSourceComponent.Config.PlayOnAwake);
			yyjson_mut_obj_add_bool(doc, root, "Looping", audioSourceComponent.Config.Looping);
			yyjson_mut_obj_add_bool(doc, root, "Spatialization", audioSourceComponent.Config.Spatialization);
			yyjson_mut_obj_add_int(doc, root, "AttenuationModel", static_cast<int>(audioSourceComponent.Config.AttenuationModel));
			yyjson_mut_obj_add_real(doc, root, "RollOff", audioSourceComponent.Config.RollOff);
			yyjson_mut_obj_add_real(doc, root, "MinGain", audioSourceComponent.Config.MinGain);
			yyjson_mut_obj_add_real(doc, root, "MaxGain", audioSourceComponent.Config.MaxGain);
			yyjson_mut_obj_add_real(doc, root, "MinDistance", audioSourceComponent.Config.MinDistance);
			yyjson_mut_obj_add_real(doc, root, "MaxDistance", audioSourceComponent.Config.MaxDistance);
			yyjson_mut_obj_add_real(doc, root, "ConeInnerAngle", audioSourceComponent.Config.ConeInnerAngle);
			yyjson_mut_obj_add_real(doc, root, "ConeOuterAngle", audioSourceComponent.Config.ConeOuterAngle);
			yyjson_mut_obj_add_real(doc, root, "ConeOuterGain", audioSourceComponent.Config.ConeOuterGain);
			yyjson_mut_obj_add_real(doc, root, "DopplerFactor", audioSourceComponent.Config.DopplerFactor);

			yyjson_mut_obj_add_bool(doc, root, "UsePlaylist", audioSourceComponent.AudioSourceData.UsePlaylist);

			if (audioSourceComponent.AudioSourceData.UsePlaylist)
			{
				yyjson_mut_obj_add_uint(doc, root, "AudioSourcesSize", audioSourceComponent.AudioSourceData.NumberOfAudioSources);
				yyjson_mut_obj_add_uint(doc, root, "StartIndex", audioSourceComponent.AudioSourceData.StartIndex);
				yyjson_mut_obj_add_bool(doc, root, "RepeatPlaylist", audioSourceComponent.AudioSourceData.RepeatPlaylist);
				yyjson_mut_obj_add_bool(doc, root, "RepeatSpecificTrack", audioSourceComponent.AudioSourceData.RepeatAfterSpecificTrackPlays);

				for (uint32_t i = 0; i < audioSourceComponent.AudioSourceData.Playlist.size(); i++)
				{
					if (audioSourceComponent.AudioSourceData.Playlist[i])
					{
						std::string handle = "AudioHandle" + std::to_string(i) + '\0';
						char* endHandle = new char[handle.length() + 1];
						std::copy(handle.begin(), handle.end(), endHandle);

						yyjson_mut_obj_add_uint(doc, root, endHandle, (uint64_t)audioSourceComponent.AudioSourceData.Playlist[i]);
					}
				}
			}
		}

		if (entity.HasComponent<AudioListenerComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "AudioListenerComponent");
			const auto& audioListenerComponent = entity.GetComponent<AudioListenerComponent>();

			yyjson_mut_obj_add_bool(doc, root, "Active", audioListenerComponent.Active);
			yyjson_mut_obj_add_real(doc, root, "ConeInnerAngle", audioListenerComponent.Config.ConeInnerAngle);
			yyjson_mut_obj_add_real(doc, root, "ConeOuterAngle", audioListenerComponent.Config.ConeOuterAngle);
			yyjson_mut_obj_add_real(doc, root, "ConeOuterGain", audioListenerComponent.Config.ConeOuterGain);
		}

		if (entity.HasComponent<VideoRendererComponent>())
		{
			yyjson_mut_obj_add_str(doc, root, "Component", "VideoRendererComponent");
			auto& videoRendererComponent = entity.GetComponent<VideoRendererComponent>();

			yyjson_mut_obj_add_real(doc, root, "ColorValueX", videoRendererComponent.Color.X);
			yyjson_mut_obj_add_real(doc, root, "ColorValueY", videoRendererComponent.Color.Y);
			yyjson_mut_obj_add_real(doc, root, "ColorValueZ", videoRendererComponent.Color.Z);
			yyjson_mut_obj_add_real(doc, root, "ColorValueW", videoRendererComponent.Color.W);

			yyjson_mut_obj_add_bool(doc, root, "RepeatVideo", videoRendererComponent.m_VideoData.RepeatVideo);
			yyjson_mut_obj_add_uint(doc, root, "VideoHandle", videoRendererComponent.Video);
			yyjson_mut_obj_add_bool(doc, root, "UseExternalAudio", videoRendererComponent.m_VideoData.UseExternalAudio);

			if (videoRendererComponent.Video)
				yyjson_mut_obj_add_real(doc, root, "Volume", videoRendererComponent.m_VideoData.Volume);
		}
	}

	void SceneSerializer::Serialize(const std::filesystem::path& filepath)
	{
		Timer timer;
		
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << m_Scene->GetName();
		out << YAML::Key << "Gravity" << YAML::Value << m_Scene->GetPhysics2DGravity();
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		m_Scene->m_ECS.each([&](flecs::entity entityID, IDComponent& id)
		{
			Entity entity = { entityID };
			if (!entity || id.ID == 0)
				return;

			SerializeEntity(out, entity, m_Scene);
		});
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
		
		NZ_CORE_WARN("YAML Serialization took {0} seconds", timer.ElapsedMicros());
	}

	void SceneSerializer::SerializeJSON(const std::filesystem::path& filepath)
	{
		Timer timer;

		yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
		yyjson_mut_val* root = yyjson_mut_obj(doc);
		yyjson_mut_doc_set_root(doc, root);

		yyjson_mut_obj_add_str(doc, root, "Scene", m_Scene->GetName().c_str() ? m_Scene->GetName().c_str() : "Empty Scene");
		yyjson_mut_obj_add_real(doc, root, "GravityX", m_Scene->GetPhysics2DGravity().X);
		yyjson_mut_obj_add_real(doc, root, "GravityY", m_Scene->GetPhysics2DGravity().Y);
		yyjson_mut_obj_add_uint(doc, root, "NumOfEntities", m_Scene->m_ECS.count<IDComponent>());
				
		{
			m_Scene->m_ECS.each([&](flecs::entity entityID, IDComponent& id)
			{
				Entity entity = { entityID };
				//if (!entity || id.ID == 0)
				if (!entity || entity.GetEntityHandle() < 567)
					return;

				SerializeEntityJSON(doc, root, entity, m_Scene);
			});
		}

		// Write the json pretty, escape unicode
		yyjson_write_flag flg = YYJSON_WRITE_PRETTY | YYJSON_WRITE_ESCAPE_UNICODE;
		yyjson_write_err err;
		yyjson_mut_write_file(filepath.string().c_str(), doc, flg, NULL, &err);
		
		if (err.code)
			NZ_CORE_WARN("Write error ({0}): {1}", err.code, err.msg);

		// Free the doc
		yyjson_mut_doc_free(doc);

		NZ_CORE_WARN("JSON (Scene file) Serialization took {0} seconds", timer.ElapsedMicros());
	}

	void SceneSerializer::SerializeRuntime(const std::filesystem::path& filepath)
	{
		// Not implemented
		NZ_CORE_ASSERT(false);
	}

	bool SceneSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		Timer timer;
		
		YAML::Node data;
		try
		{
			data = YAML::LoadFile(filepath.string());
		}
		catch (YAML::ParserException e)
		{
			return false;
		}

		if (!data["Scene"])
			return false;

		std::string sceneName = data["Scene"].as<std::string>();
		NZ_CORE_TRACE("Deserializing scene '{0}'", sceneName.c_str());
		m_Scene->SetName(sceneName);

		if (data["Gravity"])
		{
			rtmcpp::Vec2 gravity = data["Gravity"].as<rtmcpp::Vec2>();
			m_Scene->SetPhysics2DGravity(gravity);
		}

		auto entities = data["Entities"];
		if (entities)
		{
			for (auto entity : entities)
			{
				uint64_t id = entity["Entity"].as<uint64_t>(); // TODO

				std::string name;
				auto tagComponent = entity["TagComponent"];
				if (tagComponent)
					name = tagComponent["Tag"].as<std::string>();

				NZ_CORE_TRACE("Deserialized entity with ID = {0}, name = {1}", id, name.c_str());

				Entity deserializedEntity = m_Scene->CreateEntityWithID(id, name);

				auto transformComponent = entity["TransformComponent"];
				if (transformComponent)
				{
					// Entities always have transforms
					auto& tc = deserializedEntity.GetComponent<TransformComponent>();

					if (transformComponent["Enabled"])
						tc.Enabled = transformComponent["Enabled"].as<bool>();

					if (transformComponent["Translation"])
						tc.Translation = transformComponent["Translation"].as<rtmcpp::Vec4>();

					if (transformComponent["Rotation"])
						tc.Rotation = transformComponent["Rotation"].as<rtmcpp::Vec3>();

					if (transformComponent["Scale"])
						tc.Scale = transformComponent["Scale"].as<rtmcpp::Vec3>();
				}

				auto cameraComponent = entity["CameraComponent"];
				if (cameraComponent)
				{
					auto& cc = deserializedEntity.AddComponent<CameraComponent>();
					cc.Camera = RefPtr<SceneCamera>::Create();

					auto cameraProps = cameraComponent["Camera"];

					if (cameraProps["ProjectionType"])
						cc.Camera->SetProjectionType((SceneCamera::ProjectionType)cameraProps["ProjectionType"].as<int>());

					if (cameraProps["PerspectiveFOV"])
						cc.Camera->SetPerspectiveVerticalFOV(cameraProps["PerspectiveFOV"].as<float>());

					if (cameraProps["PerspectiveNear"])
						cc.Camera->SetPerspectiveNearClip(cameraProps["PerspectiveNear"].as<float>());

					if (cameraProps["PerspectiveFar"])
						cc.Camera->SetPerspectiveFarClip(cameraProps["PerspectiveFar"].as<float>());

					if (cameraProps["OrthographicSize"])
						cc.Camera->SetOrthographicSize(cameraProps["OrthographicSize"].as<float>());

					if (cameraProps["OrthographicNear"])
						cc.Camera->SetOrthographicNearClip(cameraProps["OrthographicNear"].as<float>());

					if (cameraProps["OrthographicFar"])
						cc.Camera->SetOrthographicFarClip(cameraProps["OrthographicFar"].as<float>());

					if (cameraComponent["Primary"])
						cc.Primary = cameraComponent["Primary"].as<bool>();

					if (cameraComponent["FixedAspectRatio"])
						cc.FixedAspectRatio = cameraComponent["FixedAspectRatio"].as<bool>();
				}

				auto scriptComponent = entity["ScriptComponent"];
				if (scriptComponent)
				{
					uint64_t scriptID = scriptComponent["ScriptHandle"].as<uint64_t>(0);

					if (scriptID == 0)
					{
						scriptID = scriptComponent["ClassHandle"].as<uint64_t>(0);
						NZ_CORE_VERIFY(scriptID == 0);
					}

					{
						auto& scriptEngine = ScriptEngine::GetMutable();

						if (scriptEngine.IsValidScript(scriptID))
						{
							const auto& scriptMetadata = scriptEngine.GetScriptMetadata(scriptID);

							ScriptComponent& sc = deserializedEntity.AddComponent<ScriptComponent>();
							sc.ScriptHandle = scriptID;

							m_Scene->m_ScriptStorage.InitializeEntityStorage(sc.ScriptHandle, deserializedEntity.GetEntityHandle());

							bool oldFormat = false;

							auto fieldsArray = scriptComponent["Fields"];

							if (!fieldsArray)
							{
								fieldsArray = scriptComponent["StoredFields"];
								oldFormat = true;
							}

							for (auto field : fieldsArray)
							{
								uint32_t fieldID = field["ID"].as<uint32_t>(0);
								auto fieldName = field["Name"].as<std::string>("");

								if (oldFormat && fieldName.find(':') != std::string::npos)
								{
									// Old format, try generating id from name
									fieldName = fieldName.substr(fieldName.find(':') + 1);
									fieldID = Hash::GenerateFNVHash(fieldName);
								}

								if (scriptMetadata.Fields.contains(fieldID))
								{
									const auto& fieldMetadata = scriptMetadata.Fields.at(fieldID);
									auto& fieldStorage = m_Scene->m_ScriptStorage.EntityStorage.at(deserializedEntity.GetEntityHandle()).Fields[fieldID];

									auto valueNode = oldFormat ? field["Data"] : field["Value"];

									switch (fieldMetadata.Type)
									{
									case DataType::SByte:
									{
										fieldStorage.SetValue(valueNode.as<int8_t>());
										break;
									}
									case DataType::Byte:
									{
										fieldStorage.SetValue(valueNode.as<uint8_t>());
										break;
									}
									case DataType::Short:
									{
										fieldStorage.SetValue(valueNode.as<int16_t>());
										break;
									}
									case DataType::UShort:
									{
										fieldStorage.SetValue(valueNode.as<uint16_t>());
										break;
									}
									case DataType::Int:
									{
										fieldStorage.SetValue(valueNode.as<int32_t>());
										break;
									}
									case DataType::UInt:
									{
										fieldStorage.SetValue(valueNode.as<uint32_t>());
										break;
									}
									case DataType::Long:
									{
										fieldStorage.SetValue(valueNode.as<int64_t>());
										break;
									}
									case DataType::ULong:
									{
										fieldStorage.SetValue(valueNode.as<uint64_t>());
										break;
									}
									case DataType::Float:
									{
										fieldStorage.SetValue(valueNode.as<float>());
										break;
									}
									case DataType::Double:
									{
										fieldStorage.SetValue(valueNode.as<double>());
										break;
									}
									case DataType::Bool:
									{
										fieldStorage.SetValue(valueNode.as<bool>());
										break;
									}
									case DataType::String:
									{
										fieldStorage.SetValue(Coral::String::New(valueNode.as<std::string>()));
										break;
									}
									case DataType::Bool32:
									{
										fieldStorage.SetValue((uint32_t)valueNode.as<bool>());
										break;
									}
									case DataType::AssetHandle:
									{
										fieldStorage.SetValue(valueNode.as<uint64_t>());
										break;
									}
									case DataType::Vector2:
									{
										fieldStorage.SetValue(valueNode.as<rtmcpp::Vec2>());
										break;
									}
									case DataType::Vector3:
									{
										fieldStorage.SetValue(valueNode.as<rtmcpp::Vec3>());
										break;
									}
									case DataType::Vector4:
									{
										fieldStorage.SetValue(valueNode.as<rtmcpp::Vec4>());
										break;
									}
									//case DataType::Entity:
									//{
									//	fieldStorage.SetValue(valueNode.as<uint64_t>());
									//	break;
									//}
									//case DataType::Prefab:
									//{
									//	fieldStorage.SetValue(valueNode.as<uint64_t>());
									//	break;
									//}
									//case DataType::Mesh:
									//{
									//	fieldStorage.SetValue(valueNode.as<uint64_t>());
									//	break;
									//}
									//case DataType::StaticMesh:
									//{
									//	fieldStorage.SetValue(valueNode.as<uint64_t>());
									//	break;
									//}
									//case DataType::Material:
									//{
									//	fieldStorage.SetValue(valueNode.as<uint64_t>());
									//	break;
									//}
									//case DataType::Texture2D:
									//{
									//	fieldStorage.SetValue(valueNode.as<uint64_t>());
									//	break;
									//}
									//case DataType::Scene:
									//{
									//	fieldStorage.SetValue(valueNode.as<uint64_t>());
									//	break;
									//}
									default:
										break;
									}
								}
							}

							sc.HasInitializedScript = true;
						}
					}
				}

				auto spriteRendererComponent = entity["SpriteRendererComponent"];
				if (spriteRendererComponent)
				{
					auto& src = deserializedEntity.AddComponent<SpriteRendererComponent>();

					src.Color = spriteRendererComponent["Color"].as<rtmcpp::Vec4>();

					if (spriteRendererComponent["UV"])
						src.UV = spriteRendererComponent["UV"].as<rtmcpp::Vec2>();

					if (spriteRendererComponent["TextureHandle"])
						src.TextureHandle = spriteRendererComponent["TextureHandle"].as<AssetHandle>();

					if (spriteRendererComponent["UseParallaxScrolling"])
						src.m_AnimationData.UseParallaxScrolling = spriteRendererComponent["UseParallaxScrolling"].as<bool>();

					if (src.m_AnimationData.UseParallaxScrolling)
					{
						if (spriteRendererComponent["UseCameraParallaxX"])
							src.m_AnimationData.UseCameraParallaxX = spriteRendererComponent["UseCameraParallaxX"].as<bool>();

						if (spriteRendererComponent["UseCameraParallaxY"])
							src.m_AnimationData.UseCameraParallaxY = spriteRendererComponent["UseCameraParallaxY"].as<bool>();

						if (spriteRendererComponent["ParallaxSpeed"])
							src.m_AnimationData.ParallaxSpeed = spriteRendererComponent["ParallaxSpeed"].as<rtmcpp::Vec2>();

						if (spriteRendererComponent["ParallaxDivision"])
							src.m_AnimationData.ParallaxDivision = spriteRendererComponent["ParallaxDivision"].as<float>();
					}

					if (spriteRendererComponent["UseTextureAtlasAnimation"])
						src.m_AnimationData.UseTextureAtlasAnimation = spriteRendererComponent["UseTextureAtlasAnimation"].as<bool>();

					if (spriteRendererComponent["AnimationSpeed"])
						src.m_AnimationData.AnimationSpeed = spriteRendererComponent["AnimationSpeed"].as<float>();

					if (spriteRendererComponent["NumberOfTiles"])
						src.m_AnimationData.NumberOfTiles = spriteRendererComponent["NumberOfTiles"].as<int>();

					if (spriteRendererComponent["StartIndexX"])
						src.m_AnimationData.StartIndexX = spriteRendererComponent["StartIndexX"].as<int>();

					if (spriteRendererComponent["StartIndexY"])
						src.m_AnimationData.StartIndexY = spriteRendererComponent["StartIndexY"].as<int>();

					if (spriteRendererComponent["Rows"])
						src.m_AnimationData.Rows = spriteRendererComponent["Rows"].as<int>();

					if (spriteRendererComponent["Columns"])
						src.m_AnimationData.Columns = spriteRendererComponent["Columns"].as<int>();

					if (spriteRendererComponent["UseLinear"])
						src.m_AnimationData.UseLinear = spriteRendererComponent["UseLinear"].as<bool>();

					if (spriteRendererComponent["UsePerTextureAnimation"])
						src.m_AnimationData.UsePerTextureAnimation = spriteRendererComponent["UsePerTextureAnimation"].as<bool>();

					if (src.m_AnimationData.UsePerTextureAnimation)
					{
						if (spriteRendererComponent["StartIndex"])
							src.m_AnimationData.StartIndex = spriteRendererComponent["StartIndex"].as<int>();

						if (spriteRendererComponent["TexturesSize"])
							src.m_AnimationData.NumberOfTextures = spriteRendererComponent["TexturesSize"].as<int>();

						for (uint32_t i = 0; i < src.m_AnimationData.NumberOfTextures; i++)
						{
							std::string texPath = "TextureHandle" + std::to_string(i);
							if (spriteRendererComponent[texPath.c_str()])
							{
								AssetHandle textureHandle = spriteRendererComponent[texPath.c_str()].as<AssetHandle>();
								src.m_AnimationData.Textures.emplace_back(textureHandle);

							}
						}
					}
				}

				auto circleRendererComponent = entity["CircleRendererComponent"];
				if (circleRendererComponent)
				{
					auto& src = deserializedEntity.AddComponent<CircleRendererComponent>();
					
					if (circleRendererComponent["Color"])
						src.Color = circleRendererComponent["Color"].as<rtmcpp::Vec4>();

					if (circleRendererComponent["UV"])
						src.UV = circleRendererComponent["UV"].as<rtmcpp::Vec2>();

					if (circleRendererComponent["Thickness"])
						src.Thickness = circleRendererComponent["Thickness"].as<float>();

					if (circleRendererComponent["Fade"])
						src.Fade = circleRendererComponent["Fade"].as<float>();

					if (circleRendererComponent["MakeSolid"])
						src.MakeSolid = circleRendererComponent["MakeSolid"].as<bool>();

					if (circleRendererComponent["TextureHandle"])
						src.TextureHandle = circleRendererComponent["TextureHandle"].as<AssetHandle>();

					if (circleRendererComponent["UseParallaxScrolling"])
						src.m_AnimationData.UseParallaxScrolling = circleRendererComponent["UseParallaxScrolling"].as<bool>();

					if (src.m_AnimationData.UseParallaxScrolling)
					{
						if (circleRendererComponent["UseCameraParallaxX"])
							src.m_AnimationData.UseCameraParallaxX = circleRendererComponent["UseCameraParallaxX"].as<bool>();

						if (circleRendererComponent["UseCameraParallaxY"])
							src.m_AnimationData.UseCameraParallaxY = circleRendererComponent["UseCameraParallaxY"].as<bool>();

						if (circleRendererComponent["ParallaxSpeed"])
							src.m_AnimationData.ParallaxSpeed = circleRendererComponent["ParallaxSpeed"].as<rtmcpp::Vec2>();

						if (circleRendererComponent["ParallaxDivision"])
							src.m_AnimationData.ParallaxDivision = circleRendererComponent["ParallaxDivision"].as<float>();
					}

					if (circleRendererComponent["UseTextureAtlasAnimation"])
						src.m_AnimationData.UseTextureAtlasAnimation = circleRendererComponent["UseTextureAtlasAnimation"].as<bool>();

					if (circleRendererComponent["AnimationSpeed"])
						src.m_AnimationData.AnimationSpeed = circleRendererComponent["AnimationSpeed"].as<float>();

					if (circleRendererComponent["NumberOfTiles"])
						src.m_AnimationData.NumberOfTiles = circleRendererComponent["NumberOfTiles"].as<int>();

					if (circleRendererComponent["StartIndexX"])
						src.m_AnimationData.StartIndexX = circleRendererComponent["StartIndexX"].as<int>();

					if (circleRendererComponent["StartIndexY"])
						src.m_AnimationData.StartIndexY = circleRendererComponent["StartIndexY"].as<int>();

					if (circleRendererComponent["Rows"])
						src.m_AnimationData.Rows = circleRendererComponent["Rows"].as<int>();

					if (circleRendererComponent["Columns"])
						src.m_AnimationData.Columns = circleRendererComponent["Columns"].as<int>();

					if (circleRendererComponent["UseLinear"])
						src.m_AnimationData.UseLinear = circleRendererComponent["UseLinear"].as<bool>();
				}

				auto triangleRendererComponent = entity["TriangleRendererComponent"];
				if (triangleRendererComponent)
				{
					auto& src = deserializedEntity.AddComponent<TriangleRendererComponent>();

					if (triangleRendererComponent["Color"])
						src.Color = triangleRendererComponent["Color"].as<rtmcpp::Vec4>();

					if (triangleRendererComponent["UV"])
						src.UV = triangleRendererComponent["UV"].as<rtmcpp::Vec2>();

					if (triangleRendererComponent["TextureHandle"])
						src.TextureHandle = triangleRendererComponent["TextureHandle"].as<AssetHandle>();
				}

				auto lineRendererComponent = entity["LineRendererComponent"];
				if (lineRendererComponent)
				{
					auto& src = deserializedEntity.AddComponent<LineRendererComponent>();

					if (lineRendererComponent["LineThickness"])
						src.LineThickness = lineRendererComponent["LineThickness"].as<float>();

					if (lineRendererComponent["NumberOfColors"])
					{
						src.NumberOfColors = lineRendererComponent["NumberOfColors"].as<uint32_t>();
						src.AddColor(rtmcpp::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
					}

					if (lineRendererComponent["NumberOfTranslations"])
						src.NumberOfTranslations = lineRendererComponent["NumberOfTranslations"].as<uint32_t>();

					uint32_t numberOfTranslations = src.NumberOfTranslations;

					for (uint32_t i = 0; i < numberOfTranslations; i++)
					{
						if (i >= 1)
						{
							std::string keyColorName = "Color" + std::to_string(i);
							if (lineRendererComponent[keyColorName.c_str()])
							{
								src.AddColor(lineRendererComponent[keyColorName.c_str()].as<rtmcpp::Vec4>());
							}
						}

						std::string keyTranslationName = "Translation" + std::to_string(i);
						if (lineRendererComponent[keyTranslationName.c_str()])
						{
							src.AddTranslation(lineRendererComponent[keyTranslationName.c_str()].as<rtmcpp::Vec4>());
						}
					}
				}

				auto textComponent = entity["TextComponent"];
				if (textComponent)
				{
					auto& src = deserializedEntity.AddComponent<TextComponent>();

					if (textComponent["TextString"])
					{
						src.TextString = textComponent["TextString"].as<std::string>();
					}

					if (textComponent["FontHandle"])
					{
						src.FontHandle = textComponent["FontHandle"].as<AssetHandle>();
					}

					src.Color = textComponent["Color"].as<rtmcpp::Vec4>();

					if (textComponent["LinearFiltering"])
						src.UseLinear = textComponent["LinearFiltering"].as<bool>();

					if (textComponent["LineSpacing"])
						src.LineSpacing = textComponent["LineSpacing"].as<float>();

					if (textComponent["Kerning"])
						src.Kerning = textComponent["Kerning"].as<float>();
				}

				auto particleSystemComponent = entity["ParticleSystemComponent"];
				if (particleSystemComponent)
				{
					auto& src = deserializedEntity.AddComponent<ParticleSystemComponent>();

					if (particleSystemComponent["Velocity"])
						src.Velocity = particleSystemComponent["Velocity"].as<rtmcpp::Vec3>();

					if (particleSystemComponent["VelocityVariation"])
						src.VelocityVariation = particleSystemComponent["VelocityVariation"].as<rtmcpp::Vec3>();

					if (particleSystemComponent["ColorBegin"])
						src.ColorBegin = particleSystemComponent["ColorBegin"].as<rtmcpp::Vec4>();

					if (particleSystemComponent["ColorEnd"])
						src.ColorEnd = particleSystemComponent["ColorEnd"].as<rtmcpp::Vec4>();

					if (particleSystemComponent["TextureHandle"])
						src.TextureHandle = particleSystemComponent["TextureHandle"].as<AssetHandle>();

					if (particleSystemComponent["SizeBegin"])
						src.SizeBegin = particleSystemComponent["SizeBegin"].as<float>();

					if (particleSystemComponent["SizeEnd"])
						src.SizeEnd = particleSystemComponent["SizeEnd"].as<float>();

					if (particleSystemComponent["SizeVariation"])
						src.SizeVariation = particleSystemComponent["SizeVariation"].as<float>();

					if (particleSystemComponent["LifeTime"])
						src.LifeTime = particleSystemComponent["LifeTime"].as<float>();

					if (particleSystemComponent["ParticleSize"])
						src.ParticleSize = particleSystemComponent["ParticleSize"].as<int>();

					if (particleSystemComponent["UseLinear"])
						src.UseLinear = particleSystemComponent["UseLinear"].as<bool>();

					if (particleSystemComponent["UseBillboard"])
						src.UseBillboard = particleSystemComponent["UseBillboard"].as<bool>();
				}

				auto cubeRendererComponent = entity["CubeRendererComponent"];
				if (cubeRendererComponent)
				{
					auto& src = deserializedEntity.AddComponent<CubeRendererComponent>();

					src.Color = cubeRendererComponent["Color"].as<rtmcpp::Vec4>();

					if (cubeRendererComponent["UV"])
						src.UV = cubeRendererComponent["UV"].as<rtmcpp::Vec2>();

					if (cubeRendererComponent["TextureHandle"])
						src.TextureHandle = cubeRendererComponent["TextureHandle"].as<AssetHandle>();

					if (cubeRendererComponent["UseParallaxScrolling"])
						src.m_AnimationData.UseParallaxScrolling = cubeRendererComponent["UseParallaxScrolling"].as<bool>();

					if (src.m_AnimationData.UseParallaxScrolling)
					{
						if (cubeRendererComponent["UseCameraParallaxX"])
							src.m_AnimationData.UseCameraParallaxX = cubeRendererComponent["UseCameraParallaxX"].as<bool>();

						if (cubeRendererComponent["UseCameraParallaxY"])
							src.m_AnimationData.UseCameraParallaxY = cubeRendererComponent["UseCameraParallaxY"].as<bool>();

						if (cubeRendererComponent["ParallaxSpeed"])
							src.m_AnimationData.ParallaxSpeed = cubeRendererComponent["ParallaxSpeed"].as<rtmcpp::Vec2>();

						if (cubeRendererComponent["ParallaxDivision"])
							src.m_AnimationData.ParallaxDivision = cubeRendererComponent["ParallaxDivision"].as<float>();
					}

					if (cubeRendererComponent["UseTextureAtlasAnimation"])
						src.m_AnimationData.UseTextureAtlasAnimation = cubeRendererComponent["UseTextureAtlasAnimation"].as<bool>();

					if (cubeRendererComponent["AnimationSpeed"])
						src.m_AnimationData.AnimationSpeed = cubeRendererComponent["AnimationSpeed"].as<float>();

					if (cubeRendererComponent["NumberOfTiles"])
						src.m_AnimationData.NumberOfTiles = cubeRendererComponent["NumberOfTiles"].as<int>();

					if (cubeRendererComponent["StartIndexX"])
						src.m_AnimationData.StartIndexX = cubeRendererComponent["StartIndexX"].as<int>();

					if (cubeRendererComponent["StartIndexY"])
						src.m_AnimationData.StartIndexY = cubeRendererComponent["StartIndexY"].as<int>();

					if (cubeRendererComponent["Rows"])
						src.m_AnimationData.Rows = cubeRendererComponent["Rows"].as<int>();

					if (cubeRendererComponent["Columns"])
						src.m_AnimationData.Columns = cubeRendererComponent["Columns"].as<int>();

					if (cubeRendererComponent["UseLinear"])
						src.m_AnimationData.UseLinear = cubeRendererComponent["UseLinear"].as<bool>();

					if (cubeRendererComponent["UsePerTextureAnimation"])
						src.m_AnimationData.UsePerTextureAnimation = cubeRendererComponent["UsePerTextureAnimation"].as<bool>();

					if (src.m_AnimationData.UsePerTextureAnimation)
					{
						if (cubeRendererComponent["StartIndex"])
							src.m_AnimationData.StartIndex = cubeRendererComponent["StartIndex"].as<int>();

						if (cubeRendererComponent["TexturesSize"])
							src.m_AnimationData.NumberOfTextures = cubeRendererComponent["TexturesSize"].as<int>();

						for (uint32_t i = 0; i < src.m_AnimationData.NumberOfTextures; i++)
						{
							std::string texPath = "TextureHandle" + std::to_string(i);
							if (cubeRendererComponent[texPath.c_str()])
							{
								AssetHandle textureHandle = cubeRendererComponent[texPath.c_str()].as<AssetHandle>();
								src.m_AnimationData.Textures.emplace_back(textureHandle);

							}
						}
					}
				}

				auto pyramidRendererComponent = entity["PyramidRendererComponent"];
				if (pyramidRendererComponent)
				{
					auto& src = deserializedEntity.AddComponent<PyramidRendererComponent>();

					if (pyramidRendererComponent["Color"])
						src.Color = pyramidRendererComponent["Color"].as<rtmcpp::Vec4>();

					if (pyramidRendererComponent["UV"])
						src.UV = pyramidRendererComponent["UV"].as<rtmcpp::Vec2>();

					if (pyramidRendererComponent["TextureHandle"])
						src.TextureHandle = pyramidRendererComponent["TextureHandle"].as<AssetHandle>();
				}

				auto triangularPrismRendererComponent = entity["TriangularPrismRendererComponent"];
				if (triangularPrismRendererComponent)
				{
					auto& src = deserializedEntity.AddComponent<TriangularPrismRendererComponent>();

					if (triangularPrismRendererComponent["Color"])
						src.Color = triangularPrismRendererComponent["Color"].as<rtmcpp::Vec4>();

					if (triangularPrismRendererComponent["UV"])
						src.UV = triangularPrismRendererComponent["UV"].as<rtmcpp::Vec2>();

					if (triangularPrismRendererComponent["TextureHandle"])
						src.TextureHandle = triangularPrismRendererComponent["TextureHandle"].as<AssetHandle>();
				}

				auto planeRendererComponent = entity["PlaneRendererComponent"];
				if (planeRendererComponent)
				{
					auto& src = deserializedEntity.AddComponent<PlaneRendererComponent>();

					if (planeRendererComponent["Color"])
						src.Color = planeRendererComponent["Color"].as<rtmcpp::Vec4>();

					if (planeRendererComponent["UV"])
						src.UV = planeRendererComponent["UV"].as<rtmcpp::Vec2>();

					if (planeRendererComponent["TextureHandle"])
						src.TextureHandle = planeRendererComponent["TextureHandle"].as<AssetHandle>();
				}

				auto objRendererComponent = entity["OBJRendererComponent"];
				if (objRendererComponent)
				{
					auto& src = deserializedEntity.AddComponent<OBJRendererComponent>();

					if (objRendererComponent["Color"])
						src.Color = objRendererComponent["Color"].as<rtmcpp::Vec4>();

					if (objRendererComponent["UV"])
						src.UV = objRendererComponent["UV"].as<rtmcpp::Vec2>();

					//if (objRendererComponent["FilePath"])
					//{
					//	std::filesystem::path path = objRendererComponent["FilePath"].as<std::string>();
					//	std::filesystem::path absoluteFilePath = std::filesystem::absolute(path).string();
					//
					//	if (absoluteFilePath.extension() == ".obj")
					//	{
					//		if (Renderer3D::LoadTinyObj(absoluteFilePath, src))
					//		{
					//			src.Filepath = path.string();
					//			src.HasLoadedFile = true;
					//
					//			NZ_CORE_INFO("Successfully loaded obj file: {0}", absoluteFilePath);
					//		}
					//		else
					//		{
					//			NZ_CONSOLE_LOG_CORE_WARN("Could not load file: %s", absoluteFilePath.filename().string());
					//		}
					//	}
					//}

					//if (objRendererComponent["TexturePath"])
					//{
					//	std::filesystem::path path = objRendererComponent["TexturePath"].as<std::string>();
					//	std::string texturePath = std::filesystem::absolute(path).string();
					//	auto finalPath = Project::GetAssetFileSystemPath(texturePath);
					//	src.Texture = Texture2D::Create(finalPath.string());
					//}
				}

				auto buttonWidgetComponent = entity["ButtonWidgetComponent"];
				if (buttonWidgetComponent)
				{
					auto& src = deserializedEntity.AddComponent<ButtonWidgetComponent>();

					src.Color = buttonWidgetComponent["Color"].as<rtmcpp::Vec4>();

					if (buttonWidgetComponent["UV"])
						src.UV = buttonWidgetComponent["UV"].as<rtmcpp::Vec2>();

					if (buttonWidgetComponent["TextureHandle"])
						src.TextureHandle = buttonWidgetComponent["TextureHandle"].as<AssetHandle>();

					if (buttonWidgetComponent["Radius"])
						src.Radius = buttonWidgetComponent["Radius"].as<float>();

					if (buttonWidgetComponent["Dimensions"])
						src.Dimensions = buttonWidgetComponent["Dimensions"].as<rtmcpp::Vec2>();

					if (buttonWidgetComponent["InvertCorners"])
						src.InvertCorners = buttonWidgetComponent["InvertCorners"].as<bool>();

					if (buttonWidgetComponent["UseLinear"])
						src.UseLinear = buttonWidgetComponent["UseLinear"].as<bool>();
				}

				auto circleWidgetComponent = entity["CircleWidgetComponent"];
				if (circleWidgetComponent)
				{
					auto& src = deserializedEntity.AddComponent<CircleWidgetComponent>();

					src.Color = circleWidgetComponent["Color"].as<rtmcpp::Vec4>();

					if (circleWidgetComponent["UV"])
						src.UV = circleWidgetComponent["UV"].as<rtmcpp::Vec2>();

					if (circleWidgetComponent["TextureHandle"])
						src.TextureHandle = circleWidgetComponent["TextureHandle"].as<AssetHandle>();

					if (circleWidgetComponent["Fade"])
						src.Fade = circleWidgetComponent["Fade"].as<float>();

					if (circleWidgetComponent["UseLinear"])
						src.UseLinear = circleWidgetComponent["UseLinear"].as<bool>();
				}

				auto rigidBody2DComponent = entity["RigidBody2DComponent"];
				if (rigidBody2DComponent)
				{
					auto& component = deserializedEntity.AddComponent<Rigidbody2DComponent>();
					component.Type = (Rigidbody2DComponent::BodyType)rigidBody2DComponent["BodyType"].as<int>();
					component.FixedRotation = rigidBody2DComponent["FixedRotation"] ? rigidBody2DComponent["FixedRotation"].as<bool>() : false;

					if (rigidBody2DComponent["SetEnabled"])
						component.SetEnabled = rigidBody2DComponent["SetEnabled"].as<bool>();
				}

				auto boxCollider2DComponent = entity["BoxCollider2DComponent"];
				if (boxCollider2DComponent)
				{
					auto& component = deserializedEntity.AddComponent<BoxCollider2DComponent>();

					if (boxCollider2DComponent["Offset"])
						component.Offset = boxCollider2DComponent["Offset"].as<rtmcpp::Vec2>();

					if (boxCollider2DComponent["Size"])
						component.Size = boxCollider2DComponent["Size"].as<rtmcpp::Vec2>();

					component.Density = boxCollider2DComponent["Density"] ? boxCollider2DComponent["Density"].as<float>() : 1.0f;
					component.Friction = boxCollider2DComponent["Friction"] ? boxCollider2DComponent["Friction"].as<float>() : 1.0f;
					component.Restitution = boxCollider2DComponent["Restitution"] ? boxCollider2DComponent["Restitution"].as<float>() : 1.0f;
					component.RestitutionThreshold = boxCollider2DComponent["Threshold"] ? boxCollider2DComponent["Threshold"].as<float>() : 1.0f;
				}

				auto circleCollider2DComponent = entity["CircleCollider2DComponent"];
				if (circleCollider2DComponent)
				{
					auto& component = deserializedEntity.AddComponent<CircleCollider2DComponent>();

					if (circleCollider2DComponent["Offset"])
						component.Offset = circleCollider2DComponent["Offset"].as<rtmcpp::Vec2>();

					if (circleCollider2DComponent["Size"])
						component.Radius = circleCollider2DComponent["Radius"].as<float>();

					component.Density = circleCollider2DComponent["Density"] ? circleCollider2DComponent["Density"].as<float>() : 1.0f;
					component.Friction = circleCollider2DComponent["Friction"] ? circleCollider2DComponent["Friction"].as<float>() : 1.0f;
					component.Restitution = circleCollider2DComponent["Restitution"] ? circleCollider2DComponent["Restitution"].as<float>() : 1.0f;
					component.RestitutionThreshold = circleCollider2DComponent["Threshold"] ? circleCollider2DComponent["Threshold"].as<float>() : 1.0f;
				}

				auto triangleCollider2DComponent = entity["TriangleCollider2DComponent"];
				if (triangleCollider2DComponent)
				{
					auto& component = deserializedEntity.AddComponent<TriangleCollider2DComponent>();

					if (triangleCollider2DComponent["Offset"])
						component.Offset = triangleCollider2DComponent["Offset"].as<rtmcpp::Vec2>();

					if (triangleCollider2DComponent["Size"])
						component.Size = triangleCollider2DComponent["Size"].as<rtmcpp::Vec2>();

					component.Density = triangleCollider2DComponent["Density"] ? triangleCollider2DComponent["Density"].as<float>() : 1.0f;
					component.Friction = triangleCollider2DComponent["Friction"] ? triangleCollider2DComponent["Friction"].as<float>() : 1.0f;
					component.Restitution = triangleCollider2DComponent["Restitution"] ? triangleCollider2DComponent["Restitution"].as<float>() : 1.0f;
					component.RestitutionThreshold = triangleCollider2DComponent["Threshold"] ? triangleCollider2DComponent["Threshold"].as<float>() : 1.0f;
				}

				auto capsuleCollider2DComponent = entity["CapsuleCollider2DComponent"];
				if (capsuleCollider2DComponent)
				{
					auto& component = deserializedEntity.AddComponent<CapsuleCollider2DComponent>();

					if (capsuleCollider2DComponent["Offset"])
						component.Offset = capsuleCollider2DComponent["Offset"].as<rtmcpp::Vec2>();

					if (capsuleCollider2DComponent["Size"])
						component.Size = capsuleCollider2DComponent["Size"].as<rtmcpp::Vec2>();

					if (capsuleCollider2DComponent["Density"])
						component.Density = capsuleCollider2DComponent["Density"] ? capsuleCollider2DComponent["Density"].as<float>() : 1.0f;
					if (capsuleCollider2DComponent["Friction"])
						component.Friction = capsuleCollider2DComponent["Friction"] ? capsuleCollider2DComponent["Friction"].as<float>() : 1.0f;
					if (capsuleCollider2DComponent["Restitution"])
						component.Restitution = capsuleCollider2DComponent["Restitution"] ? capsuleCollider2DComponent["Restitution"].as<float>() : 1.0f;
					if (capsuleCollider2DComponent["Threshold"])
						component.RestitutionThreshold = capsuleCollider2DComponent["Threshold"] ? capsuleCollider2DComponent["Threshold"].as<float>() : 1.0f;
				}

				auto meshCollider2DComponent = entity["MeshCollider2DComponent"];
				if (meshCollider2DComponent)
				{
					auto& component = deserializedEntity.AddComponent<MeshCollider2DComponent>();

					if (meshCollider2DComponent["Offset"])
						component.Offset = meshCollider2DComponent["Offset"].as<rtmcpp::Vec2>();

					if (meshCollider2DComponent["Size"])
						component.Size = meshCollider2DComponent["Size"].as<rtmcpp::Vec2>();

					component.Density = meshCollider2DComponent["Density"] ? meshCollider2DComponent["Density"].as<float>() : 1.0f;
					component.Friction = meshCollider2DComponent["Friction"] ? meshCollider2DComponent["Friction"].as<float>() : 1.0f;
					component.Restitution = meshCollider2DComponent["Restitution"] ? meshCollider2DComponent["Restitution"].as<float>() : 1.0f;
					component.RestitutionThreshold = meshCollider2DComponent["Threshold"] ? meshCollider2DComponent["Threshold"].as<float>() : 1.0f;

					if (meshCollider2DComponent["NumberOfPositions"])
						component.NumberOfPositions = meshCollider2DComponent["NumberOfPositions"].as<int>();

					if (component.NumberOfPositions > 0)
					{
						for (uint32_t i = 0; i < component.NumberOfPositions; i++)
						{
							std::string positionName = "Position" + std::to_string(i);

							if (meshCollider2DComponent[positionName.c_str()])
							{
								rtmcpp::Vec2 position = meshCollider2DComponent[positionName.c_str()].as<rtmcpp::Vec2>();
								component.Positions.emplace_back(position);
							}
						}
					}
				}

				auto audioSourceComponent = entity["AudioSourceComponent"];
				if (audioSourceComponent)
				{
					auto& component = deserializedEntity.AddComponent<AudioSourceComponent>();

					if (audioSourceComponent["AudioHandle"])
						component.Audio = audioSourceComponent["AudioHandle"].as<AssetHandle>();

					if (audioSourceComponent["VolumeMultiplier"])
						component.Config.VolumeMultiplier = audioSourceComponent["VolumeMultiplier"].as<float>();

					if (audioSourceComponent["PitchMultiplier"])
						component.Config.PitchMultiplier = audioSourceComponent["PitchMultiplier"].as<float>();

					if (audioSourceComponent["PlayOnAwake"])
						component.Config.PlayOnAwake = audioSourceComponent["PlayOnAwake"].as<bool>();

					if (audioSourceComponent["Looping"])
						component.Config.Looping = audioSourceComponent["Looping"].as<bool>();

					if (audioSourceComponent["Spatialization"])
						component.Config.Spatialization = audioSourceComponent["Spatialization"].as<bool>();

					TrySetEnum(component.Config.AttenuationModel, audioSourceComponent["AttenuationModel"]);

					if (audioSourceComponent["RollOff"])
						component.Config.RollOff = audioSourceComponent["RollOff"].as<float>();

					if (audioSourceComponent["MinGain"])
						component.Config.MinGain = audioSourceComponent["MinGain"].as<float>();

					if (audioSourceComponent["MaxGain"])
						component.Config.MaxGain = audioSourceComponent["MaxGain"].as<float>();

					if (audioSourceComponent["MinDistance"])
						component.Config.MinDistance = audioSourceComponent["MinDistance"].as<float>();

					if (audioSourceComponent["MaxDistance"])
						component.Config.MaxDistance = audioSourceComponent["MaxDistance"].as<float>();

					if (audioSourceComponent["ConeInnerAngle"])
						component.Config.ConeInnerAngle = audioSourceComponent["ConeInnerAngle"].as<float>();

					if (audioSourceComponent["ConeOuterAngle"])
						component.Config.ConeOuterAngle = audioSourceComponent["ConeOuterAngle"].as<float>();

					if (audioSourceComponent["ConeOuterGain"])
						component.Config.ConeOuterGain = audioSourceComponent["ConeOuterGain"].as<float>();

					if (audioSourceComponent["DopplerFactor"])
						component.Config.DopplerFactor = audioSourceComponent["DopplerFactor"].as<float>();

					if (component.Audio != 0)
					{
						RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
						audioSource->SetConfig(component.Config);
					}

					if (audioSourceComponent["UsePlaylist"])
						component.AudioSourceData.UsePlaylist = audioSourceComponent["UsePlaylist"].as<bool>();

					if (component.AudioSourceData.UsePlaylist)
					{
						if (audioSourceComponent["AudioSourcesSize"])
							component.AudioSourceData.NumberOfAudioSources = audioSourceComponent["AudioSourcesSize"].as<int>();

						if (audioSourceComponent["StartIndex"])
							component.AudioSourceData.StartIndex = audioSourceComponent["StartIndex"].as<int>();

						if (audioSourceComponent["RepeatPlaylist"])
							component.AudioSourceData.RepeatPlaylist = audioSourceComponent["RepeatPlaylist"].as<bool>();

						if (audioSourceComponent["RepeatSpecificTrack"])
							component.AudioSourceData.RepeatAfterSpecificTrackPlays = audioSourceComponent["RepeatSpecificTrack"].as<bool>();

						for (uint32_t i = 0; i < component.AudioSourceData.NumberOfAudioSources; i++)
						{
							std::string audioName = "AudioHandle" + std::to_string(i);
							if (audioSourceComponent[audioName.c_str()])
							{
								AssetHandle audioHandle = audioSourceComponent[audioName.c_str()].as<AssetHandle>();
								component.AudioSourceData.Playlist.emplace_back(audioHandle);

							}
						}
					}
				}

				auto audioListenerComponent = entity["AudioListenerComponent"];
				if (audioListenerComponent)
				{
					auto& component = deserializedEntity.AddComponent<AudioListenerComponent>();

					if (audioListenerComponent["Active"])
						component.Active = audioListenerComponent["Active"].as<bool>();

					if (audioListenerComponent["ConeInnerAngle"])
						component.Config.ConeInnerAngle = audioListenerComponent["ConeInnerAngle"].as<float>();

					if (audioListenerComponent["ConeOuterAngle"])
						component.Config.ConeOuterAngle = audioListenerComponent["ConeOuterAngle"].as<float>();

					if (audioListenerComponent["ConeOuterGain"])
						component.Config.ConeOuterGain = audioListenerComponent["ConeOuterGain"].as<float>();
				}

				auto videoRendererComponent = entity["VideoRendererComponent"];
				if (videoRendererComponent)
				{
					auto& src = deserializedEntity.AddComponent<VideoRendererComponent>();

					src.Color = videoRendererComponent["Color"].as<rtmcpp::Vec4>();

					if (videoRendererComponent["RepeatVideo"])
						src.m_VideoData.RepeatVideo = videoRendererComponent["RepeatVideo"].as<bool>();

					if (videoRendererComponent["VideoHandle"])
						src.Video = videoRendererComponent["VideoHandle"].as<AssetHandle>();

					if (videoRendererComponent["UseExternalAudio"])
						src.m_VideoData.UseExternalAudio = videoRendererComponent["UseExternalAudio"].as<bool>();

					if (videoRendererComponent["Volume"])
					{
						float volume = videoRendererComponent["Volume"].as<float>();
						if (src.Video)
							src.m_VideoData.Volume = volume;
					}
				}
			}
		}

		NZ_CORE_WARN("YAML Deserialization took {0} seconds", timer.ElapsedMicros());

		return true;
	}	

	bool SceneSerializer::DeserializeJSON(const std::filesystem::path& filepath)
	{
		Timer timer;

		// Read JSON file, allowing comments and trailing commas
		yyjson_read_flag flg = YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS | /*YYJSON_READ_INSITU |*/ YYJSON_READ_STOP_WHEN_DONE;
		yyjson_read_err err;
		yyjson_doc* doc = yyjson_read_file(filepath.string().c_str(), flg, NULL, &err);
		yyjson_val* root = yyjson_doc_get_root(doc);

		uint64_t numberOfEntities = 0;

		// Iterate over the root object
		if (doc)
		{
			yyjson_val* obj = yyjson_doc_get_root(doc);
			yyjson_obj_iter iter;
			yyjson_obj_iter_init(obj, &iter);
			yyjson_val* key, * val;

			std::string componentType = "";
			std::string componentName = "";
			
			{
				key = yyjson_obj_iter_next(&iter);

				if (key)
				{
					val = yyjson_obj_iter_get_val(key);

					std::string sceneName = yyjson_get_str(val);

					//if (sceneName.empty())
					//	return false;

					if (!sceneName.empty())
						m_Scene->SetName(sceneName);

					float gravityX = 0.0f; 
					float gravityY = 0.0f;

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					gravityX = (float)yyjson_get_real(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					gravityY = (float)yyjson_get_real(val);

					m_Scene->SetPhysics2DGravity(rtmcpp::Vec2(gravityX, gravityY));

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);
					numberOfEntities = (uint64_t)yyjson_get_uint(val);

					key = yyjson_obj_iter_next(&iter);
					val = yyjson_obj_iter_get_val(key);

					for (uint64_t i = 0; i < numberOfEntities; i++)
					{
						uint64_t id = (uint64_t)yyjson_get_uint(val);

						key = yyjson_obj_iter_next(&iter);
						val = yyjson_obj_iter_get_val(key);
						std::string tagComponentName = yyjson_get_str(val);

						key = yyjson_obj_iter_next(&iter);
						val = yyjson_obj_iter_get_val(key);;

						std::string name;
						name = yyjson_get_str(val);

						//NZ_CORE_TRACE("Deserialized entity with ID = {0}, name = {1}", id, name.c_str());

						Entity deserializedEntity = m_Scene->CreateEntityWithID(id, name);

						key = yyjson_obj_iter_next(&iter);
						val = yyjson_obj_iter_get_val(key);

						// TransformComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "TransformComponent")
								{
									// Entities always have transforms
									auto& tc = deserializedEntity.GetComponent<TransformComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									tc.Enabled = yyjson_get_bool(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									std::string translationStr = yyjson_get_str(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float xt = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float yt = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float zt = (float)yyjson_get_real(val);

									tc.Translation = rtmcpp::Vec4(xt, yt, zt, 1.0f);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);

									std::string rotationStr = yyjson_get_str(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float xr = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float yr = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float zr = (float)yyjson_get_real(val);

									tc.Rotation = rtmcpp::Vec3(xr, yr, zr);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									std::string scaleStr = yyjson_get_str(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float xs = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float ys = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float zs = (float)yyjson_get_real(val);

									tc.Scale = rtmcpp::Vec3(xs, ys, zs);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						//CameraComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "CameraComponent")
								{
									auto& cc = deserializedEntity.AddComponent<CameraComponent>();
									cc.Camera = RefPtr<SceneCamera>::Create();
									auto& cameraProps = cc.Camera;

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									cameraProps->SetProjectionType((SceneCamera::ProjectionType)yyjson_get_int(val));

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									cameraProps->SetPerspectiveVerticalFOV((float)yyjson_get_real(val));

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									cameraProps->SetPerspectiveNearClip((float)yyjson_get_real(val));

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									cameraProps->SetPerspectiveFarClip((float)yyjson_get_real(val));

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									cameraProps->SetOrthographicSize((float)yyjson_get_real(val));

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									cameraProps->SetOrthographicNearClip((float)yyjson_get_real(val));

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									cameraProps->SetOrthographicFarClip((float)yyjson_get_real(val));

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									cc.Primary = yyjson_get_bool(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									cc.FixedAspectRatio = yyjson_get_bool(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// ScriptComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "ScriptComponent")
								{
									// ScriptHandle
									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									uint64_t scriptID = yyjson_get_uint(val);

									if (scriptID == 0)
									{
										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										scriptID = yyjson_get_uint(val);

										NZ_CORE_VERIFY(scriptID == 0);
									}

									{
										auto& scriptEngine = ScriptEngine::GetMutable();

										if (scriptEngine.IsValidScript(scriptID))
										{
											const auto& scriptMetadata = scriptEngine.GetScriptMetadata(scriptID);

											ScriptComponent& sc = deserializedEntity.AddComponent<ScriptComponent>();
											sc.ScriptHandle = scriptID;

											m_Scene->m_ScriptStorage.InitializeEntityStorage(sc.ScriptHandle, deserializedEntity.GetEntityHandle());

											//bool oldFormat = false;

											// ScriptName
											key = yyjson_obj_iter_next(&iter);
											val = yyjson_obj_iter_get_val(key);
											auto scriptName = yyjson_get_str(val);

											// Fields
											key = yyjson_obj_iter_next(&iter);
											val = yyjson_obj_iter_get_val(key);
											auto fieldsArray = yyjson_get_uint(val);

											if (fieldsArray)
											{
												for (uint32_t i = 0; i < fieldsArray; i++)
												{
													// ID // This might cause a bug, when serializing a scene after saving scene, closing scene and reopen scene!
													key = yyjson_obj_iter_next(&iter);
													val = yyjson_obj_iter_get_val(key);
													uint32_t fieldID = (uint32_t)yyjson_get_uint(val);

													key = yyjson_obj_iter_next(&iter);
													val = yyjson_obj_iter_get_val(key);
													std::string fieldName = yyjson_get_str(val);

													key = yyjson_obj_iter_next(&iter);
													val = yyjson_obj_iter_get_val(key);
													std::string fieldType = yyjson_get_str(val);

													//if (oldFormat && fieldName.find(':') != std::string::npos)
													//{
													//	// Old format, try generating id from name
													//	fieldName = fieldName.substr(fieldName.find(':') + 1);
													//	fieldID = Hash::GenerateFNVHash(fieldName);
													//}

													if (scriptMetadata.Fields.contains(fieldID))
													{
														key = yyjson_obj_iter_next(&iter);
														val = yyjson_obj_iter_get_val(key);

														const auto& fieldMetadata = scriptMetadata.Fields.at(fieldID);
														auto& fieldStorage = m_Scene->m_ScriptStorage.EntityStorage.at(deserializedEntity.GetEntityHandle()).Fields[fieldID];

														switch (fieldMetadata.Type)
														{
														case DataType::SByte:
														{
															int8_t value = (int8_t)yyjson_get_int(val);
															fieldStorage.SetValue(value);
															break;
														}
														case DataType::Byte:
														{
															uint8_t value = (uint8_t)yyjson_get_uint(val);
															fieldStorage.SetValue(value);
															break;
														}
														case DataType::Short:
														{
															int16_t value = (int16_t)yyjson_get_int(val);
															fieldStorage.SetValue(value);
															break;
														}
														case DataType::UShort:
														{
															uint16_t value = (uint16_t)yyjson_get_uint(val);
															fieldStorage.SetValue(value);
															break;
														}
														case DataType::Int:
														{
															int32_t value = (int32_t)yyjson_get_int(val);
															fieldStorage.SetValue(value);
															break;
														}
														case DataType::UInt:
														{
															uint32_t value = (uint32_t)yyjson_get_uint(val);
															fieldStorage.SetValue(value);
															break;
														}
														case DataType::Long:
														{
															int64_t value = yyjson_get_int(val);
															fieldStorage.SetValue(value);
															break;
														}
														case DataType::ULong:
														{
															uint64_t value = yyjson_get_uint(val);
															fieldStorage.SetValue(value);
															break;
														}
														case DataType::Float:
														{
															float value = (float)yyjson_get_real(val);
															fieldStorage.SetValue(value);
															break;
														}
														case DataType::Double:
														{
															double value = yyjson_get_real(val);
															fieldStorage.SetValue(value);
															break;
														}
														case DataType::Bool:
														{
															uint32_t value = (uint32_t)yyjson_get_bool(val);
															fieldStorage.SetValue(value);
															break;
														}
														case DataType::String:
														{
															std::string value = (std::string)yyjson_get_str(val);
															fieldStorage.SetValue(Coral::String::New(value));
															break;
														}
														case DataType::Bool32:
														{
															uint32_t value = (uint32_t)yyjson_get_bool(val);
															fieldStorage.SetValue(value);
															break;
														}
														case DataType::AssetHandle:
														{
															uint64_t value = yyjson_get_uint(val);
															fieldStorage.SetValue(value);
															break;
														}
														case DataType::Vector2:
														{
															float x = (float)yyjson_get_real(val);

															key = yyjson_obj_iter_next(&iter);
															val = yyjson_obj_iter_get_val(key);
															float y = (float)yyjson_get_real(val);

															fieldStorage.SetValue(rtmcpp::Vec2{ x, y });
															break;
														}
														case DataType::Vector3:
														{
															float x = (float)yyjson_get_real(val);

															key = yyjson_obj_iter_next(&iter);
															val = yyjson_obj_iter_get_val(key);
															float y = (float)yyjson_get_real(val);

															key = yyjson_obj_iter_next(&iter);
															val = yyjson_obj_iter_get_val(key);
															float z = (float)yyjson_get_real(val);

															fieldStorage.SetValue(rtmcpp::Vec3{ x, y, z });
															break;
														}
														case DataType::Vector4:
														{
															float x = (float)yyjson_get_real(val);

															key = yyjson_obj_iter_next(&iter);
															val = yyjson_obj_iter_get_val(key);
															float y = (float)yyjson_get_real(val);

															key = yyjson_obj_iter_next(&iter);
															val = yyjson_obj_iter_get_val(key);
															float z = (float)yyjson_get_real(val);

															key = yyjson_obj_iter_next(&iter);
															val = yyjson_obj_iter_get_val(key);
															float w = (float)yyjson_get_real(val);

															fieldStorage.SetValue(rtmcpp::Vec4{ x, y, z, w });
															break;
														}
														//case DataType::Entity:
														//{
														//	fieldStorage.SetValue(valueNode.as<uint64_t>());
														//	break;
														//}
														//case DataType::Prefab:
														//{
														//	fieldStorage.SetValue(valueNode.as<uint64_t>());
														//	break;
														//}
														//case DataType::Mesh:
														//{
														//	fieldStorage.SetValue(valueNode.as<uint64_t>());
														//	break;
														//}
														//case DataType::StaticMesh:
														//{
														//	fieldStorage.SetValue(valueNode.as<uint64_t>());
														//	break;
														//}
														//case DataType::Material:
														//{
														//	fieldStorage.SetValue(valueNode.as<uint64_t>());
														//	break;
														//}
														//case DataType::Texture2D:
														//{
														//	fieldStorage.SetValue(valueNode.as<uint64_t>());
														//	break;
														//}
														//case DataType::Scene:
														//{
														//	fieldStorage.SetValue(valueNode.as<uint64_t>());
														//	break;
														//}
														default:
															break;
														}
													}
												}
											}

											sc.HasInitializedScript = true;

											key = yyjson_obj_iter_next(&iter);
											val = yyjson_obj_iter_get_val(key);
										}
									}
								}
							}
						}

						// SpriteRendererComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "SpriteRendererComponent")
								{
									auto& src = deserializedEntity.AddComponent<SpriteRendererComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorY = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorZ = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorW = (float)yyjson_get_real(val);

									src.Color = rtmcpp::Vec4(colorX, colorY, colorZ, colorW);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float uvX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float uvY = (float)yyjson_get_real(val);

									src.UV = rtmcpp::Vec2(uvX, uvY);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.TextureHandle = (AssetHandle)yyjson_get_uint(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.m_AnimationData.UseParallaxScrolling = yyjson_get_bool(val);

									if (src.m_AnimationData.UseParallaxScrolling)
									{
										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										src.m_AnimationData.UseCameraParallaxX = yyjson_get_bool(val);

										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										src.m_AnimationData.UseCameraParallaxY = yyjson_get_bool(val);

										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										float parallaxSpeedX = (float)yyjson_get_real(val);

										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										float parallaxSpeedY = (float)yyjson_get_real(val);

										src.m_AnimationData.ParallaxSpeed = rtmcpp::Vec2(parallaxSpeedX, parallaxSpeedY);

										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										src.m_AnimationData.ParallaxDivision = (float)yyjson_get_real(val);
									}

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.m_AnimationData.UseTextureAtlasAnimation = yyjson_get_bool(val);

									if (src.m_AnimationData.UseTextureAtlasAnimation)
									{
										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										src.m_AnimationData.AnimationSpeed = (float)yyjson_get_real(val);
									}

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.m_AnimationData.NumberOfTiles = yyjson_get_int(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									int startIndexX = yyjson_get_int(val);
									src.m_AnimationData.StartIndexX = startIndexX;

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									int startIndexY = yyjson_get_int(val);
									src.m_AnimationData.StartIndexY = startIndexY;

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									int rowsAndColumnsX = yyjson_get_int(val);
									src.m_AnimationData.Rows = rowsAndColumnsX;

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									int rowsAndColumnsY = yyjson_get_int(val);
									src.m_AnimationData.Columns = rowsAndColumnsY;

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.m_AnimationData.UseLinear = yyjson_get_bool(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.m_AnimationData.UsePerTextureAnimation = yyjson_get_bool(val);

									if (src.m_AnimationData.UsePerTextureAnimation)
									{
										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										src.m_AnimationData.StartIndex = yyjson_get_int(val);

										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										src.m_AnimationData.NumberOfTextures = (uint32_t)yyjson_get_uint(val);

										for (uint32_t i = 0; i < src.m_AnimationData.NumberOfTextures; i++)
										{
											{
												key = yyjson_obj_iter_next(&iter);
												val = yyjson_obj_iter_get_val(key);

												AssetHandle textureHandle = (AssetHandle)yyjson_get_uint(val);
												src.m_AnimationData.Textures.emplace_back(textureHandle);
											}
										}
									}

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// CircleRendererComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "CircleRendererComponent")
								{
									auto& src = deserializedEntity.AddComponent<CircleRendererComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorY = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorZ = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorW = (float)yyjson_get_real(val);

									src.Color = rtmcpp::Vec4(colorX, colorY, colorZ, colorW);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float uvX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float uvY = (float)yyjson_get_real(val);

									src.UV = rtmcpp::Vec2(uvX, uvY);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.Thickness = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.Fade = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.MakeSolid = yyjson_get_bool(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.TextureHandle = (AssetHandle)yyjson_get_uint(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.m_AnimationData.UseParallaxScrolling = yyjson_get_bool(val);

									if (src.m_AnimationData.UseParallaxScrolling)
									{
										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										src.m_AnimationData.UseCameraParallaxX = yyjson_get_bool(val);

										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										src.m_AnimationData.UseCameraParallaxY = yyjson_get_bool(val);

										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										float parallaxSpeedX = (float)yyjson_get_real(val);

										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										float parallaxSpeedY = (float)yyjson_get_real(val);

										src.m_AnimationData.ParallaxSpeed = rtmcpp::Vec2(parallaxSpeedX, parallaxSpeedY);

										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										src.m_AnimationData.ParallaxDivision = (float)yyjson_get_real(val);
									}

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.m_AnimationData.UseTextureAtlasAnimation = yyjson_get_bool(val);

									if (src.m_AnimationData.UseTextureAtlasAnimation)
									{
										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										src.m_AnimationData.AnimationSpeed = (float)yyjson_get_real(val);
									}

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.m_AnimationData.NumberOfTiles = yyjson_get_int(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									int startIndexX = yyjson_get_int(val);
									src.m_AnimationData.StartIndexX = startIndexX;

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									int startIndexY = yyjson_get_int(val);
									src.m_AnimationData.StartIndexY = startIndexY;

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									int rowsAndColumnsX = yyjson_get_int(val);
									src.m_AnimationData.Rows = rowsAndColumnsX;

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									int rowsAndColumnsY = yyjson_get_int(val);
									src.m_AnimationData.Columns = rowsAndColumnsY;

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.m_AnimationData.UseLinear = yyjson_get_bool(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// TriangleRendererComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "TriangleRendererComponent")
								{
									auto& src = deserializedEntity.AddComponent<TriangleRendererComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorY = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorZ = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorW = (float)yyjson_get_real(val);

									src.Color = rtmcpp::Vec4(colorX, colorY, colorZ, colorW);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float uvX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float uvY = (float)yyjson_get_real(val);

									src.UV = rtmcpp::Vec2(uvX, uvY);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.TextureHandle = (AssetHandle)yyjson_get_uint(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// LineRendererComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "LineRendererComponent")
								{
									auto& src = deserializedEntity.AddComponent<LineRendererComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.LineThickness = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.NumberOfColors = (uint32_t)yyjson_get_uint(val);
									src.AddColor(rtmcpp::Vec4(1.0f, 1.0f, 1.0f, 1.0f));

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.NumberOfTranslations = (uint32_t)yyjson_get_uint(val);

									uint32_t numberOfTranslations = src.NumberOfTranslations;

									for (uint32_t i = 0; i < numberOfTranslations; i++)
									{
										if (i >= 1)
										{
											{
												key = yyjson_obj_iter_next(&iter);
												val = yyjson_obj_iter_get_val(key);
												float colorX = (float)yyjson_get_real(val);

												key = yyjson_obj_iter_next(&iter);
												val = yyjson_obj_iter_get_val(key);
												float colorY = (float)yyjson_get_real(val);

												key = yyjson_obj_iter_next(&iter);
												val = yyjson_obj_iter_get_val(key);
												float colorZ = (float)yyjson_get_real(val);

												key = yyjson_obj_iter_next(&iter);
												val = yyjson_obj_iter_get_val(key);
												float colorW = (float)yyjson_get_real(val);

												src.AddColor(rtmcpp::Vec4(colorX, colorY, colorZ, colorW));
											}
										}

										{
											key = yyjson_obj_iter_next(&iter);
											val = yyjson_obj_iter_get_val(key);
											float translationX = (float)yyjson_get_real(val);

											key = yyjson_obj_iter_next(&iter);
											val = yyjson_obj_iter_get_val(key);
											float translationY = (float)yyjson_get_real(val);

											key = yyjson_obj_iter_next(&iter);
											val = yyjson_obj_iter_get_val(key);
											float translationZ = (float)yyjson_get_real(val);

											src.AddTranslation(rtmcpp::Vec4(translationX, translationY, translationZ, 1.0f));
										}
									}

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// TextComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "TextComponent")
								{
									auto& src = deserializedEntity.AddComponent<TextComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.FontHandle = (AssetHandle)yyjson_get_uint(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.TextString = yyjson_get_str(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorY = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorZ = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorW = (float)yyjson_get_real(val);

									src.Color = rtmcpp::Vec4(colorX, colorY, colorZ, colorW);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.UseLinear = yyjson_get_bool(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.LineSpacing = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.Kerning = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// ParticleSystemComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "ParticleSystemComponent")
								{
									auto& src = deserializedEntity.AddComponent<ParticleSystemComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float velX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float velY = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float velZ = (float)yyjson_get_real(val);

									src.Velocity = rtmcpp::Vec3(velX, velY, velZ);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float velVarX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float velVarY = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float velVarZ = (float)yyjson_get_real(val);

									src.VelocityVariation = rtmcpp::Vec3(velVarX, velVarY, velVarZ);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colBeginX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colBeginY = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colBeginZ = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colBeginW = (float)yyjson_get_real(val);

									src.ColorBegin = rtmcpp::Vec4(colBeginX, colBeginY, colBeginZ, colBeginW);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colEndX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colEndY = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colEndZ = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colEndW = (float)yyjson_get_real(val);

									src.ColorEnd = rtmcpp::Vec4(colEndX, colEndY, colEndZ, colEndW);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);

									src.TextureHandle = (AssetHandle)yyjson_get_uint(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);

									src.SizeBegin = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);

									src.SizeEnd = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);

									src.SizeVariation = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);

									src.LifeTime = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);

									src.ParticleSize = yyjson_get_int(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);

									src.UseLinear = yyjson_get_bool(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);

									src.UseBillboard = yyjson_get_bool(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// CubeRendererComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "CubeRendererComponent")
								{
									auto& src = deserializedEntity.AddComponent<CubeRendererComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorY = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorZ = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorW = (float)yyjson_get_real(val);

									src.Color = rtmcpp::Vec4(colorX, colorY, colorZ, colorW);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float uvX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float uvY = (float)yyjson_get_real(val);

									src.UV = rtmcpp::Vec2(uvX, uvY);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.TextureHandle = (AssetHandle)yyjson_get_uint(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.m_AnimationData.UseParallaxScrolling = yyjson_get_bool(val);

									if (src.m_AnimationData.UseParallaxScrolling)
									{
										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										src.m_AnimationData.UseCameraParallaxX = yyjson_get_bool(val);

										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										src.m_AnimationData.UseCameraParallaxY = yyjson_get_bool(val);

										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										float parallaxSpeedX = (float)yyjson_get_real(val);

										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										float parallaxSpeedY = (float)yyjson_get_real(val);

										src.m_AnimationData.ParallaxSpeed = rtmcpp::Vec2(parallaxSpeedX, parallaxSpeedY);

										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										src.m_AnimationData.ParallaxDivision = (float)yyjson_get_real(val);
									}

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.m_AnimationData.UseTextureAtlasAnimation = yyjson_get_bool(val);

									if (src.m_AnimationData.UseTextureAtlasAnimation)
									{
										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										src.m_AnimationData.AnimationSpeed = (float)yyjson_get_real(val);
									}

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.m_AnimationData.NumberOfTiles = yyjson_get_int(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									int startIndexX = yyjson_get_int(val);
									src.m_AnimationData.StartIndexX = startIndexX;

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									int startIndexY = yyjson_get_int(val);
									src.m_AnimationData.StartIndexY = startIndexY;

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									int rowsAndColumnsX = yyjson_get_int(val);
									src.m_AnimationData.Rows = rowsAndColumnsX;

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									int rowsAndColumnsY = yyjson_get_int(val);
									src.m_AnimationData.Columns = rowsAndColumnsY;

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.m_AnimationData.UseLinear = yyjson_get_bool(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.m_AnimationData.UsePerTextureAnimation = yyjson_get_bool(val);

									if (src.m_AnimationData.UsePerTextureAnimation)
									{
										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										src.m_AnimationData.StartIndex = yyjson_get_int(val);

										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										src.m_AnimationData.NumberOfTextures = yyjson_get_int(val);

										for (uint32_t i = 0; i < src.m_AnimationData.NumberOfTextures; i++)
										{
											{
												key = yyjson_obj_iter_next(&iter);
												val = yyjson_obj_iter_get_val(key);

												AssetHandle textureHandle = yyjson_get_uint(val);
												src.m_AnimationData.Textures.emplace_back(textureHandle);

											}
										}
									}

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// PyramidRendererComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "PyramidRendererComponent")
								{
									auto& src = deserializedEntity.AddComponent<PyramidRendererComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorY = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorZ = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorW = (float)yyjson_get_real(val);

									src.Color = rtmcpp::Vec4(colorX, colorY, colorZ, colorW);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float uvX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float uvY = (float)yyjson_get_real(val);

									src.UV = rtmcpp::Vec2(uvX, uvY);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.TextureHandle = (AssetHandle)yyjson_get_uint(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// TriangularPrismRendererComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "TriangularPrismRendererComponent")
								{
									auto& src = deserializedEntity.AddComponent<TriangularPrismRendererComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorY = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorZ = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorW = (float)yyjson_get_real(val);

									src.Color = rtmcpp::Vec4(colorX, colorY, colorZ, colorW);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float uvX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float uvY = (float)yyjson_get_real(val);

									src.UV = rtmcpp::Vec2(uvX, uvY);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.TextureHandle = (AssetHandle)yyjson_get_uint(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// PlaneRendererComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "PlaneRendererComponent")
								{
									auto& src = deserializedEntity.AddComponent<PlaneRendererComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorY = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorZ = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorW = (float)yyjson_get_real(val);

									src.Color = rtmcpp::Vec4(colorX, colorY, colorZ, colorW);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float uvX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float uvY = (float)yyjson_get_real(val);

									src.UV = rtmcpp::Vec2(uvX, uvY);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.TextureHandle = (AssetHandle)yyjson_get_uint(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// OBJRendererComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "OBJRendererComponent")
								{
									auto& src = deserializedEntity.AddComponent<OBJRendererComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorY = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorZ = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorW = (float)yyjson_get_real(val);

									src.Color = rtmcpp::Vec4(colorX, colorY, colorZ, colorW);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float uvX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float uvY = (float)yyjson_get_real(val);

									src.UV = rtmcpp::Vec2(uvX, uvY);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.ModelHandle = (AssetHandle)yyjson_get_uint(val);

									//if (objRendererComponent["FilePath"])
									//{
									//	std::filesystem::path path = objRendererComponent["FilePath"].as<std::string>();
									//	std::filesystem::path absoluteFilePath = std::filesystem::absolute(path).string();
									//
									//	if (absoluteFilePath.extension() == ".obj")
									//	{
									//		if (Renderer3D::LoadTinyObj(absoluteFilePath, src))
									//		{
									//			src.Filepath = path.string();
									//			src.HasLoadedFile = true;
									//
									//			NZ_CORE_INFO("Successfully loaded obj file: {0}", absoluteFilePath);
									//		}
									//		else
									//		{
									//			NZ_CONSOLE_LOG_CORE_WARN("Could not load file: %s", absoluteFilePath.filename().string());
									//		}
									//	}
									//}

									//if (objRendererComponent["TexturePath"])
									//{
									//	std::filesystem::path path = objRendererComponent["TexturePath"].as<std::string>();
									//	std::string texturePath = std::filesystem::absolute(path).string();
									//	auto finalPath = Project::GetAssetFileSystemPath(texturePath);
									//	src.Texture = Texture2D::Create(finalPath.string());
									//}

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// ButtonWidgetComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "ButtonWidgetComponent")
								{
									auto& src = deserializedEntity.AddComponent<ButtonWidgetComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorY = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorZ = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorW = (float)yyjson_get_real(val);

									src.Color = rtmcpp::Vec4(colorX, colorY, colorZ, colorW);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float uvX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float uvY = (float)yyjson_get_real(val);

									src.UV = rtmcpp::Vec2(uvX, uvY);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.TextureHandle = (AssetHandle)yyjson_get_uint(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.Radius = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float dimensionX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float dimensionY = (float)yyjson_get_real(val);

									src.Dimensions = rtmcpp::Vec2(dimensionX, dimensionY);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.InvertCorners = yyjson_get_bool(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.UseLinear = yyjson_get_bool(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// CircleWidgetComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "CircleWidgetComponent")
								{
									auto& src = deserializedEntity.AddComponent<CircleWidgetComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorY = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorZ = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorW = (float)yyjson_get_real(val);

									src.Color = rtmcpp::Vec4(colorX, colorY, colorZ, colorW);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float uvX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float uvY = (float)yyjson_get_real(val);

									src.UV = rtmcpp::Vec2(uvX, uvY);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.TextureHandle = (AssetHandle)yyjson_get_uint(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.Fade = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.UseLinear = yyjson_get_bool(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// Rigidbody2DComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "Rigidbody2DComponent")
								{
									auto& component = deserializedEntity.AddComponent<Rigidbody2DComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Type = (Rigidbody2DComponent::BodyType)yyjson_get_int(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.FixedRotation = yyjson_get_bool(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.SetEnabled = yyjson_get_bool(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// BoxCollider2DComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "BoxCollider2DComponent")
								{
									auto& component = deserializedEntity.AddComponent<BoxCollider2DComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float offsetX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float offsetY = (float)yyjson_get_real(val);

									component.Offset = rtmcpp::Vec2(offsetX, offsetY);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float sizeX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float sizeY = (float)yyjson_get_real(val);

									component.Size = rtmcpp::Vec2(sizeX, sizeY);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Density = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Friction = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Restitution = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.RestitutionThreshold = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// CircleCollider2DComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "CircleCollider2DComponent")
								{
									auto& component = deserializedEntity.AddComponent<CircleCollider2DComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float offsetX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float offsetY = (float)yyjson_get_real(val);

									component.Offset = rtmcpp::Vec2(offsetX, offsetY);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Radius = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Density = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Friction = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Restitution = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.RestitutionThreshold = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// TriangleCollider2DComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "TriangleCollider2DComponent")
								{
									auto& component = deserializedEntity.AddComponent<TriangleCollider2DComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float offsetX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float offsetY = (float)yyjson_get_real(val);

									component.Offset = rtmcpp::Vec2(offsetX, offsetY);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float sizeX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float sizeY = (float)yyjson_get_real(val);

									component.Size = rtmcpp::Vec2(sizeX, sizeY);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Density = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Friction = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Restitution = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.RestitutionThreshold = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// CapsuleCollider2DComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "CapsuleCollider2DComponent")
								{
									auto& component = deserializedEntity.AddComponent<CapsuleCollider2DComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float offsetX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float offsetY = (float)yyjson_get_real(val);

									component.Offset = rtmcpp::Vec2(offsetX, offsetY);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float sizeX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float sizeY = (float)yyjson_get_real(val);

									component.Size = rtmcpp::Vec2(sizeX, sizeY);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Density = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Friction = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Restitution = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.RestitutionThreshold = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// MeshCollider2DComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "MeshCollider2DComponent")
								{
									auto& component = deserializedEntity.AddComponent<MeshCollider2DComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float offsetX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float offsetY = (float)yyjson_get_real(val);

									component.Offset = rtmcpp::Vec2(offsetX, offsetY);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float sizeX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float sizeY = (float)yyjson_get_real(val);

									component.Size = rtmcpp::Vec2(sizeX, sizeY);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Density = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Friction = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Restitution = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.RestitutionThreshold = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.NumberOfPositions = yyjson_get_int(val);

									if (component.NumberOfPositions > 0)
									{
										for (uint32_t i = 0; i < component.NumberOfPositions; i++)
										{
											{
												key = yyjson_obj_iter_next(&iter);
												val = yyjson_obj_iter_get_val(key);
												float positionX = (float)yyjson_get_real(val);

												key = yyjson_obj_iter_next(&iter);
												val = yyjson_obj_iter_get_val(key);
												float positionY = (float)yyjson_get_real(val);

												component.Positions.emplace_back(rtmcpp::Vec2(positionX, positionY));
											}
										}
									}


									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// AudioSourceComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "AudioSourceComponent")
								{
									auto& component = deserializedEntity.AddComponent<AudioSourceComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Audio = (AssetHandle)yyjson_get_uint(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Config.VolumeMultiplier = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Config.PitchMultiplier = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Config.PlayOnAwake = yyjson_get_bool(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Config.Looping = yyjson_get_bool(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Config.Spatialization = yyjson_get_bool(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Config.AttenuationModel = (AttenuationModelType)static_cast<int>(yyjson_get_int(val));

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Config.RollOff = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Config.MinGain = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Config.MaxGain = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Config.MinDistance = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Config.MaxDistance = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Config.ConeInnerAngle = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Config.ConeOuterAngle = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Config.ConeOuterGain = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Config.DopplerFactor = (float)yyjson_get_real(val);

									if (component.Audio != 0)
									{
										RefPtr<AudioSource> audioSource = AssetManager::GetAsset<AudioSource>(component.Audio);
										audioSource->SetConfig(component.Config);
									}

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.AudioSourceData.UsePlaylist = yyjson_get_bool(val);

									if (component.AudioSourceData.UsePlaylist)
									{
										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										component.AudioSourceData.NumberOfAudioSources = yyjson_get_int(val);

										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										component.AudioSourceData.StartIndex = yyjson_get_int(val);

										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										component.AudioSourceData.RepeatPlaylist = yyjson_get_bool(val);

										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										component.AudioSourceData.RepeatAfterSpecificTrackPlays = yyjson_get_bool(val);

										for (uint32_t i = 0; i < component.AudioSourceData.NumberOfAudioSources; i++)
										{
											{
												key = yyjson_obj_iter_next(&iter);
												val = yyjson_obj_iter_get_val(key);

												AssetHandle audioHandle = yyjson_get_uint(val);
												component.AudioSourceData.Playlist.emplace_back(audioHandle);
											}
										}
									}

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// AudioListenerComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "AudioListenerComponent")
								{
									auto& component = deserializedEntity.AddComponent<AudioListenerComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Active = yyjson_get_bool(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Config.ConeInnerAngle = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Config.ConeOuterAngle = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									component.Config.ConeOuterGain = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}

						// VideoRendererComponent
						{
							componentType = yyjson_get_type_desc(val);

							if (componentType == "string")
							{
								componentName = yyjson_get_str(val);

								if (componentName == "VideoRendererComponent")
								{
									auto& src = deserializedEntity.AddComponent<VideoRendererComponent>();

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorX = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorY = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorZ = (float)yyjson_get_real(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									float colorW = (float)yyjson_get_real(val);

									src.Color = rtmcpp::Vec4(colorX, colorY, colorZ, colorW);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.m_VideoData.RepeatVideo = yyjson_get_bool(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.Video = (AssetHandle)yyjson_get_uint(val);

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
									src.m_VideoData.UseExternalAudio = yyjson_get_bool(val);

									{
										key = yyjson_obj_iter_next(&iter);
										val = yyjson_obj_iter_get_val(key);
										float volume = (float)yyjson_get_real(val);

										if (src.Video)
											src.m_VideoData.Volume = volume;
									}

									key = yyjson_obj_iter_next(&iter);
									val = yyjson_obj_iter_get_val(key);
								}
							}
						}
					}
				}
			}
		}
		else
		{
			NZ_CORE_WARN("Read error ({0}): {1} at position: {2}", err.code, err.msg, err.pos);
		}

		// Free the doc
		yyjson_doc_free(doc);

		NZ_CORE_WARN("JSON (Scene file) Deserialization took {0} seconds", timer.ElapsedMicros());

		return true;
	}

	bool SceneSerializer::DeserializeRuntime(const std::filesystem::path& filepath)
	{
		// Not implemented
		NZ_CORE_ASSERT(false);
		return false;
	}
#pragma endregion

}
