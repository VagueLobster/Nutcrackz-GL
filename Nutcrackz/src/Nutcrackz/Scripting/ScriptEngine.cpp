#include "nzpch.hpp"
#include "ScriptEngine.hpp"
#include "ScriptGlue.hpp"

#include "Nutcrackz/Core/Application.hpp"
#include "Nutcrackz/Core/Hash.hpp"
#include "Nutcrackz/Project/Project.hpp"
#include "Nutcrackz/Asset/AssetManager.hpp"

#include <Coral/HostInstance.hpp>
#include <Coral/StringHelper.hpp>
#include <Coral/Attribute.hpp>
#include <Coral/TypeCache.hpp>
#include <Coral/String.hpp>

#include <filesystem>

#include "rtmcpp/Common.hpp"

namespace Nutcrackz {

	static std::unordered_map<std::string, DataType> s_DataTypeLookup = {
		{ "System.SByte", DataType::SByte },
		{ "System.Byte", DataType::Byte },
		{ "System.Int16", DataType::Short },
		{ "System.UInt16", DataType::UShort },
		{ "System.Int32", DataType::Int },
		{ "System.UInt32", DataType::UInt },
		{ "System.Int64", DataType::Long },
		{ "System.UInt64", DataType::ULong },
		{ "System.Single", DataType::Float },
		{ "System.Double", DataType::Double },
		//{ "System.Boolean", DataType::Bool },
		{ "System.String", DataType::String },
		{ "Coral.Managed.Interop.Bool32", DataType::Bool32 },
		{ "Nutcrackz.AssetHandle", DataType::AssetHandle },
		{ "Nutcrackz.Vector2", DataType::Vector2 },
		{ "Nutcrackz.Vector3", DataType::Vector3 },
		{ "Nutcrackz.Vector4", DataType::Vector4 },
		{ "Nutcrackz.Entity", DataType::Entity },
		//{ "Nutcrackz.Prefab", DataType::Prefab },
		//{ "Nutcrackz.Mesh", DataType::Mesh },
		//{ "Nutcrackz.StaticMesh", DataType::StaticMesh },
		//{ "Nutcrackz.Material", DataType::Material },
		//{ "Nutcrackz.Texture2D", DataType::Texture2D },
		//{ "Nutcrackz.Scene", DataType::Scene },
	};

	void OnCSharpException(std::string_view message)
	{
		std::string msg = message.data();
		NZ_CORE_ERROR("C# Exception: {}", msg);
	}

	void ScriptEngine::LoadProjectAssembly()
	{
		m_AppAssemblyData.reset();

		auto filepath = std::filesystem::absolute(Project::GetScriptModuleFilePath());

		m_AppAssemblyData = std::make_unique<AssemblyData>();
		m_AppAssemblyData->Assembly = &m_LoadContext->LoadAssembly(filepath.string());
		
		if (m_AppAssemblyData->Assembly->GetLoadStatus() != Coral::AssemblyLoadStatus::Success)
		{
			return;
		}

		BuildAssemblyCache(m_AppAssemblyData.get());
	}

	bool ScriptEngine::IsValidScript(UUID scriptID) const
	{
		if (!m_AppAssemblyData)
			return false;

		return m_AppAssemblyData->CachedTypes.contains(scriptID) && m_ScriptMetadata.contains(scriptID);
	}

	void OnCoralMessage(std::string_view message, Coral::MessageLevel level)
	{
		std::string msg = message.data();
		switch (level)
		{
		case Coral::MessageLevel::Info:
			NZ_CORE_INFO("Coral: {}", msg);
			break;
		case Coral::MessageLevel::Warning:
			NZ_CORE_WARN("Coral: {}", msg);
			break;
		case Coral::MessageLevel::Error:
			NZ_CORE_ERROR("Coral: {}", msg);
			break;
		}
	}

	const Nutcrackz::ScriptMetadata& ScriptEngine::GetScriptMetadata(UUID scriptID) const
	{
		NZ_CORE_VERIFY(m_ScriptMetadata.contains(scriptID));
		return m_ScriptMetadata.at(scriptID);
	}

	const Coral::Type* ScriptEngine::GetTypeByName(std::string_view name) const
	{
		for (const auto& [id, type] : m_CoreAssemblyData->CachedTypes)
		{
			Coral::String typeName = type->GetFullName();

			if (typeName == name)
			{
				Coral::String::Free(typeName);
				return type;
			}

			Coral::String::Free(typeName);
		}

		for (const auto& [id, type] : m_AppAssemblyData->CachedTypes)
		{
			Coral::String typeName = type->GetFullName();

			if (typeName == name)
			{
				Coral::String::Free(typeName);
				return type;
			}

			Coral::String::Free(typeName);
		}

		return nullptr;
	}

	const ScriptEngine& ScriptEngine::GetInstance() { return GetMutable(); }

	void ScriptEngine::InitializeHost()
	{
		m_Host = std::make_unique<Coral::HostInstance>();

		Coral::HostSettings settings =
		{
			.CoralDirectory = (std::filesystem::current_path() / "DotNet").string(),
			.MessageCallback = OnCoralMessage,
			.ExceptionCallback = OnCSharpException
		};

		NZ_CORE_VERIFY(m_Host->Initialize(settings) == Coral::CoralInitStatus::Success, "Failed to initialize Coral");
	}

	void ScriptEngine::ShutdownHost()
	{
		Coral::TypeCache::Get().Clear();

		m_Host->Shutdown();
		m_Host.reset();
	}

	void ScriptEngine::Initialize(RefPtr<Project> project)
	{
		m_LoadContext = std::make_unique<Coral::AssemblyLoadContext>(std::move(m_Host->CreateAssemblyLoadContext("NutcrackzLoadContext")));

		auto scriptCorePath = (std::filesystem::current_path() / "Resources" / "Scripts" / "net8.0" / "Nutcrackz-ScriptCore.dll").string();
		m_CoreAssemblyData = std::make_unique<AssemblyData>();

		m_CoreAssemblyData->Assembly = &m_LoadContext->LoadAssembly(scriptCorePath);
		BuildAssemblyCache(m_CoreAssemblyData.get());

		ScriptGlue::RegisterGlue(*m_CoreAssemblyData->Assembly);

		LoadProjectAssembly();
	}

	void ScriptEngine::Shutdown()
	{
		for (auto& [scriptID, scriptMetadata] : m_ScriptMetadata)
		{
			for (auto& [fieldID, fieldMetadata] : scriptMetadata.Fields)
			{
				fieldMetadata.DefaultValue.Release();
			}
		}
		m_ScriptMetadata.clear();

		m_AppAssemblyData.reset();
		m_CoreAssemblyData.reset();
		m_Host->UnloadAssemblyLoadContext(*m_LoadContext);

		Coral::TypeCache::Get().Clear();
	}

	/*void ScriptEngine::Initialize(Ref<Project> project)
	{
		m_Host = std::make_unique<Coral::HostInstance>();

		Coral::HostSettings settings =
		{
			.CoralDirectory = (std::filesystem::current_path() / "DotNet").string(),
			.MessageCallback = OnCoralMessage,
			.ExceptionCallback = OnCSharpException
		};

		NZ_CORE_VERIFY(m_Host->Initialize(settings), "Failed to initialize Coral");

		m_LoadContext = std::make_unique<Coral::AssemblyLoadContext>(std::move(m_Host->CreateAssemblyLoadContext("NutcrackzLoadContext")));

		auto scriptCorePath = (std::filesystem::current_path() / "Resources" / "Scripts" / "net8.0" / "Nutcrackz-ScriptCore.dll").string();

		NZ_CORE_WARN("Filepath: {}", scriptCorePath);

		m_CoreAssemblyData = CreateScope<AssemblyData>();

		m_CoreAssemblyData->Assembly = &m_LoadContext->LoadAssembly(scriptCorePath);
		BuildAssemblyCache(m_CoreAssemblyData.get());

		ScriptGlue::RegisterGlue(*m_CoreAssemblyData->Assembly);

		LoadProjectAssembly();
	}

	void ScriptEngine::Shutdown()
	{
		for (auto& [scriptID, scriptMetadata] : m_ScriptMetadata)
		{
			for (auto& [fieldID, fieldMetadata] : scriptMetadata.Fields)
			{
				fieldMetadata.DefaultValue.Release();
			}
		}
		m_ScriptMetadata.clear();

		m_AppAssemblyData.reset();
		m_CoreAssemblyData.reset();
		m_Host->UnloadAssemblyLoadContext(*m_LoadContext);

		m_Host->Shutdown();
		m_Host.reset();
	}*/

	void ScriptEngine::BuildAssemblyCache(AssemblyData* assemblyData)
	{
		auto& types = assemblyData->Assembly->GetTypes();
		auto& entityType = assemblyData->Assembly->GetType("Nutcrackz.Entity");

		for (auto& type : types)
		{
			std::string fullName = type->GetFullName();
			auto scriptID = Hash::GenerateFNVHash64Bit(fullName);

			if (!Project::GetActive()->GetEditorAssetManager()->IsAssetHandleValid(scriptID))
			{
				std::string fileName = fullName;
				size_t pos = fileName.find('.');
				fileName.erase(0, pos + 1);
				std::string finalName = fileName + ".cs";
			
				auto scriptModuleFilePath = std::filesystem::absolute(Project::GetActiveAssetDirectory());
				std::string finalFilePath = scriptModuleFilePath.string() + "/Scripts/Source/" + finalName;
				//NZ_CORE_WARN("FileName = {}", finalFilePath);
			
				if (std::filesystem::exists(finalFilePath))
					Project::GetActive()->GetEditorAssetManager()->ImportScriptAsset("Scripts/Source/" + finalName, scriptID);
			}

			assemblyData->CachedTypes[scriptID] = type;

			if (type->IsSubclassOf(entityType))
			{
				auto& metadata = m_ScriptMetadata[scriptID];
				metadata.FullName = fullName;

				auto temp = type->CreateInstance();

				for (auto& fieldInfo : type->GetFields())
				{
					Coral::ScopedString fieldName = fieldInfo.GetName();
					std::string fieldNameStr = fieldName;

					if (fieldNameStr.find("k__BackingField") != std::string::npos)
						continue;

					auto typeName = fieldInfo.GetType().GetFullName();

					if (!s_DataTypeLookup.contains(typeName))
						continue;

					if (fieldInfo.GetAccessibility() != Coral::TypeAccessibility::Public)
						continue;

					// NOTE(Peter): Entity.ID bleeds through to the inheriting scripts, annoying, but we can deal
					if (fieldName == "ID")
						continue;

					auto fullFieldName = fmt::format("{}.{}", fullName, fieldNameStr);
					uint32_t fieldID = Hash::GenerateFNVHash(fullFieldName);

					auto& fieldMetadata = metadata.Fields[fieldID];
					fieldMetadata.Name = fieldName;
					fieldMetadata.Type = s_DataTypeLookup.at(typeName);
					fieldMetadata.ManagedType = &fieldInfo.GetType();

					switch (fieldMetadata.Type)
					{
					case DataType::SByte:
						fieldMetadata.SetDefaultValue<int8_t>(temp);
						break;
					case DataType::Byte:
						fieldMetadata.SetDefaultValue<uint8_t>(temp);
						break;
					case DataType::Short:
						fieldMetadata.SetDefaultValue<int16_t>(temp);
						break;
					case DataType::UShort:
						fieldMetadata.SetDefaultValue<uint16_t>(temp);
						break;
					case DataType::Int:
						fieldMetadata.SetDefaultValue<int32_t>(temp);
						break;
					case DataType::UInt:
						fieldMetadata.SetDefaultValue<uint32_t>(temp);
						break;
					case DataType::Long:
						fieldMetadata.SetDefaultValue<int64_t>(temp);
						break;
					case DataType::ULong:
						fieldMetadata.SetDefaultValue<uint64_t>(temp);
						break;
					case DataType::Float:
						fieldMetadata.SetDefaultValue<float>(temp);
						break;
					case DataType::Double:
						fieldMetadata.SetDefaultValue<double>(temp);
						break;
					case DataType::Bool:
						fieldMetadata.SetDefaultValue<bool>(temp);
						break;
					case DataType::String:
						fieldMetadata.SetDefaultValue<std::string_view>(temp);
						break;
					case DataType::Bool32:
						fieldMetadata.SetDefaultValue<uint32_t>(temp);
						break;
					case DataType::AssetHandle:
						fieldMetadata.SetDefaultValue<uint64_t>(temp);
						break;
					case DataType::Vector2:
						fieldMetadata.SetDefaultValue<rtmcpp::Vec2>(temp);
						break;
					case DataType::Vector3:
						fieldMetadata.SetDefaultValue<rtmcpp::Vec3>(temp);
						break;
					case DataType::Vector4:
						fieldMetadata.SetDefaultValue<rtmcpp::Vec4>(temp);
						break;
					case DataType::Entity:
						fieldMetadata.SetDefaultValue<uint64_t>(temp);
						//case DataType::Prefab:
						//case DataType::Mesh:
						//case DataType::StaticMesh:
						//case DataType::Material:
						//case DataType::Texture2D:
						//case DataType::Scene:
						//	fieldMetadata.DefaultValue.Allocate(sizeof(uint64_t));
						//	fieldMetadata.DefaultValue.ZeroInitialize();
						//	break;
					default:
						break;
					}
				}

				temp.Destroy();
			}
		}
	}

	ScriptEngine& ScriptEngine::GetMutable()
	{
		static ScriptEngine s_Instance;
		return s_Instance;
	}

}
