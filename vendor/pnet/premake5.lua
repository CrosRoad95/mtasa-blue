project "netc_d"
	language "C++"
	targetname "netc_d"

	includedirs { "." }

	vpaths {
		["Headers/*"] = "**.h",
		["Sources/*"] = {"**.cpp"},
		["*"] = "premake5.lua"
	}

	files {
		"premake5.lua",
		"*.cpp",
		"*.h"
	}

	filter "system:windows"
		kind "SharedLib"
		targetdir(buildpath("mta"))
