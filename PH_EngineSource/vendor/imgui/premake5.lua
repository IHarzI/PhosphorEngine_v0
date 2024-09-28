project "ImGui"
	kind "StaticLib"
	language "C++"
    staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    includedirs 
	{
        "../imgui"
    }

	files
	{
		"imconfig.h",
		"imgui.h",
		"imgui.cpp",
		"imgui_draw.cpp",
		"imgui_internal.h",
		"imgui_tables.cpp",
		"imgui_widgets.cpp",
		"imstb_rectpack.h",
		"imstb_textedit.h",
		"imstb_truetype.h",
		"imgui_demo.cpp",
        "misc/cpp/imgui_stdlib.h",
         "misc/cpp/imgui_stdlib.cpp"
	}

	filter "system:windows"
		systemversion "latest"
		cppdialect "C++17"
		buildoptions "/MD"

	filter "system:linux"
		pic "On"
		systemversion "latest"
		cppdialect "C++17"
		buildoptions "/MD"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
		buildoptions "/MD"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"
		buildoptions "/MD"

    filter "configurations:Dist"
		runtime "Release"
		optimize "on"
        symbols "off"
		buildoptions "/MD"
