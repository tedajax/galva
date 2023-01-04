workspace "galva"
	configurations { "Debug", "Release" }
	platforms { "Win64" }
	location "bin"
	files { "*.natvis" }
	includedirs { "include" }

filter "configurations:Debug"
	optimize "Off"
	symbols "On"
	defines { "_DEBUG" }

filter "platforms:Win64"
	system "Windows"
	architecture "x86_64"
	defines {"_CRT_SECURE_NO_WARNINGS" }
	disablewarnings { "4005" }

project "galva"
	kind "ConsoleApp"
	language "C"
	cdialect "C11"
	location "bin/galva"
	files { "src/**.c", "src/**.h", "src/**.inl" }
	debugdir "."

	filter "platforms:Win64"
		filter "configurations:Release"
			entrypoint "mainCRTStartup"
			kind "WindowedApp"
