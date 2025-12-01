project "Hazel"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "hzpch.h"
	pchsource "src/hzpch.cpp"

	files
	{
		"src/**.h",
		"src/**.cpp",
		"vendor/stb_image/**.h",
		"vendor/stb_image/**.cpp",
		"vendor/glm/glm/**.hpp",
		"vendor/glm/glm/**.inl",
		"vendor/VMA/VulkanMemoryAllocator/**.h",
		"vendor/VMA/VulkanMemoryAllocator/**.cpp",
		"vendor/ImGuizmo/ImGuizmo.h",
		"vendor/ImGuizmo/ImGuizmo.cpp",
		"vendor/spirv_reflect/spirv_reflect.c",
		"%{IncludeDir.mikktspace}/mikktspace.c",
		"%{IncludeDir.VulkanSDK}/Include/Volk/volk.c",  -- 添加编译后会让原来架构找不到Vulkan接口找不到
		-- "%{IncludeDir.imgui_node_editor}/**.cpp"
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE"
	}

	includedirs
	{
		"src",
		"vendor/spdlog/include",
		"%{IncludeDir.Box2D}",
		"%{IncludeDir.filewatch}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.msdfgen}",
		"%{IncludeDir.msdf_atlas_gen}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.mono}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.VMA}",
		"%{IncludeDir.VulkanSDK}/Include",
		"%{IncludeDir.choc}",
		"%{IncludeDir.nfd}",
		"%{IncludeDir.assimp}",
		"%{IncludeDir.acl}",
		"%{IncludeDir.rtm}",
		"%{IncludeDir.spirv_reflect}",
		"%{IncludeDir.cereal}",
		"%{IncludeDir.imgui_node_editor}",
		"%{IncludeDir.mikktspace}"
	}

	libdirs
	{
		"%{LibraryDir.VulkanSDK}",
		"%{LibraryDir.assimp}",
	}
	links
	{
		"Box2D",
		"GLFW",
		"Glad",
		"ImGui",
		"msdf-atlas-gen",
		"yaml-cpp",
		"opengl32.lib",
		"vulkan-1.lib",
		"%{Library.mono}",
		"NFD-Extended",
		"assimp-vc143-mtd.lib",
		"shadercd", 
        "shaderc_combinedd", 
        "glslangd", 
        "SPIRVd", 
        "spirv-toolsd" 
	}

	filter "files:vendor/ImGuizmo/**.cpp"
	flags { "NoPCH" }
	filter "files:vendor/spirv_reflect/spirv_reflect.c"
    flags { "NoPCH" }
	filter "files:**/volk.c"
    flags { "NoPCH" } 
	filter "files:**/mikktspace.c"
    flags { "NoPCH" } 

	filter "system:windows"
		systemversion "latest"

		defines
		{
		}

		links
		{
			"%{Library.WinSock}",
			"%{Library.WinMM}",
			"%{Library.WinVersion}",
			"%{Library.BCrypt}",
		}

	filter "configurations:Debug"
		defines "HZ_DEBUG"
		runtime "Debug"
		symbols "on"

		links
		{
			"%{Library.ShaderC_Debug}",
			"%{Library.SPIRV_Cross_Debug}",
			"%{Library.SPIRV_Cross_GLSL_Debug}"
		}

	filter "configurations:Release"
		defines "HZ_RELEASE"
		runtime "Release"
		optimize "on"

		links
		{
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}"
		}

	filter "configurations:Dist"
		defines "HZ_DIST"
		runtime "Release"
		optimize "on"

		links
		{
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}"
		}
	filter "configurations:Debug"
	postbuildcommands {
		'{COPY} "../Hazel/vendor/assimp/bin/windows/Debug/assimp-vc143-mtd.dll" "%{cfg.targetdir}"',
	}