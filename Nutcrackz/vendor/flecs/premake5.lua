project "Flecs"
	kind "None"
	language "C"
	staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"flecs/**.h",
		"flecs/**.c",
	}

	includedirs { "flecs/" }
	
	filter "files:**.c"
    flags {"NoPCH"}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "On"
		conformancemode "On"

	filter "configurations:Release"
		runtime "Release"
		optimize "On"
		conformancemode "On"

	filter "configurations:Dist"
		runtime "Release"
		optimize "Full"
		conformancemode "On"
