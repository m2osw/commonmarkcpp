// Copyright (c) 2021  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/commonmarkcpp
// contact@m2osw.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

// self
//
#include    "catch_main.h"


// commonmarkcpp lib
//
#include    <commonmarkcpp/commonmarkcpp.h>


// C lib
//
//#include    <unistd.h>



CATCH_TEST_CASE("commonmarkcpp", "[test-suite]")
{
    CATCH_START_SECTION("cm: run against commonmark test suite")
    {
        std::cerr << "error: actually write the test...\n";
        CATCH_REQUIRE(false);
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
