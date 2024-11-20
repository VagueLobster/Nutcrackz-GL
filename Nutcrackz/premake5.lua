local GRDK = os.getenv("GRDKLatest");

project "Nutcrackz"
	kind "StaticLib"
	language "C++"
	cppdialect "C++latest"
	staticruntime "off"
	
	targetdir ("%{wks.location}/build/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/build/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "nzpch.hpp"
	pchsource "src/nzpch.cpp"

	files
	{
		"src/**.hpp",
		"src/**.cpp",
		"vendor/stb_image/**.h",
		"vendor/stb_image/**.cpp",
		"vendor/flecs/flecs/**.h",
		"vendor/flecs/flecs/**.c",
		"vendor/tiny_obj_loader/**.h",
		
		"vendor/ImGuizmo/ImGuizmo.h",
		"vendor/ImGuizmo/ImGuizmo.cpp"
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE",
		"_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING",
		"TRACY_ENABLE",
		"TRACY_ON_DEMAND",
		"TRACY_CALLSTACK=10",
		"RTMCPP_EXPORT="
	}

	includedirs
	{
		"src",
		"vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.Box2D}",
		"%{IncludeDir.filewatch}",
		"%{IncludeDir.msdfgen}",
		"%{IncludeDir.msdf_atlas_gen}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.tiny_obj_loader}",
		"%{IncludeDir.flecs}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.yyjson}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.VulkanSDK}",		
		"%{IncludeDir.miniaudio}",
		"%{IncludeDir.choc}",
		"%{IncludeDir.FFMpeg}",
		"%{IncludeDir.nethost}",
		"%{IncludeDir.Coral}",
		"%{IncludeDir.MagicEnum}",
		"%{IncludeDir.Tracy}",
		"%{IncludeDir.rtm}",
		"%{IncludeDir.rtmcpp}",
		"%{IncludeDir.Wooting}",
	}

	links
	{
		"GLFW",
		"Glad",
		"ImGui",
		"msdf-atlas-gen",
		"Box2D",
		"yaml-cpp",
		"yyjson",
		"opengl32.lib",
		"%{Library.Coral}",
		"%{Library.Tracy}",
		"%{Library.Wooting}",

		"%{Library.FFMpeg_avcodec}",
		"%{Library.FFMpeg_avdevice}",
		"%{Library.FFMpeg_avfilter}",
		"%{Library.FFMpeg_avformat}",
		"%{Library.FFMpeg_avutil}",
		"%{Library.FFMpeg_swresample}",
		"%{Library.FFMpeg_swscale}",
	}
	
	externalincludedirs
	{
		"src/",
		"vendor/rtmcpp/rtm/includes/"
	}
	
	filter "files:vendor/ImGuizmo/**.cpp"
	flags { "NoPCH" }

	filter "system:windows"
		systemversion "latest"
		
		externalincludedirs
		{
			GRDK .. "/GameKit/Include/",
		}

		libdirs
		{
			GRDK .. "/GameKit/Lib/amd64/"
		}
		
		links
		{
			"GameInput",
			"xgameruntime",
			"Ws2_32",
			"Userenv",
			"ntdll",
			
			"%{Library.WinSock}",
			"%{Library.WinMM}",
			"%{Library.WinVersion}",
			"%{Library.BCrypt}",
			"%{Library.Dbghelp}",
		}

	filter "configurations:Debug"
		defines "NZ_DEBUG"
		runtime "Debug"
		symbols "on"
		inlining ("Auto")
		editandcontinue "Off"
		
		links
		{
			"%{Library.ShaderC_Debug}",
			"%{Library.SPIRV_Cross_Debug}",
			"%{Library.SPIRV_Cross_GLSL_Debug}",
		}
		

	filter "configurations:Release"
		defines "NZ_RELEASE"
		runtime "Release"
		optimize "on"
		inlining ("Auto")
		
		links
		{
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}",
		}

	filter "configurations:Dist"
		defines "NZ_DIST"
		runtime "Release"
		optimize "on"
		inlining ("Auto")
		
		links
		{
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}",
		}