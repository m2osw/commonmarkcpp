# - Try to find commonmarkcpp
#
# Once done this will define
#
# COMMONMARKCPP_FOUND        - System has commonmarkcpp
# COMMONMARKCPP_INCLUDE_DIRS - The commonmarkcpp include directories
# COMMONMARKCPP_LIBRARIES    - The libraries needed to use commonmarkcpp (none)
# COMMONMARKCPP_DEFINITIONS  - Compiler switches required for using commonmarkcpp (none)
#
# License:
#
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

find_path(
    COMMONMARKCPP_INCLUDE_DIR
        commonmarkcpp/version.h

    PATHS
        $ENV{COMMONMARKCPP_INCLUDE_DIR}
)

find_library(
    COMMONMARKCPP_LIBRARY
        commonmarkcpp

    PATHS
        $ENV{COMMONMARKCPP_LIBRARY}
)

mark_as_advanced(
    COMMONMARKCPP_INCLUDE_DIR
    COMMONMARKCPP_LIBRARY
)

set(COMMONMARKCPP_INCLUDE_DIRS ${COMMONMARKCPP_INCLUDE_DIR})
set(COMMONMARKCPP_LIBRARIES    ${COMMONMARKCPP_LIBRARY})

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set COMMONMARKCPP_FOUND to
# TRUE if all listed variables are TRUE
find_package_handle_standard_args(
    CommonMarkCpp
    DEFAULT_MSG
    COMMONMARKCPP_INCLUDE_DIR
    COMMONMARKCPP_LIBRARY
)

# vim: ts=4 sw=4 et
