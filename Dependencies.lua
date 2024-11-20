-- Nutcrackz Dependencies

VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["stb_image"] = "%{wks.location}/Nutcrackz/vendor/stb_image"
IncludeDir["tiny_obj_loader"] = "%{wks.location}/Nutcrackz/vendor/tiny_obj_loader"
IncludeDir["yaml_cpp"] = "%{wks.location}/Nutcrackz/vendor/yaml-cpp/include"
IncludeDir["yyjson"] = "%{wks.location}/Nutcrackz/vendor/yyjson/src"
IncludeDir["Box2D"] = "%{wks.location}/Nutcrackz/vendor/Box2D/include"
IncludeDir["filewatch"] = "%{wks.location}/Nutcrackz/vendor/filewatch"
IncludeDir["GLFW"] = "%{wks.location}/Nutcrackz/vendor/GLFW/include"
IncludeDir["Glad"] = "%{wks.location}/Nutcrackz/vendor/Glad/include"
IncludeDir["ImGui"] = "%{wks.location}/Nutcrackz/vendor/imgui"
IncludeDir["ImGuizmo"] = "%{wks.location}/Nutcrackz/vendor/ImGuizmo"
IncludeDir["miniaudio"] = "%{wks.location}/Nutcrackz/vendor/miniaudio/include"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
IncludeDir["choc"] = "%{wks.location}/Nutcrackz/vendor/choc"
IncludeDir["msdfgen"] = "%{wks.location}/Nutcrackz/vendor/msdf-atlas-gen/msdfgen"
IncludeDir["msdf_atlas_gen"] = "%{wks.location}/Nutcrackz/vendor/msdf-atlas-gen/msdf-atlas-gen"
IncludeDir["Coral"] = "%{wks.location}/Nutcrackz/vendor/Coral/Coral.Native/Include"
IncludeDir["nethost"] = "%{wks.location}/Nutcrackz/vendor/Coral/NetCore/7.0.7"
IncludeDir["FFMpeg"] = "%{wks.location}/Nutcrackz/vendor/FFMpeg/include"
IncludeDir["MagicEnum"] = "%{wks.location}/Nutcrackz/vendor/magic_enum/include"
IncludeDir["Tracy"] = "%{wks.location}/Nutcrackz/vendor/tracy/tracy/public"
IncludeDir["rtm"] = "%{wks.location}/Nutcrackz/vendor/rtmcpp/rtm/includes"
IncludeDir["rtmcpp"] = "%{wks.location}/Nutcrackz/vendor/rtmcpp/Include"
IncludeDir["Wooting"] = "%{wks.location}/Nutcrackz/vendor/wooting/includes-cpp"
IncludeDir["flecs"] = "%{wks.location}/Nutcrackz/vendor/flecs"

LibraryDir = {}

LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"
LibraryDir["Coral"] = "%{wks.location}/Nutcrackz/vendor/Coral/Build/%{cfg.buildcfg}"
LibraryDir["Tracy"] = "%{wks.location}/Nutcrackz/vendor/tracy/bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/Tracy"
LibraryDir["FFMpeg"] = "%{wks.location}/Nutcrackz/vendor/FFMpeg/lib"
LibraryDir["Wooting"] = "%{wks.location}/Nutcrackz/vendor/wooting/lib"

Library = {}
Library["Coral"] = "%{LibraryDir.Coral}/Coral.Native.lib"
Library["Tracy"] = "%{LibraryDir.Tracy}/Tracy.lib"
Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
Library["VulkanUtils"] = "%{LibraryDir.VulkanSDK}/Vklayer_utils.lib"

Library["ShaderC_Debug"] = "%{LibraryDir.VulkanSDK}/shaderc_sharedd.lib"
Library["SPIRV_Cross_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsld.lib"
Library["SPIRV_Tools_Debug"] = "%{LibraryDir.VulkanSDK}/SPIRV-Toolsd.lib"

Library["ShaderC_Release"] = "%{LibraryDir.VulkanSDK}/shaderc_shared.lib"
Library["SPIRV_Cross_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsl.lib"

Library["FFMpeg_avcodec"] = "%{LibraryDir.FFMpeg}/avcodec.lib"
Library["FFMpeg_avdevice"] = "%{LibraryDir.FFMpeg}/avdevice.lib"
Library["FFMpeg_avfilter"] = "%{LibraryDir.FFMpeg}/avfilter.lib"
Library["FFMpeg_avformat"] = "%{LibraryDir.FFMpeg}/avformat.lib"
Library["FFMpeg_avutil"] = "%{LibraryDir.FFMpeg}/avutil.lib"
Library["FFMpeg_swresample"] = "%{LibraryDir.FFMpeg}/swresample.lib"
Library["FFMpeg_swscale"] = "%{LibraryDir.FFMpeg}/swscale.lib"

Library["Wooting"] = "%{LibraryDir.Wooting}/wooting_analog_wrapper.lib"

-- Windows
Library["WinSock"] = "Ws2_32.lib"
Library["WinMM"] = "Winmm.lib"
Library["WinVersion"] = "Version.lib"
Library["BCrypt"] = "Bcrypt.lib"
Library["Dbghelp"] = "Dbghelp.lib"