cmake_minimum_required(VERSION 3.0)

project(hlbsp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find packages
find_package(Boost REQUIRED COMPONENTS system filesystem)
include_directories(${Boost_INCLUDE_DIRS})

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})

if (MSVC)
	find_package(glfw3 REQUIRED)
else()
	find_package(PkgConfig REQUIRED)
	pkg_search_module(GLFW REQUIRED glfw3)
endif()
include_directories(${GLFW3_INCLUDE_DIRS})

find_package(glm)
if (glm_FOUND)
	include_directories(${glm_INCLUDE_DIR})
else()
	find_package(GLM) # older versions
	include_directories(${GLM_INCLUDE_DIR})
endif()

include_directories(thirdparty)

# executable
file(GLOB source_files src/* thirdparty/*)
file(GLOB_RECURSE opengl_files src/opengl/*)
list(APPEND source_files ${opengl_files})
if(WIN32)
	message("Including DirectX")
	file(GLOB_RECURSE directx_files src/directx11/*)
	foreach(directx_file ${directx_files})
		if(${directx_file} MATCHES ".+\\.hlsl")
			set_source_files_properties(${directx_file} PROPERTIES VS_TOOL_OVERRIDE "None")
		endif()
	endforeach()
	list(APPEND source_files ${directx_files})
else()
	file(GLOB_RECURSE directx_imgui_files thirdparty/imgui_impl_dx11.*)
	list(REMOVE_ITEM source_files ${directx_imgui_files})
endif()

add_executable(${PROJECT_NAME} ${source_files})
source_group("" FILES ${source_files})
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${source_files})

target_link_libraries(${PROJECT_NAME} PRIVATE
	GLEW::GLEW
	${Boost_LIBRARIES}
	glfw
	OpenGL::GL
)

target_compile_options(${PROJECT_NAME} PRIVATE
	-DGLFW_INCLUDE_NONE
	-DGLM_ENABLE_EXPERIMENTAL
	-DGLM_FORCE_RADIANS
	-DIMGUI_IMPL_OPENGL_LOADER_GLEW
)

if(WIN32)
	target_compile_options(${PROJECT_NAME} PRIVATE -DGLFW_EXPOSE_NATIVE_WIN32 -DNOMINMAX)
	target_link_libraries(${PROJECT_NAME} PRIVATE D3D11.lib D3DCompiler.lib)
endif()

if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

# clang-format
find_program(CLANG_FORMAT NAMES "clang-format")
if(NOT CLANG_FORMAT)
	message("clang-format not found")
else()
	message("clang-format found: ${CLANG_FORMAT}")
	add_custom_target(clang-format COMMAND ${CLANG_FORMAT} -style=file -i ${source_files})
endif()

# clang-tidy
find_program(CLANG_TIDY NAMES "clang-tidy")
if(NOT CLANG_TIDY)
	message("clang-tidy not found")
else()
	message("clang-tidy found: ${CLANG_TIDY}")
	set_target_properties(${PROJECT_NAME} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY}")
	configure_file(.clang-tidy .clang-tidy COPYONLY)
endif()
