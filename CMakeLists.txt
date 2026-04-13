cmake_minimum_required(VERSION 3.22)

cmake_policy(SET CMP0091 NEW)

if(WIN32)
	set(VCPKG_TARGET_TRIPLET "x64-windows-static")
elseif (LINUX)
	set(VCPKG_TARGET_TRIPLET "x64-linux")
endif()

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

set(VCPKG_OVERLAY_PORTS ${CMAKE_CURRENT_SOURCE_DIR}/overlays)
set(VCPKG_FEATURE_FLAGS versions)

project(mimalloc_so5_case LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(mimalloc CONFIG REQUIRED)
find_package(sobjectizer CONFIG REQUIRED)

add_executable(mimalloc_so5_case
	main.cpp
)
target_link_libraries(mimalloc_so5_case
	PRIVATE
		$<IF:$<TARGET_EXISTS:mimalloc-static>,mimalloc-static,mimalloc>
		sobjectizer::StaticLib)

if(WIN32)
	set_property(TARGET mimalloc_so5_case PROPERTY
		MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

