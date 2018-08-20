project(hlbsp)

file(GLOB_RECURSE source_files *.cpp *.h *.hpp *.vert *.frag)
add_executable(${PROJECT_NAME} ${source_files})
source_group("" FILES ${source_files})
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${source_files})

if(MSVC)
	target_link_libraries(${PROJECT_NAME} PRIVATE
		GLEW::GLEW
		glfw
		opengl32
		glu32
		Winmm
		Imm32
		Version
		${Boost_LIBRARIES}
	)
else()
	target_link_libraries(${PROJECT_NAME} PRIVATE
		GLEW::GLEW
		glfw
		GL
		GLU
		X11
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
