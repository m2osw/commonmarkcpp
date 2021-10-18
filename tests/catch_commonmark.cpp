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
#include    <commonmarkcpp/commonmark.h>


// C lib
//
//#include    <unistd.h>



CATCH_TEST_CASE("commonmark_thematic_breaks", "[direct-test][block]")
{
    CATCH_START_SECTION("cm: test \"***\" -> <hr/>")
    {
        cm::commonmark md;
        md.add_classes();
        md.add_input("***");
        md.process();
        md.flush();
        CATCH_REQUIRE(md.get_output() == "<hr class=\"cm-break-asterisk\"/>");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: test \"***\\n\" -> <hr/>")
    {
        cm::commonmark md;
        md.add_classes();
        md.add_input("***\n");
        md.process();
        md.flush();
        CATCH_REQUIRE(md.get_output() == "<hr class=\"cm-break-asterisk\"/>");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: test \"***\\r\\n\" -> <hr/>")
    {
        cm::commonmark md;
        md.add_classes();
        md.add_input("   ***\r\n");
        md.process();
        md.flush();
        CATCH_REQUIRE(md.get_output() == "<hr class=\"cm-break-asterisk\"/>");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: test \"---\" -> <hr/>")
    {
        cm::commonmark md;
        md.add_classes();
        md.add_input("---");
        md.process();
        md.flush();
        CATCH_REQUIRE(md.get_output() == "<hr class=\"cm-break-dash\"/>");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: test \"---\\n\" -> <hr/>")
    {
        cm::commonmark md;
        md.add_classes();
        md.add_input("  ---\n");
        md.process();
        md.flush();
        CATCH_REQUIRE(md.get_output() == "<hr class=\"cm-break-dash\"/>");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: test \"---\\r\\n\" -> <hr/>")
    {
        cm::commonmark md;
        md.add_classes();
        md.add_input("---\r\n");
        md.process();
        md.flush();
        CATCH_REQUIRE(md.get_output() == "<hr class=\"cm-break-dash\"/>");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: test \"___\" -> <hr/>")
    {
        cm::commonmark md;
        md.add_classes();
        md.add_input(" ___");
        md.process();
        md.flush();
        CATCH_REQUIRE(md.get_output() == "<hr class=\"cm-break-underline\"/>");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: test \"___ \\t\\n\" -> <hr/>")
    {
        cm::commonmark md;
        md.add_classes();
        md.add_input("___ \t\n");
        md.process();
        md.flush();
        CATCH_REQUIRE(md.get_output() == "<hr class=\"cm-break-underline\"/>");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: test \"_ _ _\\r\\n\" -> <hr/>")
    {
        cm::commonmark md;
        md.add_classes();
        md.add_input("_ _ _\r\n");
        md.process();
        md.flush();
        CATCH_REQUIRE(md.get_output() == "<hr class=\"cm-break-underline\"/>");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("commonmark_atx_heading", "[direct-test][block]")
{
    CATCH_START_SECTION("cm: test \"#... ...\" -> <hN/>")
    {
        cm::commonmark md;
        md.add_classes();
        md.add_input("# H1\n## H2\n### H3\n#### H4\n##### H5\n###### H6\n");
        md.process();
        md.flush();
        CATCH_REQUIRE(md.get_output() == "<h1 class=\"cm-open\">H1</h1>"
                                         "<h2 class=\"cm-open\">H2</h2>"
                                         "<h3 class=\"cm-open\">H3</h3>"
                                         "<h4 class=\"cm-open\">H4</h4>"
                                         "<h5 class=\"cm-open\">H5</h5>"
                                         "<h6 class=\"cm-open\">H6</h6>");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: test \"#... ... #...\" -> <hN/>")
    {
        cm::commonmark md;
        md.add_classes();
        md.add_input("# H1 # # #\n## H2 #\t#\t#\n### H3 ###   \n"
            "#### H4 ########## #########\n##### H5 ##\n###### H6 # \t \t\n");
        md.process();
        md.flush();
        CATCH_REQUIRE(md.get_output() == "<h1 class=\"cm-enclosed\">H1 # #</h1>"
                                         "<h2 class=\"cm-enclosed\">H2 #\t#</h2>"
                                         "<h3 class=\"cm-enclosed\">H3</h3>"
                                         "<h4 class=\"cm-enclosed\">H4 ##########</h4>"
                                         "<h5 class=\"cm-enclosed\">H5</h5>"
                                         "<h6 class=\"cm-enclosed\">H6</h6>");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("commonmark_test_suite", "[test-suite]")
{
    CATCH_START_SECTION("cm: run against commonmark test suite")
    {
        std::cerr << "error: actually write the test...\n";
        CATCH_REQUIRE(false);
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
