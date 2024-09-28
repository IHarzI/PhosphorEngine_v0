workspace "PhosphorEngine"
	architecture "x64"
configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

	-- include dir relatives to root folder
	IncludeDir = {}
	IncludeDir["GLFW"] = "PH_EngineSource/vendor/glfw/include"
	IncludeDir["imgui"] = "PH_EngineSource/vendor/imgui"
	IncludeDir["stb_image"] = "PH_EngineSource/vendor/stb"
	IncludeDir["Vulkan"] = "$(VULKAN_SDK)"
	IncludeDir["glfwLIB2022"] = "PH_EngineSource/vendor/glfw/lib-vc2022"
	IncludeDir["TinyObjLoader"] = "PH_EngineSource/vendor/TinyObjLoader"
	IncludeDir["SimdJson"] = "PH_EngineSource/vendor/simdjson/singleheader"
	IncludeDir["GLTF"] = "PH_EngineSource/vendor/fastgltf/include"
	IncludeDir["SpirVReflect"] = "PH_EngineSource/vendor/SpirVReflect"

	    startproject "Sandbox"
	os.execute("call ShadersCompile.bat")

include "PH_EngineSource/vendor/imgui"

project "PH_EngineSource"
	location "PH_EngineSource"
	kind "StaticLib"
	language "C++"
	staticruntime "on"
	cppdialect "C++17"

	targetdir("bin/" .. outputdir .. "/%{prj.name}")
	objdir("bin-int/" .. outputdir .. "/%{prj.name}")

	files 
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/stb_image/**.cpp",
		"%{prj.name}/vendor/stb_image/**.h",
		"%{prj.name}/vendor/imgui/backends/imgui_impl_glfw.cpp",
		"%{prj.name}/vendor/imgui/backends/imgui_impl_glfw.h",
		"%{prj.name}/vendor/imgui/backends/imgui_impl_vulkan.cpp",
		"%{prj.name}/vendor/imgui/backends/imgui_impl_vulkan.h",
		"%{prj.name}/vendor/fastgltf/include/fastgltf/**.h",
		"%{prj.name}/vendor/fastgltf/src/**.cpp",
		"%{prj.name}/vendor/simdjson/singleheader/**.h",
		"%{prj.name}/vendor/simdjson/singleheader/**.cpp",
		"%{prj.name}/vendor/SpirVReflect/**.cpp",
		"%{prj.name}/vendor/SpirVReflect/**.h"
	}

	includedirs 
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor", 
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.imgui}", 
		"%{IncludeDir.SimdJson}",
		"%{IncludeDir.GLTF}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.SpirVReflect}",
		"$(VULKAN_SDK)/include"
	}
	
	libdirs { "%{IncludeDir.Vulkan}/Lib", "%{IncludeDir.glfwLIB2022}" }
	

	links
	{
		"glfw3.lib",
		"ImGui",
		"$(VULKAN_SDK)/lib/vulkan-1.lib" 
	}

	filter "system:windows"
	
		staticruntime "On"
		systemversion "latest"

		defines 
		{
			"PH_PLATFORM_WINDOWS",
			"GLFW_INCLUDE_VULKAN"
		}

		postbuildcommands
			{
				("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Sandbox")
			}

	filter "configurations:Debug"
		defines "_PH_DEBUG"
		runtime "Debug"
		buildoptions "/MD"
		symbols "on"
		optimize "off"
	
	filter "configurations:Release"
		defines "_PH_RELEASE"
		runtime "Release"
		buildoptions "/MD"
		optimize "on"
	
	
	filter "configurations:Dist"
		defines "_PH_DIST"
		runtime "Release"
		buildoptions "/MD"
		optimize "on"

project "Sandbox"
location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	staticruntime "on"
	cppdialect "C++17"

	targetdir("bin/" .. outputdir .. "/%{prj.name}")
	objdir("bin-int/" .. outputdir .. "/%{prj.name}")

	files 
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"PH_EngineSource/src",
		"%{prj.name}/src",
		"$(VULKAN_SDK)/include" 
	}

	links
	{
		"PH_EngineSource"
	}

	filter "system:windows"
	
		staticruntime "On"
		systemversion "latest"

		defines 
		{
			"PH_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "_PH_DEBUG"
		runtime "Debug"
		buildoptions "/MD"
		symbols "on"
		optimize "off"
	
	filter "configurations:Release"
		defines "_PH_RELEASE"
		runtime "Release"
		buildoptions "/MD"
		optimize "on"
	
	
	filter "configurations:Dist"
		defines "_PH_DIST"
		runtime "Release"
		buildoptions "/MD"
		optimize "on"