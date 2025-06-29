// Copyright (c) 2021-2025  Made to Order Software Corp.  All Rights Reserved
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


// commonmarkcpp
//
#include    <commonmarkcpp/character.h>


// C
//
//#include    <unistd.h>



CATCH_TEST_CASE("character", "[test-suite]")
{
    CATCH_START_SECTION("cm: invalid character (null)")
    {
        cm::character c{};
        CATCH_REQUIRE(c.is_null());
        c.fix_null();
        CATCH_REQUIRE_FALSE(c.is_null());
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: test basic characters")
    {
        cm::character c{};

        // by default, it's a 'null'
        CATCH_REQUIRE(c.is_null());
        CATCH_REQUIRE_FALSE(c.is_tab());
        CATCH_REQUIRE_FALSE(c.is_space());
        CATCH_REQUIRE_FALSE(c.is_eos());
        CATCH_REQUIRE_FALSE(c.is_carriage_return());
        CATCH_REQUIRE_FALSE(c.is_eol());
        CATCH_REQUIRE_FALSE(c.is_thematic_break());
        CATCH_REQUIRE_FALSE(c.is_dash());
        CATCH_REQUIRE_FALSE(c.is_period());
        CATCH_REQUIRE_FALSE(c.is_setext());
        CATCH_REQUIRE_FALSE(c.is_hash());
        CATCH_REQUIRE_FALSE(c.is_close_parenthesis());
        CATCH_REQUIRE_FALSE(c.is_asterisk());
        CATCH_REQUIRE_FALSE(c.is_plus());
        CATCH_REQUIRE_FALSE(c.is_semicolon());
        CATCH_REQUIRE_FALSE(c.is_backslash());
        CATCH_REQUIRE_FALSE(c.is_ascii_punctuation());

        c.f_char = U'\t';
        CATCH_REQUIRE_FALSE(c.is_null());
        CATCH_REQUIRE(c.is_tab());
        CATCH_REQUIRE_FALSE(c.is_space());
        CATCH_REQUIRE_FALSE(c.is_eos());
        CATCH_REQUIRE_FALSE(c.is_carriage_return());
        CATCH_REQUIRE_FALSE(c.is_eol());
        CATCH_REQUIRE_FALSE(c.is_thematic_break());
        CATCH_REQUIRE_FALSE(c.is_dash());
        CATCH_REQUIRE_FALSE(c.is_period());
        CATCH_REQUIRE_FALSE(c.is_setext());
        CATCH_REQUIRE_FALSE(c.is_hash());
        CATCH_REQUIRE_FALSE(c.is_close_parenthesis());
        CATCH_REQUIRE_FALSE(c.is_asterisk());
        CATCH_REQUIRE_FALSE(c.is_plus());
        CATCH_REQUIRE_FALSE(c.is_semicolon());
        CATCH_REQUIRE_FALSE(c.is_backslash());
        CATCH_REQUIRE_FALSE(c.is_ascii_punctuation());

        c.f_char = U' ';
        CATCH_REQUIRE_FALSE(c.is_null());
        CATCH_REQUIRE_FALSE(c.is_tab());
        CATCH_REQUIRE(c.is_space());
        CATCH_REQUIRE_FALSE(c.is_eos());
        CATCH_REQUIRE_FALSE(c.is_carriage_return());
        CATCH_REQUIRE_FALSE(c.is_eol());
        CATCH_REQUIRE_FALSE(c.is_thematic_break());
        CATCH_REQUIRE_FALSE(c.is_dash());
        CATCH_REQUIRE_FALSE(c.is_period());
        CATCH_REQUIRE_FALSE(c.is_setext());
        CATCH_REQUIRE_FALSE(c.is_hash());
        CATCH_REQUIRE_FALSE(c.is_close_parenthesis());
        CATCH_REQUIRE_FALSE(c.is_asterisk());
        CATCH_REQUIRE_FALSE(c.is_plus());
        CATCH_REQUIRE_FALSE(c.is_semicolon());
        CATCH_REQUIRE_FALSE(c.is_backslash());
        CATCH_REQUIRE_FALSE(c.is_ascii_punctuation());

        c.f_char = libutf8::EOS;
        CATCH_REQUIRE_FALSE(c.is_null());
        CATCH_REQUIRE_FALSE(c.is_tab());
        CATCH_REQUIRE_FALSE(c.is_space());
        CATCH_REQUIRE(c.is_eos());
        CATCH_REQUIRE_FALSE(c.is_carriage_return());
        CATCH_REQUIRE_FALSE(c.is_eol());
        CATCH_REQUIRE_FALSE(c.is_thematic_break());
        CATCH_REQUIRE_FALSE(c.is_dash());
        CATCH_REQUIRE_FALSE(c.is_period());
        CATCH_REQUIRE_FALSE(c.is_setext());
        CATCH_REQUIRE_FALSE(c.is_hash());
        CATCH_REQUIRE_FALSE(c.is_close_parenthesis());
        CATCH_REQUIRE_FALSE(c.is_asterisk());
        CATCH_REQUIRE_FALSE(c.is_plus());
        CATCH_REQUIRE_FALSE(c.is_semicolon());
        CATCH_REQUIRE_FALSE(c.is_backslash());
        CATCH_REQUIRE_FALSE(c.is_ascii_punctuation());

        c.f_char = U'\r';
        CATCH_REQUIRE_FALSE(c.is_null());
        CATCH_REQUIRE_FALSE(c.is_tab());
        CATCH_REQUIRE_FALSE(c.is_space());
        CATCH_REQUIRE_FALSE(c.is_eos());
        CATCH_REQUIRE(c.is_carriage_return());
        CATCH_REQUIRE_FALSE(c.is_eol());
        CATCH_REQUIRE_FALSE(c.is_thematic_break());
        CATCH_REQUIRE_FALSE(c.is_dash());
        CATCH_REQUIRE_FALSE(c.is_period());
        CATCH_REQUIRE_FALSE(c.is_setext());
        CATCH_REQUIRE_FALSE(c.is_hash());
        CATCH_REQUIRE_FALSE(c.is_close_parenthesis());
        CATCH_REQUIRE_FALSE(c.is_asterisk());
        CATCH_REQUIRE_FALSE(c.is_plus());
        CATCH_REQUIRE_FALSE(c.is_semicolon());
        CATCH_REQUIRE_FALSE(c.is_backslash());
        CATCH_REQUIRE_FALSE(c.is_ascii_punctuation());

        c.f_char = U'\n';
        CATCH_REQUIRE_FALSE(c.is_null());
        CATCH_REQUIRE_FALSE(c.is_tab());
        CATCH_REQUIRE_FALSE(c.is_space());
        CATCH_REQUIRE_FALSE(c.is_eos());
        CATCH_REQUIRE_FALSE(c.is_carriage_return());
        CATCH_REQUIRE(c.is_eol());
        CATCH_REQUIRE_FALSE(c.is_thematic_break());
        CATCH_REQUIRE_FALSE(c.is_dash());
        CATCH_REQUIRE_FALSE(c.is_period());
        CATCH_REQUIRE_FALSE(c.is_setext());
        CATCH_REQUIRE_FALSE(c.is_hash());
        CATCH_REQUIRE_FALSE(c.is_close_parenthesis());
        CATCH_REQUIRE_FALSE(c.is_asterisk());
        CATCH_REQUIRE_FALSE(c.is_plus());
        CATCH_REQUIRE_FALSE(c.is_semicolon());
        CATCH_REQUIRE_FALSE(c.is_backslash());
        CATCH_REQUIRE_FALSE(c.is_ascii_punctuation());

        std::string const ascii_punctuation("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~");
        char32_t f = U'a';
        for(auto p : ascii_punctuation)
        {
            c.f_char = static_cast<char32_t>(p);

            // ASCII punctuation
            //
            CATCH_REQUIRE(c.is_ascii_punctuation());

            // non-punctuation
            //
            CATCH_REQUIRE_FALSE(c.is_null());
            CATCH_REQUIRE_FALSE(c.is_tab());
            CATCH_REQUIRE_FALSE(c.is_space());
            CATCH_REQUIRE_FALSE(c.is_eos());
            CATCH_REQUIRE_FALSE(c.is_carriage_return());
            CATCH_REQUIRE_FALSE(c.is_eol());
            CATCH_REQUIRE_FALSE(c.is_digit());
            CATCH_REQUIRE_FALSE(c.is_hexdigit());

            // specialized punctuation
            //
            if(p == '*'
            || p == '-'
            || p == '_')
            {
                CATCH_REQUIRE(c.is_thematic_break());
            }
            else
            {
                CATCH_REQUIRE_FALSE(c.is_thematic_break());
            }
            if(p == '='
            || p == '-')
            {
                CATCH_REQUIRE(c.is_setext());
            }
            else
            {
                CATCH_REQUIRE_FALSE(c.is_setext());
            }

            // simple punctuation
            //
            if(p == '-')
            {
                CATCH_REQUIRE(c.is_dash());
            }
            else
            {
                CATCH_REQUIRE_FALSE(c.is_dash());
            }
            if(p == '.')
            {
                CATCH_REQUIRE(c.is_period());
            }
            else
            {
                CATCH_REQUIRE_FALSE(c.is_period());
            }
            if(p == '#')
            {
                CATCH_REQUIRE(c.is_hash());
            }
            else
            {
                CATCH_REQUIRE_FALSE(c.is_hash());
            }
            if(p == ')')
            {
                CATCH_REQUIRE(c.is_close_parenthesis());
            }
            else
            {
                CATCH_REQUIRE_FALSE(c.is_close_parenthesis());
            }
            if(p == '*')
            {
                CATCH_REQUIRE(c.is_asterisk());
            }
            else
            {
                CATCH_REQUIRE_FALSE(c.is_asterisk());
            }
            if(p == '+')
            {
                CATCH_REQUIRE(c.is_plus());
            }
            else
            {
                CATCH_REQUIRE_FALSE(c.is_plus());
            }
            if(p == ';')
            {
                CATCH_REQUIRE(c.is_semicolon());
            }
            else
            {
                CATCH_REQUIRE_FALSE(c.is_semicolon());
            }
            if(p == '\\')
            {
                CATCH_REQUIRE(c.is_backslash());
            }
            else
            {
                CATCH_REQUIRE_FALSE(c.is_backslash());
            }

            // various compares
            //
            CATCH_REQUIRE(c == c.f_char);
            CATCH_REQUIRE(c == c);
            cm::character d{};
            CATCH_REQUIRE(c != d);
            CATCH_REQUIRE(c != f);
            d.f_char = f;
            CATCH_REQUIRE(c != d);
            d.f_char = static_cast<char32_t>(p);
            CATCH_REQUIRE(c == d);
            if(f == U'z')
            {
                f = U'a';
            }
            else
            {
                ++f;
            }
        }

        // go back to 'null' to make sure
        c.f_char = U'\0';
        CATCH_REQUIRE(c.is_null());
        CATCH_REQUIRE_FALSE(c.is_tab());
        CATCH_REQUIRE_FALSE(c.is_space());
        CATCH_REQUIRE_FALSE(c.is_eos());
        CATCH_REQUIRE_FALSE(c.is_carriage_return());
        CATCH_REQUIRE_FALSE(c.is_eol());
        CATCH_REQUIRE_FALSE(c.is_thematic_break());
        CATCH_REQUIRE_FALSE(c.is_dash());
        CATCH_REQUIRE_FALSE(c.is_period());
        CATCH_REQUIRE_FALSE(c.is_setext());
        CATCH_REQUIRE_FALSE(c.is_hash());
        CATCH_REQUIRE_FALSE(c.is_close_parenthesis());
        CATCH_REQUIRE_FALSE(c.is_asterisk());
        CATCH_REQUIRE_FALSE(c.is_plus());
        CATCH_REQUIRE_FALSE(c.is_semicolon());
        CATCH_REQUIRE_FALSE(c.is_backslash());
        CATCH_REQUIRE_FALSE(c.is_ascii_punctuation());
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: character: print hello")
    {
        cm::character::string_t str;
        cm::character c{};

        c.f_char = 'h';
        str += c;
        c.f_char = 'e';
        str += c;
        c.f_char = 'l';
        str += c;
        c.f_char = 'l';
        str += c;
        c.f_char = 'o';
        str += c;

        std::stringstream ss;
        ss << str;
        CATCH_REQUIRE(ss.str() == "hello");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cm: character: digits")
    {
        for(char32_t idx(0); idx < 0x110000; ++idx)
        {
            cm::character c{};
            c.f_char = static_cast<char32_t>(idx);
            if(idx >= '0' && idx <= '9')
            {
                CATCH_REQUIRE(c.is_digit());
                CATCH_REQUIRE(c.is_hexdigit());
            }
            else if(idx >= 'a' && idx <= 'f')
            {
                CATCH_REQUIRE_FALSE(c.is_digit());
                CATCH_REQUIRE(c.is_hexdigit());
            }
            else if(idx >= 'A' && idx <= 'F')
            {
                CATCH_REQUIRE_FALSE(c.is_digit());
                CATCH_REQUIRE(c.is_hexdigit());
            }
            else
            {
                CATCH_REQUIRE_FALSE(c.is_digit());
                CATCH_REQUIRE_FALSE(c.is_hexdigit());
            }
        }
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
