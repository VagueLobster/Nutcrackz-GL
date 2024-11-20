local GRDK = os.getenv("GRDKLatest");

project "Nutcrackz-Editor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++latest"
	staticruntime "off"

	targetdir ("%{wks.location}/build/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/build/bin-int/" .. outputdir .. "/%{prj.name}")
	
	files
	{
		"Nutcrackz-Editor.rc",
		"resource.h",
		"src/**.hpp",
		"src/**.cpp"
	}

	includedirs
	{
		"src",
		"%{wks.location}/Nutcrackz/vendor/spdlog/include",
		"%{wks.location}/Nutcrackz/src",
		"%{wks.location}/Nutcrackz/vendor",
		"%{IncludeDir.flecs}",
		"%{IncludeDir.filewatch}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.yyjson}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.miniaudio}",
		"%{IncludeDir.choc}",
		"%{IncludeDir.FFMpeg}",
		"%{IncludeDir.nethost}",
		"%{IncludeDir.Coral}",
		"%{IncludeDir.Tracy}",
		"%{IncludeDir.rtm}",
		"%{IncludeDir.rtmcpp}",
		"%{IncludeDir.Wooting}",
	}

	links
	{
		"Nutcrackz",
		
		"yaml-cpp",

		"%{Library.FFMpeg_avcodec}",
		"%{Library.FFMpeg_avdevice}",
		"%{Library.FFMpeg_avfilter}",
		"%{Library.FFMpeg_avformat}",
		"%{Library.FFMpeg_avutil}",
		"%{Library.FFMpeg_swresample}",
		"%{Library.FFMpeg_swscale}",
		"%{Library.Coral}",
		"%{Library.Tracy}",
		"%{Library.Wooting}",
	}

	defines
	{
		"_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING",
		"TRACY_ENABLE",
		"TRACY_ON_DEMAND",
		"TRACY_CALLSTACK=10",
		"RTMCPP_EXPORT="
	}

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
	}

	filter "configurations:Debug"
		defines "NZ_DEBUG"
		runtime "Debug"
		symbols "on"
		inlining ("Auto")
		editandcontinue "Off"

	filter "configurations:Release"
		defines "NZ_RELEASE"
		runtime "Release"
		optimize "on"
		inlining ("Auto")

	filter "configurations:Dist"
		kind "WindowedApp"
		defines "NZ_DIST"
		runtime "Release"
		optimize "on"
		inlining ("Auto")
		
	filter ""

project "Coral.Native"
	dependson "Coral.Managed"

	filter { "configurations:Debug" }
		postbuildcommands
		{
			'{COPYFILE} "%{wks.location}Nutcrackz/vendor/Coral/Coral.Managed/Coral.Managed.runtimeconfig.json" "%{wks.location}Nutcrackz-Editor/DotNet/Coral.Managed.runtimeconfig.json"',
			'{COPYFILE} "%{wks.location}Nutcrackz/vendor/Coral/Build/Debug/Coral.Managed.dll" "%{wks.location}Nutcrackz-Editor/DotNet/Coral.Managed.dll"',
			'{COPYFILE} "%{wks.location}Nutcrackz/vendor/Coral/Build/Debug/Coral.Managed.pdb" "%{wks.location}Nutcrackz-Editor/DotNet/Coral.Managed.pdb"',
			'{COPYFILE} "%{wks.location}Nutcrackz/vendor/Coral/Build/Debug/Coral.Managed.deps.json" "%{wks.location}Nutcrackz-Editor/DotNet/Coral.Managed.deps.json"',
		}

	filter { "configurations:Release" }
		postbuildcommands
		{
			'{COPYFILE} "%{wks.location}Nutcrackz/vendor/Coral/Coral.Managed/Coral.Managed.runtimeconfig.json" "%{wks.location}Nutcrackz-Editor/DotNet/Coral.Managed.runtimeconfig.json"',
			'{COPYFILE} "%{wks.location}Nutcrackz/vendor/Coral/Build/Release/Coral.Managed.dll" "%{wks.location}Nutcrackz-Editor/DotNet/Coral.Managed.dll"',
			'{COPYFILE} "%{wks.location}Nutcrackz/vendor/Coral/Build/Release/Coral.Managed.pdb" "%{wks.location}Nutcrackz-Editor/DotNet/Coral.Managed.pdb"',
			'{COPYFILE} "%{wks.location}Nutcrackz/vendor/Coral/Build/Release/Coral.Managed.deps.json" "%{wks.location}Nutcrackz-Editor/DotNet/Coral.Managed.deps.json"',
		}