cmake_minimum_required(VERSION 3.16)
project(qtiker VERSION 0.1.2 LANGUAGES CXX)

include(GNUInstallDirs)
include(CTest)

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)
qt_standard_project_setup()

configure_file(qtiker.desktop.in qtiker.desktop @ONLY)
configure_file(io.github.mateball9333.qtiker.metainfo.xml.in io.github.mateball9333.qtiker.metainfo.xml @ONLY)

add_executable(qtiker
    main.cpp
    clicker.cpp
    clicker.h
    gamestate.h
    game_rules.h
    appversion.h
)
target_link_libraries(qtiker Qt6::Widgets Qt6::Svg)
qt_add_resources(qtiker "app_resources"
    PREFIX "/"
    FILES
        assets/qtiker-64.png
        assets/tux.png
        assets/ui/add.svg
        assets/ui/changed.svg
        assets/ui/inbox.svg
        assets/ui/remove.svg
        assets/ui/settings.svg
)

if(BUILD_TESTING)
    add_executable(qtiker_state_tests tests/gamestate_test.cpp)
    target_include_directories(qtiker_state_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
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
