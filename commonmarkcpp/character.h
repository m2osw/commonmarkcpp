// Copyright (c) 2021-2022  Made to Order Software Corp.  All Rights Reserved
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
#include    "commonmarkcpp/exception.h"


// libutf8 lib
//
#include    <libutf8/iterator.h>
#include    <libutf8/libutf8.h>


// snapdev lib
//
#include    <snapdev/hexadecimal_string.h>
#include    <snapdev/not_reached.h>


// C++ lib
//
#include    <cwctype>
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
constexpr char32_t const    CHAR_ZERO = U'0';                       // 0030
constexpr char32_t const    CHAR_NINE = U'9';                       // 0039
constexpr char32_t const    CHAR_COLON = U':';                      // 003A
constexpr char32_t const    CHAR_SEMICOLON = U';';                  // 003B
constexpr char32_t const    CHAR_OPEN_ANGLE_BRACKET = U'<';         // 003C
constexpr char32_t const    CHAR_EQUAL = U'=';                      // 003D
constexpr char32_t const    CHAR_CLOSE_ANGLE_BRACKET = U'>';        // 003E
constexpr char32_t const    CHAR_QUESTION_MARK = U'?';              // 003F
constexpr char32_t const    CHAR_AT = U'@';                         // 0040
constexpr char32_t const    CHAR_OPEN_SQUARE_BRACKET = U'[';        // 005B
constexpr char32_t const    CHAR_BACKSLASH = U'\\';                 // 005C
constexpr char32_t const    CHAR_CLOSE_SQUARE_BRACKET = U']';       // 005D
constexpr char32_t const    CHAR_CIRCUMFLEX = U'^';                 // 005E
constexpr char32_t const    CHAR_UNDERSCORE = U'_';                 // 005F
constexpr char32_t const    CHAR_GRAVE = U'`';                      // 0060
constexpr char32_t const    CHAR_OPEN_CURLY_BRACKET = U'{';         // 007B
constexpr char32_t const    CHAR_PIPE = U'|';                       // 007C
constexpr char32_t const    CHAR_CLOSE_CURLY_BRACKET = U'}';        // 007D
constexpr char32_t const    CHAR_TILDE = U'~';                      // 007E
constexpr char32_t const    CHAR_REPLACEMENT_CHARACTER = U'\xFFFD'; // FFFD




#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
struct character
{
    typedef std::basic_string<character>  string_t;

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

    bool is_ctrl() const // 0x0000 to 0x001F
    {
        return f_char < CHAR_SPACE;
    }

    bool is_eos() const // EOS (-1)
    {
        return f_char == libutf8::EOS;
    }

    bool is_tab() const // 0x0009
    {
        return f_char == CHAR_TAB;
    }

    bool is_eol() const // 0x000A
    {
        return f_char == CHAR_LINE_FEED;
    }

    bool is_carriage_return() const // 0x000D
    {
        return f_char == CHAR_CARRIAGE_RETURN;
    }

    bool is_space() const // 0x0020
    {
        return f_char == CHAR_SPACE;
    }

    bool is_blank() const // 0x0020 or 0x0009
    {
        return f_char == CHAR_SPACE
            || f_char == CHAR_TAB;
    }

    bool is_exclamation_mark() const // 0x0021
    {
        return f_char == CHAR_EXCLAMATION_MARK;
    }

    bool is_quote() const // 0x0022
    {
        return f_char == CHAR_QUOTE;
    }

    bool is_hash() const // 0x0023
    {
        return f_char == CHAR_HASH;
    }

    bool is_apostrophe() const // 0x0027
    {
        return f_char == CHAR_APOSTROPHE;
    }

    bool is_dash() const // 0x002D
    {
        return f_char == CHAR_DASH;
    }

    bool is_period() const // 0x002E
    {
        return f_char == CHAR_PERIOD;
    }

    bool is_open_parenthesis() const // 0x0028
    {
        return f_char == CHAR_OPEN_PARENTHESIS;
    }

    bool is_close_parenthesis() const // 0x0029
    {
        return f_char == CHAR_CLOSE_PARENTHESIS;
    }

    bool is_asterisk() const // 0x002A
    {
        return f_char == CHAR_ASTERISK;
    }

    bool is_plus() const // 0x002B
    {
        return f_char == CHAR_PLUS;
    }

    bool is_slash() const // 0x002F
    {
        return f_char == CHAR_SLASH;
    }

    bool is_colon() const // 0x003A
    {
        return f_char == CHAR_COLON;
    }

    bool is_semicolon() const // 0x003B
    {
        return f_char == CHAR_SEMICOLON;
    }

    bool is_open_angle_bracket() const // 0x003C
    {
        return f_char == CHAR_OPEN_ANGLE_BRACKET;
    }

    bool is_equal() const // 0x003D
    {
        return f_char == CHAR_EQUAL;
    }

    bool is_close_angle_bracket() const // 0x003E
    {
        return f_char == CHAR_CLOSE_ANGLE_BRACKET;
    }

    bool is_question_mark() const // 0x003F
    {
        return f_char == CHAR_QUESTION_MARK;
    }

    bool is_open_square_bracket() const // 0x005B
    {
        return f_char == CHAR_OPEN_SQUARE_BRACKET;
    }

    bool is_backslash() const // 0x005C
    {
        return f_char == CHAR_BACKSLASH;
    }

    bool is_close_square_bracket() const // 0x005D
    {
        return f_char == CHAR_CLOSE_SQUARE_BRACKET;
    }

    bool is_grave() const // 0x0060
    {
        return f_char == CHAR_GRAVE;
    }

    bool is_thematic_break() const
    {
        return f_char == CHAR_ASTERISK
            || f_char == CHAR_DASH
            || f_char == CHAR_UNDERSCORE;
    }

    bool is_setext() const
    {
        return f_char == CHAR_DASH
            || f_char == CHAR_EQUAL;
    }

    bool is_link_title_open_quote() const
    {
        return f_char == CHAR_QUOTE
            || f_char == CHAR_APOSTROPHE
            || f_char == CHAR_OPEN_PARENTHESIS;
    }

    bool is_unordered_list_bullet() const
    {
        return f_char == CHAR_DASH
            || f_char == CHAR_ASTERISK
            || f_char == CHAR_PLUS;
    }

    bool is_ordered_list_end_marker() const
    {
        return f_char == CHAR_PERIOD
            || f_char == CHAR_CLOSE_PARENTHESIS;
    }

    bool is_fenced_code_block() const
    {
        return f_char == CHAR_GRAVE
            || f_char == CHAR_TILDE;
    }

    bool is_ascii_letter() const
    {
        return (f_char >= 'a' && f_char <= 'z')
            || (f_char >= 'A' && f_char <= 'Z');
    }

    bool is_first_protocol() const
    {
        return is_ascii_letter();
    }

    bool is_protocol() const
    {
        return is_ascii_letter()
            || is_digit()
            || f_char == CHAR_PLUS
            || f_char == CHAR_PERIOD
            || f_char == CHAR_DASH;
    }

    bool is_uri() const
    {
        return (f_char >= 0x21 && f_char <= 0x3D)
            || (f_char >= 0x3F && f_char <= 0x7E);
    }

    bool is_first_tag() const
    {
        return is_ascii_letter();
    }

    bool is_tag() const
    {
        return is_ascii_letter()
            || is_digit()
            || f_char == CHAR_DASH;
    }

    bool is_first_attribute() const
    {
        return is_ascii_letter()
            || f_char == CHAR_UNDERSCORE
            || f_char == CHAR_COLON;
    }

    bool is_attribute() const
    {
        return is_ascii_letter()
            || is_digit()
            || f_char == CHAR_DASH
            || f_char == CHAR_UNDERSCORE
            || f_char == CHAR_COLON
            || f_char == CHAR_PERIOD;
    }

    bool is_attribute_standalone_value() const
    {
        return !(is_blank()
              || f_char == CHAR_QUOTE
              || f_char == CHAR_APOSTROPHE
              || f_char == CHAR_EQUAL
              || f_char == CHAR_OPEN_ANGLE_BRACKET
              || f_char == CHAR_CLOSE_ANGLE_BRACKET
              || f_char == CHAR_GRAVE);
    }

    bool is_left_flanking(character const & previous) const
    {

// A left-flanking delimiter run is a delimiter run that is
// (1) not followed by Unicode whitespace, and either
// (2a) not followed by a Unicode punctuation character, or
// (2b) followed by a Unicode punctuation character and preceded
// by Unicode whitespace or a Unicode punctuation character.
// For purposes of this definition, the beginning and the end of
// the line count as Unicode whitespace.

        if(std::iswspace(f_char))
        {
            return false;
        }

        if(!std::iswpunct(f_char))
        {
            return true;
        }

        if(!previous.is_null()
        && !std::iswspace(previous.f_char)
        && !std::iswpunct(previous.f_char))
        {
            return false;
        }

        return true;
    }

    bool is_right_flanking(character const & previous) const
    {

// A right-flanking delimiter run is a delimiter run that is:
//
//  (1) not preceded by Unicode whitespace, and either
// (2a) not preceded by a Unicode punctuation character, or
// (2b) preceded by a Unicode punctuation character and followed by
//      Unicode whitespace or a Unicode punctuation character.
//
// For purposes of this definition, the beginning and the end of the
// line count as Unicode whitespace.
//
// -- in our case `this` represents the character after the mark
// -- and `previous` represents the character just before the mark

        // if previous is a null character, then we're at the start of a
        // paragraph which doesn't make sense for a right flanking so
        // it's obviously not one of those in that case
        //
        if(previous.is_null()
        || std::iswspace(previous.f_char))
        {
            return false;
        }

        if(std::iswpunct(previous.f_char))
        {
            return std::iswspace(f_char)
                || std::iswpunct(f_char);
        }

        return true;
    }

    bool is_digit() const
    {
        return f_char >= CHAR_ZERO
            && f_char <= CHAR_NINE;
    }

    int digit_number() const
    {
        if(!is_digit())
        {
            throw commonmark_logic_error("digit_number() called with an invalid digit");
        }
        return f_char - CHAR_ZERO;
    }

    bool is_hexdigit() const
    {
        return (f_char >= CHAR_ZERO && f_char <= CHAR_NINE)
            || (f_char >= 'a' && f_char <= 'f')
            || (f_char >= 'A' && f_char <= 'F');
    }

    int hexdigit_number() const
    {
        return snapdev::hexdigit_to_number(f_char);
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
        case CHAR_OPEN_ANGLE_BRACKET:
        case CHAR_EQUAL:
        case CHAR_CLOSE_ANGLE_BRACKET:
        case CHAR_QUESTION_MARK:
        case CHAR_AT:
        case CHAR_OPEN_SQUARE_BRACKET:
        case CHAR_BACKSLASH:
        case CHAR_CLOSE_SQUARE_BRACKET:
        case CHAR_CIRCUMFLEX:
        case CHAR_UNDERSCORE:
        case CHAR_GRAVE:
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

    bool operator < (char32_t const c) const
    {
        return f_char < c;
    }

    bool operator <= (character const & c) const
    {
        return f_char <= c.f_char;
    }

    bool operator > (char32_t const c) const
    {
        return f_char > c;
    }

    bool operator >= (character const & c) const
    {
        return f_char >= c.f_char;
    }

    std::string to_utf8() const
    {
        return libutf8::to_u8string(f_char);
    }

    static std::string to_utf8(character::string_t const & s)
    {
        std::string result;
        for(auto const & c : s)
        {
            result += c.to_utf8();
        }
        return result;
    }

    static character::string_t to_character_string(std::string const & s)
    {
        character::string_t result;

        std::u32string const u32(libutf8::to_u32string(s));
        for(auto const & ch : u32)
        {
            character c{};
            c.f_char = ch;
            result += c;
        }

        return result;
    }

    // to make this struct available to std::string_basic<>() we cannot have
    // a constructor and default values act like such...
    //
    char32_t            f_char /*= CHAR_NULL*/;
    std::uint32_t       f_line /*= 0*/;
    std::uint32_t       f_column /*= 0*/;
};
#pragma GCC diagnostic pop



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
