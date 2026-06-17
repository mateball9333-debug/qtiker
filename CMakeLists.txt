cmake_minimum_required(VERSION 3.16)
project(qtiker VERSION 0.9.1 LANGUAGES CXX)

include(GNUInstallDirs)
include(CTest)

find_package(Qt6 6.3 REQUIRED COMPONENTS Widgets Svg Multimedia)
qt_standard_project_setup()

configure_file(qtiker.desktop.in qtiker.desktop @ONLY)
configure_file(io.github.mateball9333.qtiker.metainfo.xml.in io.github.mateball9333.qtiker.metainfo.xml @ONLY)

add_executable(qtiker
    src/main.cpp
    src/ui/clicker.cpp
    src/ui/clicker.h
    src/ui/clicker_cards.cpp
    src/ui/clicker_economy.cpp
    src/ui/clicker_save.cpp
    src/ui/game_shell.cpp
    src/ui/game_shell.h
    src/ui/gacha_dialog.cpp
    src/ui/gacha_dialog.h
    src/ui/legacy_clicker.cpp
    src/ui/legacy_clicker.h
    src/ui/legacy_gamestate.h
    src/ui/particle_overlay.cpp
    src/ui/particle_overlay.h
    src/ui/release_notes.h
    src/ui/settings_dialog.cpp
    src/ui/settings_dialog.h
    src/ui/carat_dialog.cpp
    src/ui/carat_dialog.h
    src/ui/statistics_dialog.cpp
    src/ui/statistics_dialog.h
    src/ui/save_slots_dialog.cpp
    src/ui/save_slots_dialog.h
    src/ui/casino_dialog.cpp
    src/ui/casino_dialog.h
    src/ui/status_bar.cpp
    src/ui/status_bar.h
    src/core/gamestate.h
    src/core/game_rules.h
    src/core/cards.h
    src/core/utils.h
    src/core/svg_utils.h
    src/core/appversion.h
)
target_include_directories(qtiker PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(qtiker Qt6::Widgets Qt6::Svg Qt6::Multimedia)

set(ResourceFiles
    assets/qtiker-64.png
    assets/tux.png
    assets/ui/add.svg
    assets/ui/carat.png
    assets/ui/changed.svg
    assets/ui/casino.svg
    assets/ui/clock.svg
    assets/ui/info.svg
    assets/ui/release-notes.svg
    assets/ui/remove.svg
    assets/ui/reset.svg
    assets/ui/settings.svg
    assets/ui/statistics.svg
    assets/ui/stat-down-double.svg
    assets/ui/stat-down.svg
    assets/ui/stat-neutral.svg
    assets/ui/stat-up-double.svg
    assets/ui/stat-up.svg
    assets/ui/bolt.svg
    assets/ui/bookmark.svg
    assets/ui/save.svg
    assets/ui/candle.svg
    assets/ui/hourglass.svg
    assets/ui/snowflake.svg
    assets/ui/alert.svg
    assets/ui/apple.svg
    assets/ui/bug.svg
    assets/ui/delete.svg
    assets/ui/diamond.svg
    assets/ui/dna.svg
    assets/ui/language.svg
    assets/ui/mystery.svg
    assets/sound/click.wav
    assets/sound/buy.wav
    assets/sound/crit.wav
    assets/sound/burn.wav
    assets/sound/buff.wav
    assets/sound/unlock.wav
    assets/sound/reel_stop.wav
    assets/sound/win.wav
)

file(GLOB LoreTxts RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "assets/かかかかかかかかかかかかかかかかかかかかかかかかかかかか/*.txt"
)
list(APPEND ResourceFiles ${LoreTxts})

qt_add_resources(qtiker "app_resources"
    PREFIX "/"
    FILES ${ResourceFiles}
)

if(BUILD_TESTING)
    add_executable(qtiker_state_tests tests/gamestate_test.cpp)
    target_include_directories(qtiker_state_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
    target_link_libraries(qtiker_state_tests Qt6::Core)
    add_test(NAME gamestate COMMAND qtiker_state_tests)

    add_test(
        NAME ui_theme_policy
        COMMAND ${CMAKE_COMMAND}
            -DPROJECT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/ui_theme_policy.cmake
    )
    add_test(
        NAME ui_cursor_policy
        COMMAND ${CMAKE_COMMAND}
            -DPROJECT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/ui_cursor_policy.cmake
    )
endif()

install(TARGETS qtiker RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/qtiker.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/io.github.mateball9333.qtiker.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES assets/qtiker-64.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/64x64/apps RENAME qtiker.png)
install(FILES assets/qtiker-256.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps RENAME qtiker.png)
install(FILES README.md THIRD_PARTY.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(FILES LICENSE THIRD_PARTY.md DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/qtiker)
