include "Dependencies.lua"

workspace "Nutcrackz"
	conformancemode "On"
	startproject "Nutcrackz-Editor"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	flags
	{
		"MultiProcessorCompile"
	}
	
	filter "language:C++ or language:C"
		architecture "x86_64"
		
	filter "files:**.c"
		flags {"NoPCH"}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
	include "Nutcrackz/vendor/Coral/Coral.Native"
	include "Nutcrackz/vendor/Coral/Coral.Managed"
	include "Nutcrackz/vendor/Box2D"
	include "Nutcrackz/vendor/tracy"
	include "Nutcrackz/vendor/GLFW"
	include "Nutcrackz/vendor/Glad"
	include "Nutcrackz/vendor/msdf-atlas-gen"
	include "Nutcrackz/vendor/imgui"
	include "Nutcrackz/vendor/yaml-cpp"
	include "Nutcrackz/vendor/yyjson"
	include "Nutcrackz/vendor/rtmcpp"
	include "Nutcrackz/vendor/flecs"
group ""

group "Core"
	include "Nutcrackz"
	include "Nutcrackz-ScriptCore"
group ""

group "Tools"
	include "Nutcrackz-Editor"
group ""
