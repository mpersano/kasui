find_package(SDL REQUIRED)
find_package(GLEW REQUIRED)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)

include_directories(
    ${ZLIB_INCLUDE_DIR}
	${PNG_INCLUDE_DIR})

set(GUAVA2D_SOURCES
    draw_queue.cpp
    file.cpp
    font.cpp
    font_manager.cpp
    panic.cpp
    pixmap.cpp
    program.cpp
    shader.cpp
    texture.cpp
    texture_manager.cpp
    xwchar.cpp)

add_library(guava2d ${GUAVA2D_SOURCES})
