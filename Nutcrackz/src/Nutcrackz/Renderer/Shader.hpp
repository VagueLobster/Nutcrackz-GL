#pragma once

#include "Nutcrackz/Core/Ref.hpp"

#include <string>
#include <unordered_map>

//#include <glm/glm.hpp>
//#include "rtmcpp/Common.hpp"

namespace Nutcrackz {

	class Shader : public RefCounted
	{
	public:
		virtual ~Shader() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual const std::string& GetName() const = 0;

		static RefPtr<Shader> Create(const std::string& filepath);
		static RefPtr<Shader> Create(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
	};

	class ShaderLibrary
	{
	public:
		void Add(const std::string& name, const RefPtr<Shader>& shader);
		void Add(const RefPtr<Shader>& shader);
		RefPtr<Shader> Load(const std::string& filepath);
		RefPtr<Shader> Load(const std::string& name, const std::string& filepath);

		RefPtr<Shader> Get(const std::string& name);

		bool Exists(const std::string& name) const;
	private:
		std::unordered_map<std::string, RefPtr<Shader>> m_Shaders;
	};

}