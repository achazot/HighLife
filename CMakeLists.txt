cmake_minimum_required(VERSION 3.3)
project(life)

set(CMAKE_BUILD_TYPE Debug)

file(GLOB_RECURSE SOURCES "src/*.c")

set(EXECUTABLE_OUTPUT_PATH ./build)


add_executable (
	life ${SOURCES}
)

add_custom_command(
	TARGET life POST_BUILD COMMAND cp -a ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:life>
)

# ------------------------------------------------------------------------------
# libs
# ------------------------------------------------------------------------------

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

include_directories(
	${OPENGL_INCLUDE_DIR}
)

target_link_libraries(
	life
	${OPENGL_gl_LIBRARY}
	GLU
	glfw
)
