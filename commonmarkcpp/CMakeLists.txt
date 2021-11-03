# Copyright (c) 2021  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/project/commonmarkcpp
# contact@m2osw.com
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

##
## generate entities
##
project(generate_entities)

add_executable(generate_entities
    generate_entities.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${ADVGETOPT_INCLUDE_DIRS}
        ${EVENTDISPATCHER_INCLUDE_DIRS}
        ${LIBEXCEPT_INCLUDE_DIRS}
        ${LIBUTF8_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    ${ADVGETOPT_LIBRARIES}
    ${EVENTDISPATCHER_LIBRARIES}
    ${LIBEXCEPT_LIBRARIES}
    ${LIBUTF8_LIBRARIES}
)


##
##
##
project(commonmark_entities)

set(ENTITIES_CPP ${PROJECT_BINARY_DIR}/commonmark_entities.cpp)

add_custom_command(
    OUTPUT ${ENTITIES_CPP}
    COMMAND generate_entities --output ${ENTITIES_CPP} entities.json
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/entities.json
    DEPENDS generate_entities
)

add_custom_target(${PROJECT_NAME}
    ALL DEPENDS ${ENTITIES_CPP}
)

define_property(SOURCE
    PROPERTY GENERATED
    BRIEF_DOCS "The commonmark_entities.cpp file is a table of all the entities supported in MarkDown."
    FULL_DOCS "The file is used to know whether a named entity is valid."
)


##
## commonmarkcpp library
##
project(commonmarkcpp)

# Put the version in the header file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

add_library(${PROJECT_NAME} SHARED
    block.cpp
    commonmark.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/commonmark_entities.cpp
    link.cpp
    version.cpp

    ${ENTITIES_CPP}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${LIBEXCEPT_INCLUDE_DIRS}
        ${LIBUTF8_INCLUDE_DIRS}
        ${SNAPLOGGER_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    ${LIBEXCEPT_LIBRARIES}
    ${LIBUTF8_LIBRARIES}
    ${SNAPLOGGER_LIBRARIES}
)


set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION
        ${COMMONMARKCPP_VERSION_MAJOR}.${COMMONMARKCPP_VERSION_MINOR}

    SOVERSION
        ${COMMONMARKCPP_VERSION_MAJOR}
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        bin

    LIBRARY DESTINATION
        lib

    ARCHIVE DESTINATION
        lib
)

install(
    FILES
        block.h
        character.h
        commonmark.h
        exception.h
        link.h
        ${CMAKE_CURRENT_BINARY_DIR}/version.h

    DESTINATION
        include/commonmarkcpp
)


# vim: ts=4 sw=4 et
