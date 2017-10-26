find_package(SDL REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PNG REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}
	${SDL_INCLUDE_DIR}
	${GLEW_INCLUDE_DIR}
	${PNG_INCLUDE_DIRS})

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
    main.cpp
    main_menu.cpp
    menu.cpp
    options.cpp
    panic.cpp
    pause_button.cpp
    program_manager.cpp
    program_wrapper.cpp
    render.cpp
    sakura.cpp
    score_display.cpp
    settings_lexer.cpp
    settings_parser.cpp
    sound.cpp
    sprite.cpp
    sprite_manager.cpp
    stats_page.cpp
    text_box.cpp
    theme.cpp
    timer_display.cpp
    title_background.cpp
    tutorial.cpp
    utf8.cpp
    world.cpp)

add_executable(kasui ${KASUI_SOURCES})

target_link_libraries(
	kasui
    guava2d
	${SDL_LIBRARY}
	${GLEW_LIBRARY}
	${OPENGL_LIBRARIES}
	${PNG_LIBRARIES})

set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")

add_custom_command(TARGET kasui POST_BUILD
    COMMAND ln -sf ${ASSETS_DIR} ${CMAKE_CURRENT_BINARY_DIR}/assets)
