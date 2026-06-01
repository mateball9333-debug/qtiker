cmake_minimum_required(VERSION 3.16)
project(qtiker VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

configure_file(qtiker.desktop.in qtiker.desktop @ONLY)
configure_file(io.github.mateball9333.qtiker.metainfo.xml.in io.github.mateball9333.qtiker.metainfo.xml @ONLY)

add_executable(qtiker main.cpp)
target_link_libraries(qtiker Qt6::Widgets)
qt_add_resources(qtiker "app_resources"
    PREFIX "/"
    FILES
        assets/qtiker-64.png
        assets/tux.png
)

install(TARGETS qtiker RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/qtiker.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/io.github.mateball9333.qtiker.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES assets/qtiker-64.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/64x64/apps RENAME qtiker.png)
install(FILES assets/qtiker-256.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps RENAME qtiker.png)
install(FILES README.md THIRD_PARTY.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(FILES LICENSE THIRD_PARTY.md DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/qtiker)
