cmake_minimum_required(VERSION 3.16)
project(qtiker VERSION 0.2.1 LANGUAGES CXX)

include(GNUInstallDirs)
include(CTest)

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)
qt_standard_project_setup()

configure_file(qtiker.desktop.in qtiker.desktop @ONLY)
configure_file(io.github.mateball9333.qtiker.metainfo.xml.in io.github.mateball9333.qtiker.metainfo.xml @ONLY)

add_executable(qtiker
    src/main.cpp
    src/clicker.cpp
    src/clicker.h
    src/game_shell.cpp
    src/game_shell.h
    src/gacha_dialog.cpp
    src/gacha_dialog.h
    src/gamestate.h
    src/game_rules.h
    src/legacy_clicker.cpp
    src/legacy_clicker.h
    src/legacy_gamestate.h
    src/particle_overlay.cpp
    src/particle_overlay.h
    src/release_notes.h
    src/appversion.h
)
target_link_libraries(qtiker Qt6::Widgets Qt6::Svg)
qt_add_resources(qtiker "app_resources"
    PREFIX "/"
    FILES
        assets/qtiker-64.png
        assets/tux.png
        assets/ui/add.svg
        assets/ui/carat.png
        assets/ui/changed.svg
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
)

if(BUILD_TESTING)
    add_executable(qtiker_state_tests tests/gamestate_test.cpp)
    target_include_directories(qtiker_state_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(qtiker_state_tests Qt6::Core)
    add_test(NAME gamestate COMMAND qtiker_state_tests)

    add_test(
        NAME ui_theme_policy
        COMMAND ${CMAKE_COMMAND}
            -DPROJECT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/ui_theme_policy.cmake
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
