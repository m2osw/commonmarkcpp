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
#pragma once

/** \file
 * \brief Declaration of the commonmark class.
 *
 * The commonmark is a state machine that accepts Markdown data as input
 * and spits out the corresponding HTML.
 */


// self
//


// libutf8 lib
//
#include    <libutf8/iterator.h>
#include    <libutf8/libutf8.h>


// C++ lib
//
#include    <iostream>



namespace cm
{


constexpr char32_t const    CHAR_NULL = U'\0';                      // 0000
constexpr char32_t const    CHAR_TAB = U'\t';                       // 0009
constexpr char32_t const    CHAR_LINE_FEED = U'\n';                 // 000A
constexpr char32_t const    CHAR_CARRIAGE_RETURN = U'\r';           // 000D
constexpr char32_t const    CHAR_SPACE = U' ';                      // 0020
constexpr char32_t const    CHAR_EXCLAMATION_MARK = U'!';           // 0021
constexpr char32_t const    CHAR_QUOTE = U'"';                      // 0022
constexpr char32_t const    CHAR_HASH = U'#';                       // 0023
constexpr char32_t const    CHAR_DOLLAR = U'$';                     // 0024
constexpr char32_t const    CHAR_PERCENT = U'%';                    // 0025
constexpr char32_t const    CHAR_AMPERSAND = U'&';                  // 0026
constexpr char32_t const    CHAR_APOSTROPHE = U'\'';                // 0027
constexpr char32_t const    CHAR_OPEN_PARENTHESIS = U'(';           // 0028
constexpr char32_t const    CHAR_CLOSE_PARENTHESIS = U')';          // 0029
constexpr char32_t const    CHAR_ASTERISK = U'*';                   // 002A
constexpr char32_t const    CHAR_PLUS = U'+';                       // 002B
constexpr char32_t const    CHAR_COMMA = U',';                      // 002C
constexpr char32_t const    CHAR_DASH = U'-';                       // 002D
constexpr char32_t const    CHAR_PERIOD = U'.';                     // 002E
constexpr char32_t const    CHAR_SLASH = U'/';                      // 002F
constexpr char32_t const    CHAR_COLON = U':';                      // 003A
constexpr char32_t const    CHAR_SEMICOLON = U';';                  // 003B
constexpr char32_t const    CHAR_LESS = U'<';                       // 003C
constexpr char32_t const    CHAR_EQUAL = U'=';                      // 003D
constexpr char32_t const    CHAR_GREATER = U'>';                    // 003E
constexpr char32_t const    CHAR_QUESTION_MARK = U'?';              // 003F
constexpr char32_t const    CHAR_AT = U'@';                         // 0040
constexpr char32_t const    CHAR_OPEN_SQUARE_BRACKET = U'[';        // 005B
constexpr char32_t const    CHAR_BACKSLASH = U'\\';                 // 005C
constexpr char32_t const    CHAR_CLOSE_SQUARE_BRACKET = U']';       // 005D
constexpr char32_t const    CHAR_CIRCUMFLEX = U'^';                 // 005E
constexpr char32_t const    CHAR_UNDERSCORE = U'_';                 // 005F
constexpr char32_t const    CHAR_ACUTE = U'`';                      // 0060
constexpr char32_t const    CHAR_OPEN_CURLY_BRACKET = U'{';         // 007B
constexpr char32_t const    CHAR_PIPE = U'|';                       // 007C
constexpr char32_t const    CHAR_CLOSE_CURLY_BRACKET = U'}';        // 007D
constexpr char32_t const    CHAR_TILDE = U'~';                      // 007E
constexpr char32_t const    CHAR_REPLACEMENT_CHARACTER = U'\xFFFD'; // FFFD





struct character
{
    typedef std::basic_string<character>  string_t;

    bool is_eol() const
    {
        return f_char == CHAR_LINE_FEED;
    }

    bool is_carriage_return() const
    {
        return f_char == CHAR_CARRIAGE_RETURN;
    }

    bool is_eos() const
    {
        return f_char == libutf8::EOS;
    }

    bool is_tab() const
    {
        return f_char == CHAR_TAB;
    }

    bool is_space() const
    {
        return f_char == CHAR_SPACE;
    }

    bool is_null() const
    {
        return f_char == CHAR_NULL;
    }

    void fix_null()
    {
        if(is_null())
        {
            f_char = CHAR_REPLACEMENT_CHARACTER;
        }
    }

    bool is_thematic_break() const
    {
        return f_char == CHAR_ASTERISK
            || f_char == CHAR_DASH
            || f_char == CHAR_UNDERSCORE;
    }

    bool is_dash() const
    {
        return f_char == CHAR_DASH;
    }

    bool is_setext() const
    {
        return f_char == CHAR_DASH
            || f_char == CHAR_UNDERSCORE;
    }

    bool is_hash() const
    {
        return f_char == CHAR_HASH;
    }

    bool is_backslash() const
    {
        return f_char == CHAR_BACKSLASH;
    }

    bool is_ascii_punctuation() const
    {
        switch(f_char)
        {
        case CHAR_EXCLAMATION_MARK:
        case CHAR_QUOTE:
        case CHAR_HASH:
        case CHAR_DOLLAR:
        case CHAR_PERCENT:
        case CHAR_AMPERSAND:
        case CHAR_APOSTROPHE:
        case CHAR_OPEN_PARENTHESIS:
        case CHAR_CLOSE_PARENTHESIS:
        case CHAR_ASTERISK:
        case CHAR_PLUS:
        case CHAR_COMMA:
        case CHAR_DASH:
        case CHAR_PERIOD:
        case CHAR_SLASH:
        case CHAR_COLON:
        case CHAR_SEMICOLON:
        case CHAR_LESS:
        case CHAR_EQUAL:
        case CHAR_GREATER:
        case CHAR_QUESTION_MARK:
        case CHAR_AT:
        case CHAR_OPEN_SQUARE_BRACKET:
        case CHAR_BACKSLASH:
        case CHAR_CLOSE_SQUARE_BRACKET:
        case CHAR_CIRCUMFLEX:
        case CHAR_UNDERSCORE:
        case CHAR_ACUTE:
        case CHAR_OPEN_CURLY_BRACKET:
        case CHAR_PIPE:
        case CHAR_CLOSE_CURLY_BRACKET:
        case CHAR_TILDE:
            return true;

        default:
            return false;

        }
    }

    bool operator == (char32_t const c) const
    {
        return f_char == c;
    }

    bool operator == (character const & c) const
    {
        return f_char == c.f_char;
    }

    bool operator != (char32_t const c) const
    {
        return f_char != c;
    }

    bool operator != (character const & c) const
    {
        return f_char != c.f_char;
    }

    std::string to_utf8() const
    {
        return libutf8::to_u8string(f_char);
    }

    char32_t            f_char = CHAR_NULL;
    std::uint32_t       f_line = 0;
    std::uint32_t       f_column = 0;
    std::uint32_t       f_flags = 0;
};



template<typename CharT, typename Traits>
inline std::basic_ostream<CharT, Traits> &
operator << (std::basic_ostream<CharT, Traits> & os, cm::character c)
{
    return os << c.to_utf8();
}


template<typename CharT, typename Traits>
inline std::basic_ostream<CharT, Traits> &
operator << (std::basic_ostream<CharT, Traits> & os, cm::character::string_t s)
{
    for(auto c : s)
    {
        os << c;
    }
    return os;
}



} // namespace cm
// vim: ts=4 sw=4 et
