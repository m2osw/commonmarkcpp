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


// libutf8 lib
//
#include    <libutf8/json_tokens.h>


// snapdev lib
//
#include    <snapdev/file_contents.h>


// C lib
//
//#include    <unistd.h>



CATCH_TEST_CASE("commonmark_thematic_breaks", "[direct-test][block]")
{
    CATCH_START_SECTION("cm: test \"***\" -> <hr/>")
    {
        cm::features f;
        f.set_add_document_div();
        f.set_add_classes();
        cm::commonmark md;
        md.set_features(f);
        CATCH_REQUIRE(md.process("***") == "<div class=\"cm-document\"><hr class=\"cm-break-asterisk\"/></div>");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: test \"***\" -> <hr /> (with a space)")
    {
        cm::features f;
        f.set_add_document_div();
        f.set_add_classes();
        f.set_add_space_in_empty_tag();
        cm::commonmark md;
        md.set_features(f);
        CATCH_REQUIRE(md.process("***") == "<div class=\"cm-document\"><hr class=\"cm-break-asterisk\" /></div>");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: test \"***\" -> <hr /> (space & no class)")
    {
        cm::features f;
        f.set_add_document_div();
        f.set_add_space_in_empty_tag();
        cm::commonmark md;
        md.set_features(f);
        CATCH_REQUIRE(md.process("***") == "<div><hr /></div>");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: test \"***\\n\" -> <hr/>")
    {
        cm::features f;
        f.set_add_classes();
        cm::commonmark md;
        md.set_features(f);
        CATCH_REQUIRE(md.process("***\n") == "<hr class=\"cm-break-asterisk\"/>");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: test \"   ***\\r\\n\" -> <hr/>")
    {
        cm::features f;
        f.set_add_classes();
        cm::commonmark md;
        md.set_features(f);
        CATCH_REQUIRE(md.process("   ***\r\n") == "<hr class=\"cm-break-asterisk\"/>");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: test \"---\" -> <hr/>")
    {
        cm::features f;
        f.set_add_classes();
        cm::commonmark md;
        md.set_features(f);
        CATCH_REQUIRE(md.process("---") == "<hr class=\"cm-break-dash\"/>");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: test \"  ---\\n\" -> <hr/>")
    {
        cm::features f;
        f.set_add_classes();
        cm::commonmark md;
        md.set_features(f);
        CATCH_REQUIRE(md.process("  ---\n") == "<hr class=\"cm-break-dash\"/>");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: test \"---\\r\\n\" -> <hr/>")
    {
        cm::features f;
        f.set_add_classes();
        cm::commonmark md;
        md.set_features(f);
        CATCH_REQUIRE(md.process("---\r\n") == "<hr class=\"cm-break-dash\"/>");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: test \" ___\" -> <hr/>")
    {
        cm::features f;
        f.set_add_classes();
        cm::commonmark md;
        md.set_features(f);
        CATCH_REQUIRE(md.process(" ___") == "<hr class=\"cm-break-underline\"/>");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: test \"___ \\t\\n\" -> <hr/>")
    {
        cm::features f;
        f.set_add_classes();
        cm::commonmark md;
        md.set_features(f);
        CATCH_REQUIRE(md.process("___ \t\n") == "<hr class=\"cm-break-underline\"/>");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: test \"_ _ _\\r\\n\" -> <hr/>")
    {
        cm::features f;
        f.set_add_classes();
        cm::commonmark md;
        md.set_features(f);
        CATCH_REQUIRE(md.process("_ _ _\r\n") == "<hr class=\"cm-break-underline\"/>");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("commonmark_atx_heading", "[direct-test][block]")
{
    CATCH_START_SECTION("cm: test \"#... ...\" -> <hN/>")
    {
        cm::features f;
        f.set_add_classes();
        cm::commonmark md;
        md.set_features(f);
        CATCH_REQUIRE(md.process("# H1\n## H2\n### H3\n#### H4\n##### H5\n###### H6\n")
                == "<h1 class=\"cm-header-open\">H1</h1>"
                   "<h2 class=\"cm-header-open\">H2</h2>"
                   "<h3 class=\"cm-header-open\">H3</h3>"
                   "<h4 class=\"cm-header-open\">H4</h4>"
                   "<h5 class=\"cm-header-open\">H5</h5>"
                   "<h6 class=\"cm-header-open\">H6</h6>");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: test \"#... ... #...\" -> <hN/>")
    {
        cm::features f;
        f.set_add_classes();
        cm::commonmark md;
        md.set_features(f);
        CATCH_REQUIRE(md.process("# H1 # # #\n## H2 #\t#\t#\n### H3 ###   \n"
            "#### H4 ########## #########\n##### H5 ##\n###### H6 # \t \t\n")
                == "<h1 class=\"cm-header-enclosed\">H1 # #</h1>"
                   "<h2 class=\"cm-header-enclosed\">H2 #\t#</h2>"
                   "<h3 class=\"cm-header-enclosed\">H3</h3>"
                   "<h4 class=\"cm-header-enclosed\">H4 ##########</h4>"
                   "<h5 class=\"cm-header-enclosed\">H5</h5>"
                   "<h6 class=\"cm-header-enclosed\">H6</h6>");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("commonmark_test_suite", "[test-suite]")
{
    CATCH_START_SECTION("cm: run against commonmark test suite")
    {
        // TODO: allow for command line to change this path
        //
        // file found on the commonmark website:
        //   https://spec.commonmark.org/0.30/
        //
        snap::file_contents spec("tests/spec.json");
        CATCH_REQUIRE(spec.read_all());
        libutf8::json_tokens json(spec.contents());

        CATCH_REQUIRE(json.next_token() == libutf8::token_t::TOKEN_OPEN_ARRAY);

        libutf8::token_t token(json.next_token());
        for(int count(1); ; ++count)
        {
            CATCH_REQUIRE(token == libutf8::token_t::TOKEN_OPEN_OBJECT);

            std::string markdown;
            std::string html;
            for(;;)
            {
                token = json.next_token();
                CATCH_REQUIRE(token == libutf8::token_t::TOKEN_STRING);
                std::string const field_name(json.string());

                CATCH_REQUIRE(json.next_token() == libutf8::token_t::TOKEN_COLON);

                token = json.next_token();
                if(field_name == "markdown")
                {
                    CATCH_REQUIRE(token == libutf8::token_t::TOKEN_STRING);
                    markdown = json.string();
                }
                else if(field_name == "html")
                {
                    CATCH_REQUIRE(token == libutf8::token_t::TOKEN_STRING);
                    html = json.string();
                }
                //else -- ignore that value

                token = json.next_token();
                if(token == libutf8::token_t::TOKEN_CLOSE_OBJECT)
                {
                    break;
                }
                CATCH_REQUIRE(token == libutf8::token_t::TOKEN_COMMA);
            }

            // run that test
            {
                cm::features f;
                f.set_commonmark_compatible();
                cm::commonmark md;
                md.set_features(f);

                std::cout << "markdown #" << count << ": [";
                for(auto c : markdown)
                {
                    // the characters used to show the white spaces are
                    // otfen wider so I add a space after to make it
                    // clearer in a console
                    //
                    switch(c)
                    {
                    case ' ':
                        std::cout << "\xE2\x8E\xB5 ";
                        break;

                    case '\t':
                        std::cout << "\xE2\x87\xA5 ";
                        break;

                    case '\n':
                        std::cout << "\xE2\x86\xB5 ";
                        break;

                    case '\r':
                        std::cout << "\\r";
                        break;

                    default:
                        std::cout << c;
                        break;

                    }
                }
                std::cout << "]\n";

                std::string const result(md.process(markdown));

// while building the parser, a REQUIRE is way more practical as it won't
// output hundred of errors before stopping the test
#if 0
                CATCH_CHECK(result == html);
#else
                CATCH_REQUIRE(result == html);
#endif
                std::cout << " |\n";
                std::cout << " +---> resulting html: [" << html << "] matched!\n";
            }

            token = json.next_token();
            if(token != libutf8::token_t::TOKEN_COMMA)
            {
                break;
            }

            token = json.next_token();
        }

        CATCH_REQUIRE(token == libutf8::token_t::TOKEN_CLOSE_ARRAY);
        CATCH_REQUIRE(json.next_token() == libutf8::token_t::TOKEN_END);

        std::cerr << "error: actually write the test...\n";
        CATCH_REQUIRE(false);
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
