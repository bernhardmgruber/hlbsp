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

find_package(Freetype REQUIRED)
if (NOT MSVC)
	include_directories(${FREETYPE_INCLUDE_DIRS})
endif()

find_package(imgui CONFIG REQUIRED)

include_directories(thirdparty)

# executable
file(GLOB_RECURSE source_files src/*.cpp src/*.h src/*.hpp src/*.vert src/*.frag thirdparty/*.cpp thirdparty/*.h)
add_executable(${PROJECT_NAME} ${source_files})
source_group("" FILES ${source_files})
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${source_files})

target_link_libraries(${PROJECT_NAME} PRIVATE
	GLEW::GLEW
	imgui::imgui
	${Boost_LIBRARIES}
)
if(MSVC)
	target_link_libraries(${PROJECT_NAME} PRIVATE
		glfw
		Freetype::Freetype
		opengl32
	)
else()
	target_link_libraries(${PROJECT_NAME} PRIVATE
		${GLFW_LIBRARIES}
		${FREETYPE_LIBRARIES}
		GL
		stdc++fs
	)
endif()

target_compile_options(${PROJECT_NAME} PRIVATE
	-DGLFW_INCLUDE_NONE
	-DGLM_ENABLE_EXPERIMENTAL
	-DGLM_FORCE_RADIANS
	-DIMGUI_IMPL_OPENGL_LOADER_GLEW
)
if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

# clang-format
add_custom_target(
	clang-format
	COMMAND clang-format
	-style=file
	-i
	${source_files}
)
