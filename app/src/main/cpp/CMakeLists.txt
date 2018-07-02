cmake_minimum_required(VERSION 2.6)

project(kasui)

option(ANDROID "Android build" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
if (ANDROID)
    add_definitions("-DANDROID_NDK")
endif()

find_package(ZLIB REQUIRED)

add_subdirectory(libpng)
add_subdirectory(guava2d)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PNG_INCLUDE_DIR})

set(KASUI_LIBRARIES
    guava2d
    ${PNG_LIBRARY}
    ${ZLIB_LIBRARIES})

if (ANDROID)
    list(APPEND KASUI_LIBRARIES
        android
        log
        EGL
        GLESv3)

    include_directories(
        ${ANDROID_NDK}/sources/android/native_app_glue)
else()
    find_package(SDL REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(PNG REQUIRED)

    list(APPEND KASUI_LIBRARIES
        ${SDL_LIBRARY}
        ${GLEW_LIBRARY}
        ${OPENGL_LIBRARIES})

    include_directories(
        ${SDL_INCLUDE_DIR}
        ${GLEW_INCLUDE_DIR})
endif()

find_package(BISON REQUIRED)

link_directories(
    ${CMAKE_BINARY_DIR}/guava2d)

set(KASUI_SOURCES
    action.cpp
    background.cpp
    bakudan_sprite.cpp
    clouds_theme.cpp
    combo_sprite.cpp
    credits.cpp
    falling_leaves_theme.cpp
    flowers_theme.cpp
    hint_animation.cpp
    hiscore_input.cpp
    hiscore_list.cpp
    http_request.cpp
    in_game.cpp
    in_game_menu.cpp
    jukugo.cpp
    jukugo_info_sprite.cpp
    kanji_info.cpp
    kasui.cpp
    leaderboard_page.cpp
    line_splitter.cpp
    main_menu.cpp
    menu.cpp
    options.cpp
    panic.cpp
    pause_button.cpp
    program_manager.cpp
    render.cpp
    sakura.cpp
    score_display.cpp
    settings_lexer.cpp
    settings_parser.cpp
    sprite.cpp
    sprite_manager.cpp
    stats_page.cpp
    text_box.cpp
    theme.cpp
    timer_display.cpp
    title_background.cpp
    tutorial.cpp
    utf8.cpp
    world.cpp
    fonts.cpp)

if(ANDROID)
    list(APPEND KASUI_SOURCES
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
        android_main.cpp)
else()
    list(APPEND KASUI_SOURCES
        sound.cpp
        main.cpp)
endif()

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/settings_parser.cpp ${CMAKE_CURRENT_SOURCE_DIR}/settings_parser.h
                   COMMAND ${BISON_EXECUTABLE} --defines=${CMAKE_CURRENT_SOURCE_DIR}/settings_parser.h -t ${CMAKE_CURRENT_SOURCE_DIR}/settings.y -o ${CMAKE_CURRENT_SOURCE_DIR}/settings_parser.cpp
                   MAIN_DEPENDENCY settings.y
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if (ANDROID)
    add_library(kasui SHARED ${KASUI_SOURCES})
else()
    add_executable(kasui ${KASUI_SOURCES})
endif()

target_link_libraries(
    kasui
    ${KASUI_LIBRARIES})

if (NOT ANDROID)
    set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/../assets")

    add_custom_command(TARGET kasui POST_BUILD
        COMMAND ln -sf ${ASSETS_DIR} ${CMAKE_CURRENT_BINARY_DIR}/assets)
endif()
