project "rtmcpp"
	kind "None"
	location "Include/"

	files
	{
		"rtmcpp/**.hpp"
	}

	externalincludedirs
	{
		"./rtm/includes/"
	}
	
	defines { "RTMCPP_EXPORT=" }

	filter "system:windows"
		systemversion "latest"
		cppdialect "C++latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

    filter "configurations:Dist"
		runtime "Release"
		optimize "on"
        symbols "off"