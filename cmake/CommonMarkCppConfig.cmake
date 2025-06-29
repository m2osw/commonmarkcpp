# - Find Commonmark C++
#
# COMMONMARKCPP_FOUND        - System has commonmarkcpp
# COMMONMARKCPP_INCLUDE_DIRS - The commonmarkcpp include directories
# COMMONMARKCPP_LIBRARIES    - The libraries needed to use commonmarkcpp
# COMMONMARKCPP_DEFINITIONS  - Compiler switches required for using commonmarkcpp
#
# License:
#
# Copyright (c) 2021-2025  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/project/libmimemail
# contact@m2osw.com
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

find_path(
    COMMONMARKCPP_INCLUDE_DIR
        commonmarkcpp/version.h

    PATHS
        ENV COMMONMARKCPP_INCLUDE_DIR
)

find_library(
    COMMONMARKCPP_LIBRARY
        commonmarkcpp

    PATHS
        ${COMMONMARKCPP_LIBRARY_DIR}
        ENV COMMONMARKCPP_LIBRARY
)

mark_as_advanced(
    COMMONMARKCPP_INCLUDE_DIR
    COMMONMARKCPP_LIBRARY
)

set(COMMONMARKCPP_INCLUDE_DIRS ${COMMONMARKCPP_INCLUDE_DIR})
set(COMMONMARKCPP_LIBRARIES    ${COMMONMARKCPP_LIBRARY})


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    CommonMarkCpp
    REQUIRED_VARS
        COMMONMARKCPP_INCLUDE_DIR
        COMMONMARKCPP_LIBRARY
)

# vim: ts=4 sw=4 et
