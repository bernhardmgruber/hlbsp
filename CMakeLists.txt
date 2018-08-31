cmake_minimum_required(VERSION 3.0)

project(hlbsp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system filesystem)
include_directories(${Boost_INCLUDE_DIRS})

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})

find_package(glfw3 REQUIRED)
include_directories(${GLFW3_INCLUDE_DIRS})

find_package(Freetype REQUIRED)

include_directories(thirdparty)

file(GLOB_RECURSE source_files src/*.cpp src/*.h src/*.hpp src/*.vert src/*.frag)
add_executable(${PROJECT_NAME} ${source_files})
source_group("" FILES ${source_files})
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${source_files})

target_compile_options(${PROJECT_NAME} PRIVATE
	-DGLM_ENABLE_EXPERIMENTAL
)

if(MSVC)
	target_link_libraries(${PROJECT_NAME} PRIVATE
		GLEW::GLEW
		Freetype::Freetype
		glfw
		opengl32
		${Boost_LIBRARIES}
	)
else()
	target_link_libraries(${PROJECT_NAME} PRIVATE
		GLEW::GLEW
		Freetype::Freetype
		glfw
		GL
		stdc++fs
		${Boost_LIBRARIES}
	)
endif()

if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

add_custom_target(
	clang-format
	COMMAND clang-format
	-style=file
	-i
	${source_files}
)
