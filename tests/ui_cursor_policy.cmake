# SPDX-License-Identifier: GPL-2.0-or-later
set(source_root "${PROJECT_SOURCE_DIR}")

file(GLOB_RECURSE ui_sources "${source_root}/src/*.cpp")

foreach(src IN LISTS ui_sources)
    file(READ "${src}" content)
    string(FIND "${content}" "setCursor" match_index)
    if(NOT match_index EQUAL -1)
        string(REGEX MATCH ".{0,60}setCursor.{0,60}" snippet "${content}")
        message(FATAL_ERROR "setCursor found in ${src}: ...${snippet}...")
    endif()
endforeach()
