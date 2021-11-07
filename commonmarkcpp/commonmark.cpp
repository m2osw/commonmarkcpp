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

/** \file
 * \brief Implementation of the CommonMark specification in C++.
 *
 * This file is the implementation of the commonmark class which allows
 * one to convert an input string in HTML.
 *
 * I try to include references to the documentation throughout the code
 * as in:
 *
 *     [REF] <section-name>
 *
 * The reference is found at https://spec.commonmark.org/ and I used
 * version 0.30 first my first version here. This is here:
 * https://spec.commonmark.org/0.30/
 *
 * \sa https://spec.commonmark.org/
 */

// self
//
#include    "commonmarkcpp/commonmark.h"

#include    "commonmarkcpp/commonmark_entities.h"


// snapdev lib
//
#include    <snapdev/hexadecimal_string.h>
#include    <snapdev/string_replace_many.h>


// C++ lib
//
#include    <algorithm>
#include    <cstring>
#include    <iostream>
#include    <sstream>


// last include
//
#include    <snapdev/poison.h>



namespace cm
{



namespace
{



constexpr int mark_and_count(char32_t c, int count)
{
#ifdef _DEBUG
    if(c >= 256)
    {
        throw commonmark_logic_error("character mark is expected to be an ASCII code (under 256)."); // LCOV_EXCL_IGNORE
    }
#endif
    count = (count - 1) % 3 + 1;
    return (count << 8) + static_cast<int>(c);
}


bool parse_link_text(
      character::string_t line
    , character::string_t::const_iterator & et
    , std::string & link_text)
{
    int inner_bracket(1);
    bool start(true);
    bool inside_inline_code(false);
    for(++et; et != line.cend(); ++et)
    {
        switch(et->f_char)
        {
        case CHAR_SPACE:
        case CHAR_TAB:
            if(start)
            {
                // left trim (but not right trim)
                //
                continue;
            }
            break;

        case CHAR_OPEN_SQUARE_BRACKET:
            if(!inside_inline_code)
            {
                ++inner_bracket;
            }
            break;

        case CHAR_CLOSE_SQUARE_BRACKET:
            if(!inside_inline_code)
            {
                --inner_bracket;
                if(inner_bracket == 0)
                {
                    // skip the ']' too before returning
                    //
                    ++et;
                    return !link_text.empty();
                }
            }
            break;

        case CHAR_OPEN_ANGLE_BRACKET:
            // TBD: does the angle bracket need to represent a valid
            //      inline tag or autolink to be refused here?
            //
            return false;

        case CHAR_GRAVE:
            inside_inline_code = !inside_inline_code;
            break;

        case CHAR_BACKSLASH:
            ++et;
            if(et == line.cend()
            || !et->is_ascii_punctuation())
            {
                // this character is not expected to be escaped
                //
                --et;
            }
            else
            {
                // We always need to keep the backslashes for now
                // since we re-parse the inner part as inline content
                //
                link_text += '\\';
            }
            break;

        }
        link_text += et->to_utf8();
        start = false;
    }

    return false;
}


bool parse_link_destination(
      character::string_t line
    , character::string_t::const_iterator & et
    , std::string & link_destination
    , std::string & link_title)
{
    // inline link destination has to have an open parenthesis
    //
    if(et == line.cend()
    || !et->is_open_parenthesis())
    {
        return false;
    }
std::cerr << " >>> destination -- got '('\n";

    // skip the '('
    //
    ++et;
    if(et == line.cend())
    {
        return false;
    }
std::cerr << " >>> destination -- got more than '('\n";

    std::string destination;
    if(et->is_open_angle_bracket())
    {
std::cerr << " >>> destination -- URL within < ... >\n";
        for(++et; ; ++et)
        {
            if(et != line.cend()
            || et->is_open_angle_bracket()
            || et->is_blank()
            || et->is_ctrl())
            {
                return false;
            }
            if(et->is_close_angle_bracket())
            {
                ++et;
                break;
            }
            if(et->is_backslash())
            {
                ++et;
                if(et == line.cend()
                || !et->is_ascii_punctuation())
                {
                    --et;
                }
            }
            destination += et->to_utf8();
        }
    }
    else
    {
        int inner_parenthesis(1);
        bool valid(false);
        for(; et != line.cend(); ++et)
        {
            switch(et->f_char)
            {
            case CHAR_OPEN_PARENTHESIS:
                ++inner_parenthesis;
                break;

            case CHAR_CLOSE_PARENTHESIS:
                --inner_parenthesis;
                if(inner_parenthesis == 0)
                {
                    valid = true;
                }
                break;

            case CHAR_SPACE:    // may be followed by a title
            case CHAR_TAB:
                valid = true;
                break;

            case CHAR_BACKSLASH:
                ++et;
                if(et == line.cend()
                || !et->is_ascii_punctuation())
                {
                    --et;
                }
                break;

            }
            if(valid)
            {
                break;
            }

            destination += et->to_utf8();
        }
std::cerr << " >>> destination -- as is URL valid? " << (valid ? "true" : "false") << "\n";
        if(!valid)
        {
            return false;
        }
    }

    // skip blanks
    //
    for(;; ++et)
    {
        if(et == line.cend())
        {
            return false;
        }
        if(!et->is_blank())
        {
            break;
        }
    }

std::cerr << " >>> destination -- check for title open quote\n";
    std::string title;
    if(et->is_link_title_open_quote())
    {
        character quote(*et);
        if(quote.is_open_parenthesis())
        {
            quote.f_char = CHAR_CLOSE_PARENTHESIS;
        }

        for(++et; et != line.cend() && *et != quote; ++et)
        {
            if(quote.is_close_parenthesis()
            && et->is_open_parenthesis())
            {
                return false;
            }
            if(et->is_backslash())
            {
                ++et;
                if(et == line.cend()
                || !et->is_ascii_punctuation())
                {
                    --et;
                }
            }
            title += et->to_utf8();
        }
        if(et == line.cend())
        {
            return false;
        }
        ++et;
    }

std::cerr << " >>> destination -- closing parenthesis?\n";
    if(et->is_close_parenthesis())
    {
        ++et;
        link_destination = destination;
        link_title = title;
        return true;
    }

std::cerr << " >>> destination -- return false?\n";
    return false;
}


std::string convert_ampersand(
      character::string_t line
    , character::string_t::const_iterator & it
    , bool convert_entities)
{
    // skip the '&'
    //
    ++it;
    if(it == line.cend())
    {
        return "&";
    }

    std::string result;
    auto et(it);
    bool valid(false);
    if(et->is_hash())
    {
        // expected a decimal or hexadecimal number
        //
        char32_t code(0);
        bool has_digit(false);
        ++et;
        if(et != line.end()
        && (et->f_char == 'x' || et->f_char == 'X'))
        {
            for(++et;
                et != line.end() && et->is_hexdigit();
                ++et)
            {
                code *= 16;
                code += et->hexdigit_number();
                if(code >= 0x110000)
                {
                    ++et;
                    break;
                }
                has_digit = true;
            }
        }
        else
        {
            for(;
                et != line.end() && et->is_digit();
                ++et)
            {
                code *= 10;
                code += et->digit_number();
                if(code >= 0x110000)
                {
                    ++et;
                    break;
                }
                has_digit = true;
            }
        }
std::cerr << "--- code = [" << static_cast<int>(code) << "] -- "
<< (has_digit ? "HAS DIGIT" : "no digit")
<< (et->is_semicolon() ? " SEMI-COLON" : " not ';'")
<< "\n";
        if(has_digit
        && et != line.cend()
        && et->is_semicolon())
        {
            ++et;
            valid = true;

            switch(code)
            {
            case CHAR_NULL:
                result += libutf8::to_u8string(CHAR_REPLACEMENT_CHARACTER);
                break;

            case CHAR_AMPERSAND:
                result += "&amp;";
                break;

            case CHAR_QUOTE:
                result += "&quot;";
                break;

            case CHAR_OPEN_ANGLE_BRACKET:
                result += "&lt;";
                break;

            case CHAR_CLOSE_ANGLE_BRACKET:
                result += "&gt;";
                break;

            default:
                if(convert_entities)
                {
                    result += libutf8::to_u8string(code);
                }
                else
                {
                    std::stringstream sd;
                    sd << static_cast<int>(code);
                    std::stringstream sh;
                    sh << 'x' << std::hex << static_cast<int>(code);
                    result += "&#";
                    if(sd.str().length() > sh.str().length())
                    {
                        result += sh.str();
                    }
                    else
                    {
                        result += sd.str();
                    }
                    result += ';';
                }
                break;

            }
        }
    }
    else
    {
        // expect a name
        //
        // note: entity names are not limited to ASCII letters and digits
        //       however the HTML supported entities are and this is why
        //       I limit these names as follow (markdown does not give
        //       you the ability to add new names)
        //
        std::string name;
        for(; et != line.cend(); ++et)
        {
            if(et->f_char == ';')
            {
                ++et;
                valid = !name.empty();
                break;
            }
            if(et->f_char >= '0'
            && et->f_char <= '9')
            {
                if(name.empty())
                {
                    // needs to start with a letter
                    break;
                }
                name += static_cast<char>(et->f_char);
            }
            else if((et->f_char >= 'a' && et->f_char <= 'z')
                 || (et->f_char >= 'A' && et->f_char <= 'Z'))
            {
                name += static_cast<char>(et->f_char);
            }
            else
            {
                break;
            }
        }
        if(valid)
        {
            // we got a "valid" name, search for the corresponding
            // characters instead (although we can as well add the
            // label)
            //
            constexpr struct entity_t const * const end(g_entities + ENTITY_COUNT);
            auto entity(std::lower_bound(
                  g_entities
                , end
                , name
                , [](entity_t const & ent, std::string const & n)
                    {
                        return ent.f_name < n;
                    }));
            if(entity == end
            || name != entity->f_name)
            {
                valid = false;
                name += ';';
            }
            else if(convert_entities
                 && name != "amp"
                 && name != "lt"
                 && name != "gt"
                 && name != "quot")
            {
                // f_codes is already a UTF-8 string
                //
                result += entity->f_codes;
std::cerr << "results name = ["
<< name
<< "] and codes = ";
bool first(true);
for(char const * s(entity->f_codes); *s != '\0'; ++s)
{
if(first)
{
first = false;
}
else
{
std::cerr << ", ";
}
std::cerr << static_cast<int>(*s);
}
std::cerr << "\n";
            }
            else
            {
                // this is a valid entity, keep it as is
                //
                result += '&';
                result += name;
                result += ';';
            }
        }
    }

    if(valid)
    {
        it = et;
    }
    else
    {
        result += "&amp;";
    }

    return result;
}


std::string generate_attribute(
      character::string_t const & line
    , bool convert_entities)
{
    std::string result;
    for(auto it(line.cbegin());
        it != line.cend();
        ++it)
    {
        switch(it->f_char)
        {
        case CHAR_BACKSLASH:
            ++it;
            if(it == line.cend())
            {
                --it;
                result += '\\';
            }
            else
            {
                result += it->to_utf8();
            }
            break;

        case CHAR_OPEN_ANGLE_BRACKET:
            result += "&lt;";
            break;

        case CHAR_CLOSE_ANGLE_BRACKET:
            result += "&gt;";
            break;

        case CHAR_QUOTE:
            result += "&quot;";
            break;

        case CHAR_AMPERSAND:
            result += convert_ampersand(
                          line
                        , it
                        , convert_entities);
            --it;
            break;

        default:
            result += it->to_utf8();
            break;

        }
    }

    return result;
}


std::string convert_uri(character::string_t const & uri)
{
    std::string result;

    for(character::string_t::const_iterator c(uri.cbegin());
        c != uri.cend();
        ++c)
    {
        switch(c->f_char)
        {
        case ' ':       // 20
        case '"':       // 22
        case '#':       // 23
        case '$':       // 24
        case '&':       // 26
        case '\'':      // 27
        case '(':       // 28
        case ')':       // 29
        case '+':       // 2B
        case ',':       // 2C
        //case '/':       // 2F -- this should be done in the query string
        //case ':':       // 3A -- don't convert the protocol ':', others are fine too
        case ';':       // 3B
        case '<':       // 3C
        //case '=':       // 3D -- validly used in query srings
        case '>':       // 3E
        //case '?':       // 3F -- we assume this is the query string introducer
        case '@':       // 40
        case '[':       // 5B
        case '\\':      // 5C
        case ']':       // 5D
        case '^':       // 5E
        case '{':       // 7B
        case '|':       // 7C
        case '}':       // 7D
        case '~':       // 7E
        case '`':       // 60
            result += "%";
            result += snap::int_to_hex(
                              static_cast<std::uint8_t>(c->f_char)
                            , true
                            , 2);
            break;

        case '%':       // 25
            // if already followed by 2 hexdigits, do
            // nothing, otherwise replace the % with %25
            //
            ++c;
            if(c != uri.end()
            && snap::is_hexdigit(c->f_char))
            {
                ++c;
                if(c != uri.end()
                && snap::is_hexdigit(c->f_char))
                {
                    result += '%';
                    result += static_cast<char>(c[-1].f_char);
                    result += static_cast<char>(c->f_char);
                    break;
                }
                --c;
            }
            --c;
            result += "%25";
            break;

        default:
            {
std::cerr << "convert character: " << static_cast<int>(c->f_char) << "\n";
                std::string u(c->to_utf8());
                for(auto nc : u)
                {
                    if(static_cast<unsigned  char>(nc) >= 0x80)
                    {
                        // upper UTF-8 codes are all encoded
                        //
                        result += '%';
                        result += snap::int_to_hex(nc, true, 2);
                    }
                    else
                    {
                        result += nc;
                    }
                }
            }
            break;

        }
    }

    return result;
}


enum class state_t
{
    STATE_NAME_OR_END,                      // attribute name or ">" or "/"
    STATE_NAME_OR_EQUAL_OR_END,             // attribute name or "=" or ">" or "/"
    STATE_EQUAL_OR_END,                     // "=" or ">" or "/"
    STATE_ATTRIBUTE_VALUE,                  // "..." or '...' or <value>
    STATE_ATTRIBUTE_VALUE_SINGLE_QUOTED,    // the ... in "..."
    STATE_ATTRIBUTE_VALUE_DOUBLE_QUOTED,    // the ... in '...'
    STATE_END,                              // ">"
};

bool verify_tag_attributes(
      character::string_t line
    , character::string_t::const_iterator & et)
{
    state_t state(state_t::STATE_NAME_OR_END);
    for(;;)
    {
std::cerr << " ---- tag innards state: " << static_cast<int>(state) << "...\n";

        if(et == line.cend())
        {
            // TODO: this is currently always true here; it may be that
            //       there are other cases where it could be otherwise?
            //
std::cerr << " ---- EOL found, tag needs to be complete...\n";
            return false;
        }

        if(state == state_t::STATE_END)
        {
            if(et->is_close_angle_bracket())
            {
                // tag is considered valid
                //
std::cerr << " ---- tag innards got '>'\n";
                ++et;
                return true;
            }

            // TODO: restore input state (especially if we read new lines...)
            //
std::cerr << " ---- tag innards expect missing '>'\n";
            return false;
        }

        if(et->is_blank())
        {
std::cerr << " ---- tag innards skip blank\n";
            ++et;
            continue;
        }

        if(state == state_t::STATE_NAME_OR_END
        || state == state_t::STATE_NAME_OR_EQUAL_OR_END)
        {
            if(et->is_first_attribute())
            {
                for(++et; et != line.cend(); ++et)
                {
                    if(!et->is_attribute())
                    {
                        break;
                    }
                }
std::cerr << " ---- tag innards found attribute name\n";
                state = state_t::STATE_NAME_OR_EQUAL_OR_END;
                continue;
            }
            else if(et->is_slash())
            {
std::cerr << " ---- tag innards found slash\n";
                ++et;
                state = state_t::STATE_END;
                continue;
            }
            else if(et->is_close_angle_bracket())
            {
std::cerr << " ---- tag innards found '>', valid!\n";
                ++et;
                return true;
            }
        }

        if(state == state_t::STATE_NAME_OR_EQUAL_OR_END
        && et->is_equal())
        {
std::cerr << " ---- tag innards got '=' before value\n";
            state = state_t::STATE_ATTRIBUTE_VALUE;
            ++et;
            continue;
        }

        if(state == state_t::STATE_ATTRIBUTE_VALUE)
        {
            if(et->is_quote())
            {
                state = state_t::STATE_ATTRIBUTE_VALUE_DOUBLE_QUOTED;
                ++et;
                continue;
            }
            else if(et->is_apostrophe())
            {
                state = state_t::STATE_ATTRIBUTE_VALUE_SINGLE_QUOTED;
                ++et;
                continue;
            }
            else
            {
                for(;
                    et != line.cend()
                        && et->is_attribute_standalone_value();
                    ++et);

                if(et != line.cend()
                && !et->is_blank()
                && !et->is_slash()
                && !et->is_close_angle_bracket())
                {
std::cerr << " ---- tag innards invalid attribute value\n";
                    // TODO: fix input if multi-line
                    //
                    return false;
                }
            }

            state = state_t::STATE_NAME_OR_END;
            continue;
        }

        if(state == state_t::STATE_ATTRIBUTE_VALUE_DOUBLE_QUOTED)
        {
            if(et->is_quote())
            {
                state = state_t::STATE_NAME_OR_END;
            }

            ++et;
            continue;
        }

        if(state == state_t::STATE_ATTRIBUTE_VALUE_SINGLE_QUOTED)
        {
            if(et->is_apostrophe())
            {
                state = state_t::STATE_NAME_OR_END;
            }

            ++et;
            continue;
        }

        // state / current character mismatch, we've got an error
        //
        // TODO: fix input if multi-line
        //
        return false;
    }
}



}
// no name namespace




commonmark::input_status_t::input_status_t(
          libutf8::utf8_iterator & iterator
        , std::uint32_t line
        , std::uint32_t column)
    : f_iterator(iterator)
    , f_line(line)
    , f_column(column)
{
}



/** \brief Setup the UTF-8 iterator to go through the input string data.
 *
 * The constructor initializes the UTF-8 iterator which we use to read
 * the characters as char32_t values. We then reconvert them to UTF-8
 * on output. This process allows us to at least fix any invalid
 * UTF-8 characters (later we also want to look at making sure it
 * is also canonicalized).
 */
commonmark::commonmark()
    : f_iterator(f_input)
{
}


/** \brief Change the set of features.
 *
 * The commonmark parsing and output can be tweaked in many different ways.
 * For you to control these tweaks, the library offers a features object
 * which has many flags that can be turned on or off to determine what
 * happens.
 *
 * You must call this function before the process() function.
 */
void commonmark::set_features(features const & features)
{
    f_features = features;
}


/** \brief Retrieve the current features of this commonmark object.
 *
 * This function returns the set of features as defined in the features
 * object passed to the set_features() function.
 *
 * The default is a features object without any modifications.
 *
 * \return The commonmark internal set of features.
 */
features const & commonmark::get_features() const
{
    return f_features;
}


/** \brief Add a link to the list of links of the commonmark object.
 *
 * Each link found in the document are added to the commonmark
 * object using this function.
 *
 * \param[in] name  The name of the link (used as the label or alt=...).
 * \param[in] destination  The destination URI.
 * \param[in] title  The title (description/alternative) of the link.
 * \param[in] reference  The link was defined as a reference.
 */
void commonmark::add_link(
      std::string const & name
    , std::string const & destination
    , std::string const & title
    , bool reference)
{
    link::pointer_t l;
    auto const it(f_links.find(name));
    if(it == f_links.end())
    {
        l = std::make_shared<link>(name);
        f_links[name] = l;
    }
    else
    {
        l = it->second;
    }

    uri u;
    if(reference)
    {
        u.mark_as_reference();
    }
    u.destination(destination);
    u.title(title);

    l->add_uri(u);
}


/** \brief Search for a link reference.
 *
 * This function returns the link reference \p name or a null pointer
 * if the reference is not found.
 *
 * There is nearly no constraints on the name of a link reference.
 * Note that in our case, the link name is not processed. It is as
 * it appears in the source file (except that it usually get trimmed).
 *
 * \return A link point or nullptr.
 */
link::pointer_t commonmark::find_link_reference(std::string const & name)
{
    auto it(f_links.find(name));
    if(it == f_links.end())
    {
        return link::pointer_t();
    }
    return it->second;
}


/** \brief Process the specified input data.
 *
 * This function processes the specified \p input data and returns the
 * resulting HTML.
 *
 * \param[in] input  The input markdown to convert to HTML.
 *
 * \return The resulting HTML in a UTF-8 string.
 */
std::string commonmark::process(std::string const & input)
{
    f_input = input;
    f_output.clear();

    parse();
std::cerr << "- * -------------------------------------------- TREE:\n";
std::cerr << f_document->tree();
std::cerr << "- * -------------------------------------------- TREE END ---\n";
    generate(f_document);

    return f_output;
}


/** \brief Get the next character.
 *
 * This function returns the next character and returns it.
 *
 * The character is assigned a line and column number so we can easily
 * reference the exact location were we find an invalid token. It also
 * takes care of computing the next line and colun number. It also
 * transforms the `\\r` and `\\r\\n` in just one `\\n` so that way
 * the reset of the software only has to deal with the linefeed
 * character.
 *
 * The column number is adjusted to the next column which is a multiple
 * of 4 when a tab is read.
 *
 * The function returns EOS if there are no more characters available in
 * the input string.
 *
 * The function transforms the NULL character in the \em replacement
 * \em character (a.k.a. 0xFFFD). The NULL character is viewed as a
 * potential security issue and it can be an annoyance in C/C++ strings
 * so it's a good thing all around for us.
 *
 * \return The character we just read.
 */
character commonmark::getc()
{
    //if(f_unget_pos > 0)
    //{
    //    --f_unget_pos;
    //    return f_unget[f_unget_pos];
    //}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    character c =
    {
        .f_char = *f_iterator,
        .f_line = f_line,
        .f_column = f_column,
    };
#pragma GCC diagnostic pop

    ++f_iterator;

    // [REF] 2.2 Tabs
    //
    if(c.is_tab())
    {
        // we count columns starting at 1, so the following makes it a
        // multiple of 4 with that constraint (i.e. one tab at the start
        // of a line gets you to column 5, not 4)
        //
        f_column = ((f_column + 3) & -4) + 1;
    }
    else
    {
        ++f_column;
    }

    // [REF] 2.3 Insecure characters
    //
    c.fix_null();

    // [REF] 2.1 Characters and lines (line ending)
    //
    if(c.is_carriage_return())
    {
        if(*f_iterator == CHAR_LINE_FEED)
        {
            ++f_iterator;
        }

        // always replace the '\r' with '\n' so the rest of the parser
        // doesn't have to deal with both characters
        //
        c.f_char = CHAR_LINE_FEED;
    }

    if(c.is_eol())
    {
        // move to the next line for next getc()
        //
        ++f_line;
        f_column = 1;
    }

    return c;   // returns libutf8::EOS at the end of the input
}


/** \brief Get one line of input.
 *
 * The markdown format is very much based on a per line basis, although
 * some lines get merged (i.e. in a multiline paragraph, for example).
 *
 * The command returns a string composed of the characters of the line
 * except the end of line character (the linefeed). Some other functions
 * re-add that character later when necessary (i.e. the blocks of code
 * are required to keep all the line feeds intact since they are important
 * even in the final HTML output).
 *
 * If the function finds the end of the string, then it sets a flag to
 * true. However, this is not the end of the file yet unless the returned
 * string is also empty.
 *
 * \return The line in a string.
 */
void commonmark::get_line()
{
    f_last_line.clear();

    for(;;)
    {
        character c(getc());
        if(c.is_eos())
        {
            f_eos = true;
            break;
        }
        if(c.is_eol())
        {
            break;
        }
        f_last_line += c;
    }
}


commonmark::input_status_t commonmark::get_current_status()
{
    input_status_t const status(f_iterator, f_line, f_column);
    return status;
}


void commonmark::restore_status(input_status_t const & status)
{
    f_iterator = status.f_iterator;
    f_line = status.f_line;
    f_column = status.f_column;
}


///** \brief Check whether the line is considered empty.
// *
// * Empty lines have special effects on some paragraphs and blocks.
// * For example, they close a standard paragraph.
// *
// * An empty line is a string composed exclusively of spaces and tabs
// * or no characters at all.
// *
// * \return true if the line is considered empty.
// */
//bool commonmark::is_empty(character::string_t const & str)
//{
//    for(auto c : str)
//    {
//        if(!c.is_space()
//        || !c.is_tab())
//        {
//            return false;
//        }
//    }
//
//    return true;
//}


/** \brief Parse the input data, line by line.
 *
 * The parser first determines the type of each line (in term of which block
 * it pertains to). Then it adds that to the document tree.
 *
 * The types are:
 *
 * \li `'#'` -- header with one or more `'#'`.
 *
 * \li `'===='` -- setext header (also `'---'`), similar to the `'#'` but
 * transforms a preceeding paragraph in a header.
 *
 * \li `'>'` -- blockquote, there can be multiple `'>'` in a row, and the first
 * space is ignored to determine further block indentation; note that we can
 * have a blockquote within a list item and a list item within a blockquote
 * which gives us multiple levels, however, going from `'>>>'` to `'>'` on
 * the next line does not change the level, it remains at 3.
 *
 * \li `'- ...'` -- list item (also `'+'`, `'*'`, a number  and `')'` or
 * `'.'`). this is peculiar since it can start an indentation area
 * (i.e. the following `'    '` is a paragraph part of the list item and
 * not a block of code)
 *
 * \li `'    '` -- one block of indentation, by itself it's a block of code,
 * after a list item, it's a continuation paragraph from the
 * list item; one more indentation level and it becomes a
 * block of code again; also we can have sub-list items which
 * makes the indentation increase by another 4 spaces or
 * equivalent
 *
 * \li Anything else (other than an empty line) represents a regular paragraph.
 *
 * \return true if we found something, false if the line is considered empty.
 */
void commonmark::parse()
{
    // reset iterator in case the parser is used multiple times
    //
    f_iterator.rewind();
    f_eos = false;

    // create a new document
    //
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    f_document = std::make_shared<block>(character{
            .f_char = BLOCK_TYPE_DOCUMENT,
            .f_line = 1,
            .f_column = 1,
        });
#pragma GCC diagnostic pop

    f_document->followed_by_an_empty_line(true);
    f_last_block = f_document;

    for(;;)
    {
        // create the line block before we get the line so we get
        // the correct line & column numbers
        //
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        f_top_working_block = std::make_shared<block>(character{
                .f_char = BLOCK_TYPE_LINE,
                .f_line = f_line,
                .f_column = 1,
            });
#pragma GCC diagnostic pop
        f_working_block = f_top_working_block;

        get_line();

std::cerr << "---------------- process containers...\n";
        auto it(parse_containers());

std::cerr << "any containers (other than line)? "
<< " -- it position: "
<< it - f_last_line.cbegin()
<< "\n";
        if(f_working_block->is_line()
        && it == f_last_line.cend())
        {
            // we found an empty line
            //
std::cerr << "found empty line (Last? " << (f_eos ? "EOS" : "not eos?!") << ")\n";
            if(f_eos)
            {
                // end of data, return
                //
                return;
            }

            // handle an empty line (important for code blocks)
            //
            process_empty_line();
            continue;
        }
std::cerr << " --- not block? "
<< " -- it position: "
<< it - f_last_line.cbegin()
<< "\n";

        // TODO: the f_code_block flag should be set by the parse_contrainers
        //       but we need to fix the algorithm in there
        //
        if(f_code_block)
        {
            if(process_indented_code_block(it))
            {
std::cerr << " ---- got indented code block\n";
                append_line();
                continue;
            }
        }
std::cerr << " --- not indented block? "
<< " -- it position: "
<< it - f_last_line.cbegin()
<< "\n";

        // TBD: here we add everything to the previous paragraph
        //      we may have to move a few of the block detection
        //      still below before this line so things work as
        //      expected
        //
        if(f_last_block->parent() != nullptr
        && !f_last_block->parent()->is_list()
        && f_last_block->is_paragraph()
        && !f_last_block->followed_by_an_empty_line()
        && it->f_column >= (f_list_subblock == 0 ? 5 : f_list_subblock + 4))
        {
            process_paragraph(it);
std::cerr << " ---- early paragraph with list sub-block = "
<< it->f_column
<< " >= "
<< f_list_subblock
<< " + 4\n";
            append_line();
            continue;
        }

        if(process_fenced_code_block(it))
        {
std::cerr << " ---- got fenced code block\n";
            append_line();
            continue;
        }
std::cerr << " --- not fenced block? "
<< " -- it position: "
<< it - f_last_line.cbegin()
<< " -- " << reinterpret_cast<void const *>(&*it)
<< "\n";
void const * it_ptr(reinterpret_cast<void const *>(&*it));
if(it_ptr < reinterpret_cast<void const *>(&*f_last_line.cbegin())
|| it_ptr >= reinterpret_cast<void const *>(&*f_last_line.cend()))
{
    throw commonmark_logic_error("invalid it from fenced code?");
}

        if(process_html_blocks(it))
        {
std::cerr << " ---- got HTML block\n";
            append_line();
            continue;
        }
std::cerr << " --- not HTML block? "
<< " -- it position: "
<< it - f_last_line.cbegin()
<< " -- " << reinterpret_cast<void const *>(&*it)
<< "\n";
void const * it_ptr2 = reinterpret_cast<void const *>(&*it);
if(it_ptr2 < reinterpret_cast<void const *>(&*f_last_line.cbegin())
|| it_ptr2 >= reinterpret_cast<void const *>(&*f_last_line.cend()))
{
    throw commonmark_logic_error("invalid it after HTML");
}

        // TBD: can we have a header inside a list?
        //      the specs asks the question but no direct answer...
        //      at this point we think not
        //
        // header introducer
        //
        if(process_header(it))
        {
std::cerr << " ---- header\n";
            append_line();
            continue;
        }
std::cerr << " --- not header? "
<< " -- it position: "
<< it - f_last_line.cbegin()
<< "\n";

        if(process_thematic_break_or_setext_heading(it))
        {
std::cerr << " ---- break or setext\n";
            append_line();
            continue;
        }
std::cerr << " --- not thematic break? "
<< " -- it position: "
<< it - f_last_line.cbegin()
<< "\n";

        if(process_reference_definition(it))
        {
std::cerr << " ---- link reference\n";
            continue;
        }
std::cerr << " --- not reference? "
<< " -- it position: "
<< it - f_last_line.cbegin()
<< " == "
<< (it == f_last_line.cend() ? " END REACHED!" : " it is not at the end, we're good, right?")
<< "\n";

        process_paragraph(it);
std::cerr << " ---- paragraph\n";
        append_line();
    }
}


character::string_t::const_iterator commonmark::parse_containers()
{
    // the meaning of the current position depends on the previous line
    // found in f_last_block, the current line found in f_working_block
    // and the current iterator
    //
    // depending on the meaning, we may have to directly process the
    // line as a code block so one could view this as a code block
    // processing... but many of the tests are not linked to the code
    // block which is why it is defined in a separate function
    //
    auto it(f_last_line.cbegin());

std::cerr << "- * ---------------------------- DOCUMENT TREE BEFORE CHECKING CONTAINERS:\n";
std::cerr << f_document->tree();
std::cerr << "- * ---------------------------- DOCUMENT TREE BEFORE CHECKING CONTAINERS END\n";
    bool const previous_line_is_indented_code_block(f_last_block->is_indented_code_block());
    bool const previous_line_is_header(f_last_block->is_header());
    bool const has_empty_line(
               f_last_block->parent() != nullptr
            && f_last_block->parent()->is_list()
                    ? f_last_block->parent()->followed_by_an_empty_line()
                    : f_last_block->followed_by_an_empty_line());
    std::uint32_t const list_indent(
               f_last_block->parent() != nullptr
            && f_last_block->parent()->is_list()
                    ? (has_empty_line
                        ? f_last_block->parent()->first_child()->end_column()
                        : std::numeric_limits<std::uint32_t>::max())
                    : (has_empty_line
                        ? 1U
                        : std::numeric_limits<std::uint32_t>::max()));

std::cerr
<< "+++ checking for containers (list_indent="
<< list_indent
<< ", f_last_line=\""
<< f_last_line
<< "\", list?="
<< (f_last_block->parent() != nullptr
        ? (f_last_block->parent()->is_list() ? "LIST" : "(not list)")
        : "(no parent)")
<< ", f_working_block->end_column()="
<< f_working_block->end_column()
<< ")\n";

    f_code_block = false;
    f_list_subblock = 0;
    while(it != f_last_line.cend())
    {
std::cerr << "   column: " << it->f_column << " indent " << list_indent
<< " -- it position: "
<< it - f_last_line.cbegin()
<< "\n";
        bool const skipped_blank(parse_blank(it));

        // compute what blanks mean here
        //
        // E - empty line
        // L - list
        // B - blockquote
        // C - code
        // S - spaces (blanks)
        // P - paragraph
        // N - sub-list 
        // K - continuation from previous block
        // . - start/end are one and the same
        // ? - anything accepted
        //
        //  /---- Previous Line START
        //  |
        //  |  /---- Previous Line END
        //  |  |
        //  |  |  /---- Current Line
        //  |  |  |
        //  |  |  |  /---- Number of blanks (# to #+3, S=start of previous list content)
        //  |  |  |  |
        //  |  |  |  |  /---- Resulting Effect
        //  |  |  |  |  |
        //  v  v  v  v  v
        //
        //  .  E  L  0  L
        //  .  E  L  4  N
        //  .  E  L  8  C
        //  .  L  L  0  L
        //  .  L  L  S  N
        //  .  L  L S+4 N
        //  .  B  S  0  K
        //  .  B  B  0  K
        //  .  L  B  S  B
        //  B  L  B  0  K
        //  B  L  B S+4 B
        //
        f_current_gap = it->f_column - f_working_block->end_column();
        if((has_empty_line
            || previous_line_is_indented_code_block
            || previous_line_is_header)
        && f_current_gap >= 4
        && it->f_column >= list_indent + 4)
        {
std::cerr << "+++ got a gap of "
<< f_current_gap
<< " >= 4 blanks! (working column: "
<< f_working_block->end_column()
<< ", it column "
<< it->f_column
<< ", list indent + 4: "
<< list_indent + 4
<< " -- it position: "
<< it - f_last_line.cbegin()
<< ")\n";

            f_code_block = true;

            if(f_last_block->parent() != nullptr
            && f_last_block->parent()->is_list()
            && has_empty_line)
            {
                f_list_subblock = list_indent + 4;
            }
            else if(f_working_block->is_list())
            {
                // TBD: why the -1 here?
                //
                f_list_subblock = it->f_column - f_working_block->end_column();
            }
std::cerr << " ---- gap return with f_list_subblock of "
<< f_list_subblock
<< "\n";

            break;
        }
else
{
std::cerr << "+++ gap so far "
<< f_current_gap
<< " blanks, working column: "
<< f_working_block->end_column()
<< ", it column "
<< it->f_column
<< ", list indent: "
<< list_indent
<< " -- it position: "
<< it - f_last_line.cbegin()
<< "\n";
}

        if(skipped_blank)
        {
            continue;
        }

        if(parse_blockquote(it))
        {
std::cerr << "+++ found a blockquote...\n";
            continue;
        }

        // after a list item we can find a blockquote with blanks in
        // between, but we can't have two list items one after the other
        //
std::cerr << "working block is a list? "
<< (f_working_block->is_list() ? "YES" : "no")
<< " and list_indent is "
<< list_indent
<< " -- it position: "
<< it - f_last_line.cbegin()
<< "\n";
        if(!f_working_block->is_list()
        && parse_list(it))
        {
std::cerr << "+++ found a list...\n";
            continue;
        }
std::cerr << "but after calling parse_list() we have list_indent = "
<< list_indent
<< " -- it position: "
<< it - f_last_line.cbegin()
<< "\n";

        if(f_last_block->parent() != nullptr
        && f_last_block->parent()->is_list()
        && has_empty_line)
        {
std::cerr << "then compare "
<< it->f_column
<< " vs list_indent + 4 = "
<< (list_indent + 4)
<< " -- it position: "
<< it - f_last_line.cbegin()
<< "\n";
            if(it->f_column >= list_indent + 4)
            {
std::cerr << "+++ this looks like a list item followed by this block code: "
<< it->f_column
<< " >= "
<< (list_indent + 4)
<< " blanks! (working column: "
<< f_working_block->end_column()
<< " -- it position: "
<< it - f_last_line.cbegin()
<< ")\n";

                f_code_block = true;
                f_list_subblock = list_indent + 4;
            }
            else if(it->f_column >= list_indent)
            {
std::cerr << "+++ this is a list item followed by this additional paragraph: "
<< it->f_column
<< " >= "
<< list_indent
<< " blanks! (working column: "
<< f_working_block->end_column()
<< " -- it position: "
<< it - f_last_line.cbegin()
<< ")\n";

                f_list_subblock = list_indent;
            }
        }

std::cerr << "+++ not a list or blockquote?"
<< " -- it position: "
<< it - f_last_line.cbegin()
<< "\n";
        break;
    }

    return it;
}


bool commonmark::parse_blank(character::string_t::const_iterator & it)
{
    if(it->is_blank())
    {
        ++it;
        return true;
    }

    return false;
}


bool commonmark::parse_blockquote(character::string_t::const_iterator & it)
{
    if(!it->is_close_angle_bracket())
    {
        return false;
    }

    if(f_working_block->is_blockquote())
    {
        f_working_block->number(f_working_block->number() + 1);
        f_working_block->end_column(it->f_column);
    }
    else
    {
        block::pointer_t b(std::make_shared<block>(*it));
        b->number(1);
        if((it + 1)->is_blank())
        {
            b->end_column(b->column() + 2);
            ++it;
        }
        else
        {
            b->end_column(b->column() + 1);
        }
        f_working_block->link_child(b);
        f_working_block = b;
    }

    ++it;
    return true;
}


bool commonmark::parse_list(character::string_t::const_iterator & it)
{
    // make sure `it` does not get modified if we end up returning false
    //
    auto et(it);

std::cerr << " >> parse for list [" << static_cast<int>(it->f_char) << "]...\n";
    int number(-1);
    if(et->is_digit())
    {
std::cerr << " >> numbered list...\n";
        // a list starts with a number if it is followed by a '.' or ')'
        // but first read the whole number
        //
        number = et->digit_number();
        for(++et; et->is_digit(); ++et)
        {
            number *= 10;
            number += et->digit_number();
            if(number >= 1'000'000'000)
            {
                // this number is limited to 9 digits; more and it's not
                // considered to be a a list number
                //
                return false;
            }
        }
        if(!et->is_ordered_list_end_marker())
        {
            return false;
        }
    }
    else
    {
        if(!et->is_unordered_list_bullet()
        || is_thematic_break(et))
        {
std::cerr << " >> not a list mark...\n";
            return false;
        }
        ++et;
        if(!et->is_blank())
        {
std::cerr << " >> not a list item (blank missing after mark)...\n";
            return false;
        }
        --et;
    }

    character type(*it);            // get position of 'it'
    type.f_char = et->f_char;       // but character (a.k.a. type) of 'et'
std::cerr << " >> list type [" << static_cast<int>(et->f_char) << "]...\n";

    ++et;
    if(!et->is_blank())
    {
std::cerr << " >> list blank missing [" << static_cast<int>(et->f_char) << "]...\n";
        return false;
    }

    block::pointer_t b(std::make_shared<block>(type));
    if(number >= 0)
    {
        b->number(number);
    }
    b->end_column(et->f_column);    // end column is defined in 'et'

    f_working_block->link_child(b);
    f_working_block = b;

    it = et;
    ++it;
    return true;
}


/** \brief Check whether the character from it represent a thematic break.
 *
 * While working on the parsing of lists & blockquotes, we need to make sure
 * that the start of a list item is really a list and not a thematic break.
 * This function is used for that purpose.
 *
 * \warning
 * The function returns true whether the inupt represents a thematic break
 * or an setext.
 *
 * \param[in] it  The iterator to start from.
 *
 * \return true if the input looks like a thematic break.
 */
bool commonmark::is_thematic_break(character::string_t::const_iterator it)
{
    character const c(*it);
    if(!c.is_thematic_break())
    {
        return false;
    }

    int count(1);
    for(++it; it != f_last_line.cend(); ++it)
    {
        if(!it->is_blank())
        {
            if(*it != c)
            {
                return false;
            }
            ++count;
        }
    }

    return count >= 3;
}


/** \brief Process an empty line.
 *
 * By default, empty lines are just ignored.
 *
 * However, in a block of code, empty lines are expected and these need to
 * be tracked. The function adds an empty line to the list of blocks if the
 * current block is a block of code.
 */
void commonmark::process_empty_line()
{
    if(f_last_block->parent() != nullptr
    && f_last_block->parent()->is_list())
    {
        f_last_block->parent()->followed_by_an_empty_line(true);
    }
    else
    {
        f_last_block->followed_by_an_empty_line(true);
    }

    if(f_last_block->is_indented_code_block())
    {
        // with blocks of code, we actually keep empty lines (we'll trim
        // the last new lines later as required)
        //
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        character c{
            .f_char = '\n',
            .f_line = f_line,   // TBD: probably f_line - 1?
            .f_column = 1,
        };
#pragma GCC diagnostic pop
        f_last_block->append(c);    // the empty line is represented by a '\n' in a `block` object
    }
std::cerr << "- * ---------------------------- DOCUMENT TREE AFTER EMPTY LINE:\n";
std::cerr << f_document->tree();
std::cerr << "- * ---------------------------- DOCUMENT TREE AFTER EMPTY LINE END\n";
std::cerr << "- * ---------------------------- LAST BLOCK TREE AFTER EMPTY LINE:\n";
std::cerr << f_last_block->tree();
std::cerr << "- * ---------------------------- LAST BLOCK TREE AFTER EMPTY LINE END ---\n";
}


bool commonmark::process_paragraph(character::string_t::const_iterator & it)
{
    void const * it_ptr(reinterpret_cast<void const *>(&*it));
    if(it_ptr < reinterpret_cast<void const *>(&*f_last_line.cbegin())
    || it_ptr >= reinterpret_cast<void const *>(&*f_last_line.cend()))
    {
        throw commonmark_logic_error("paragraph called with a mismatached it parameter");
    }

std::cerr << "    setup character\n";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    character const paragraph{
        .f_char = BLOCK_TYPE_PARAGRAPH,
        .f_line = it->f_line,
        .f_column = it->f_column,
    };
#pragma GCC diagnostic pop

std::cerr << "    create paragraph\n";
    block::pointer_t b(std::make_shared<block>(paragraph));
std::cerr << "    add content -- "
<< reinterpret_cast<void const *>(&*f_last_line.cbegin())
<< " -- "
<< reinterpret_cast<void const *>(&*it)
<< " -- "
<< reinterpret_cast<void const *>(&*f_last_line.cend())
<< "\n";
    b->append(character::string_t(it, f_last_line.cend()));
std::cerr << "    link child\n";
    f_working_block->link_child(b);
std::cerr << "    save working child\n";
    f_working_block = b;
std::cerr << "    end paragraph handling\n";

    return true;
}


void commonmark::append_line()
{
    block::pointer_t b(f_top_working_block->first_child());
    if(b == nullptr)
    {
        throw commonmark_logic_error("append_line() called with an empty line.");
    }

    if(f_list_subblock > 0
    || (f_working_block->parent() != nullptr
           && f_working_block->parent()->is_blockquote()
           && f_working_block->is_indented_code_block()))
    {
std::cerr << "+++ add child to list?\n";
        b->unlink();
        if(f_last_block->parent() != nullptr
        && f_last_block->parent()->is_list())
        {
            f_last_block->parent()->link_child(b);
        }
        else
        {
            f_last_block->link_child(b);
        }
        f_last_block = f_working_block;
    }
    else if((f_last_block->is_indented_code_block()
                && b->is_indented_code_block())
         || (f_last_block->is_paragraph()
                && !f_last_block->followed_by_an_empty_line()
                && b->is_paragraph()))
    {
std::cerr << "+++ append or what? ["
<< b->content()
<< "]\n";

        // in case of a paragraph, we also may need the newline because that
        // acts as a space if nothing else is at that point
        //
        // the code block always gets a line feed (even at the end) so it's
        // handled differently
        //
        if(b->is_paragraph())
        {
            character c(*f_last_line.crbegin());
            c.f_char = CHAR_LINE_FEED;
            ++c.f_column;
            f_last_block->append(c);
        }

        f_last_block->append(b->content());
    }
    else if(f_working_block->parent() != nullptr
         && f_working_block->parent()->is_list()
         && f_last_block->parent() != nullptr
         && f_last_block->parent()->is_list()
         && f_working_block->parent()->column() >= f_last_block->parent()->end_column()
         && f_working_block->parent()->column() <= f_last_block->parent()->end_column() + 3)
    {
std::cerr << "+++ link sub-list? " << (reinterpret_cast<void *>(b.get())) << "\n";
        b = f_working_block->parent();
        b->unlink();
        f_last_block->parent()->link_child(b);
        f_last_block = f_working_block;
    }
    else
    {
std::cerr << "+++ working parent is list? "
    << (f_working_block->parent() ? (f_working_block->parent()->is_list() ? "LIST" : "no") : "no parent")
<< "\n    last parent is list? "
    << (f_last_block->parent() ? (f_last_block->parent()->is_list() ? "LIST" : "no") : "no parent")
<< "\n    columns: "
    << (f_working_block->parent() ? std::to_string(f_working_block->parent()->column()) : "no parent")
<< " & "
    << (f_last_block->parent() ? std::to_string(f_last_block->parent()->end_column()) : "no parent")
<< "\n";

std::cerr << "+++ link new? " << (reinterpret_cast<void *>(b.get())) << "\n";
        b->unlink();
        f_document->link_child(b);
        f_last_block = f_working_block;
    }
}


bool commonmark::process_thematic_break_or_setext_heading(character::string_t::const_iterator & it)
{
std::cerr << "process_thematic_break_or_setext_heading() called!\n";
    // [REF] 4.1 Thematic breaks
    //

    if(f_list_subblock >= 4)
    {
        return false;
    }

    // get the first character, we can't have a mix, so we need to make sure
    // the other ones we find on the line are the same
    //
    character c(*it);
    if(!c.is_thematic_break()
    && !c.is_equal())
    {
        return false;
    }
std::cerr << " >>> sub-block? " << f_list_subblock << "\n";

    // ignore ending blanks (otherwise internal_spaces used below would
    // improperly be set to true)
    //
    auto et(f_last_line.cend());
    while(et != it)
    {
        --et;
        if(!et->is_blank())
        {
            ++et;
            break;
        }
    }

    bool internal_spaces(false);
    int count(1);
    auto st(it);
    for(++st; st != et; ++st, ++count)
    {
        if(st->is_blank())
        {
            internal_spaces = true;
        }
        else if(*st != c)
        {
            return false;
        }
    }

std::cerr << "- * ---------------------------- DOCUMENT TREE SO FAR:\n";
std::cerr << f_document->tree();
std::cerr << "- * ---------------------------- DOCUMENT TREE SO FAR END ---\n";

std::cerr << "- * ---------------------------- WORKING BLOCK:\n";
std::cerr << f_working_block->tree();
std::cerr << "- * ---------------------------- WORKING BLOCK END ---\n";
std::cerr << "last block: " << (f_last_block->is_in_blockquote() ? "IN BLOCKQUOTE" : "not in block quote")
<< " & working block: " << (f_working_block->is_in_blockquote() ? "IN BLOCKQUOTE" : "not in block quote")
<< "\n";

    if(c.is_setext()
    && !internal_spaces
    && f_last_block != nullptr
    && f_last_block->is_paragraph()
    && !f_last_block->followed_by_an_empty_line()
    && f_last_block->is_in_blockquote() == f_working_block->is_in_blockquote()
    && f_last_block->parent() != nullptr && !f_last_block->parent()->is_list())
    {
std::cerr << "- * ---------------------------- LAST BLOCK:\n";
std::cerr << f_last_block->tree();
std::cerr << "- * ---------------------------- LAST BLOCK END ---\n";

std::cerr << "- * ---------------------------- TOP WORKING BLOCK:\n";
std::cerr << f_top_working_block->tree();
std::cerr << "- * ---------------------------- TOP WORKING BLOCK END ---\n";

        // [REF] 4.3 Setext headings
        //
        if(c == CHAR_DASH)
        {
            // the DASH is used by the list so the block type uses "single"
            // (an underscore at the moment, but we may change that later)
            //
            c.f_char = BLOCK_TYPE_HEADER_SINGLE;
        }
        block::pointer_t b(std::make_shared<block>(c));
        b->number(c.is_equal() ? 1 : 2);

std::cerr << " --- unlink: "
<< static_cast<int>(f_last_block->type().f_char)
<< " --- parent "
<< reinterpret_cast<void *>(f_last_block->parent().get())
<< " --- next "
<< reinterpret_cast<void *>(f_last_block->next().get())
<< " --- previous "
<< reinterpret_cast<void *>(f_last_block->previous().get())
<< "\n";

        b->append(f_last_block->content());
        f_last_block->unlink();
std::cerr << "- * ---------------------------- DOCUMENT TREE AFTER UNLINK\n";
std::cerr << f_document->tree();
std::cerr << "- * ---------------------------- DOCUMENT TREE AFTER UNLINK END ---\n";
        //block::pointer_t p(f_last_block->parent());
        //if(p == nullptr)
        //{
        //    throw unexpected_null_pointer("cannot properly unlink/re-link an Setext block");
        //}
        //p->link_child(b);

        f_working_block->link_child(b);
        f_working_block = b;
std::cerr << "- * ---------------------------- DOCUMENT TREE AFTER RELINK\n";
std::cerr << f_document->tree();
std::cerr << "- * ---------------------------- DOCUMENT TREE AFTER RELINK END ---\n";

        return true;
    }

    if(count < 3
    || c.is_equal())        // equal is only for the Setext
    {
        return false;
    }

    character type(c);
    switch(type.f_char)
    {
    case CHAR_DASH:
        type.f_char = BLOCK_TYPE_BREAK_DASH;
        break;

    case CHAR_ASTERISK:
        type.f_char = BLOCK_TYPE_BREAK_ASTERISK;
        break;

    case CHAR_UNDERSCORE:
        type.f_char = BLOCK_TYPE_BREAK_UNDERLINE;
        break;

    }
    block::pointer_t b(std::make_shared<block>(type));
    f_working_block->link_child(b);
    f_working_block = b;
std::cerr << "linked as child!\n";

    return true;
}


bool commonmark::process_reference_definition(character::string_t::const_iterator & it)
{
std::cerr << " ---- process_reference_definition()...\n";
    // [REF] 4.7 Link reference definitions
    //
    if(!it->is_open_square_bracket())
    {
std::cerr << " ---- not reference (1)...\n";
        return false;
    }

    auto et(it);

std::cerr << " ---- parse link text...\n";
    std::string reference_name;
    if(!parse_link_text(f_last_line, et, reference_name))
    {
std::cerr << " ---- not reference (2)...\n";
        return false;
    }

std::cerr << " ---- check for colon: " << static_cast<int>(et->f_char) << "...\n";
    if(et == f_last_line.cend()
    || !et->is_colon())
    {
std::cerr << " ---- not reference (3)...\n";
        return false;
    }
    ++et;

std::cerr << " ---- skip blanks...\n";
    for(++et; et != f_last_line.cend() && et->is_blank(); ++et);

    if(et == f_last_line.cend())
    {
        // TODO: support multi-line properly (so we can cancel)
        //
        get_line();
        it = f_last_line.cbegin();
        et = it;
std::cerr << " ---- skip blanks on next line...\n";
        for(++et; et != f_last_line.cend() && et->is_blank(); ++et);
        if(et == f_last_line.cend())
        {
std::cerr << " ---- not reference (4)...\n";
            return false;
        }
    }

std::cerr << " ---- parse link destination...\n";
    std::string link_destination;
    std::string link_title;
    if(!parse_reference_destination(
                          et
                        , link_destination
                        , link_title))
    {
std::cerr << " ---- not reference (5)...\n";
        return false;
    }

std::cerr << " ---- add result as link reference ["
<< reference_name
<< "] ["
<< link_destination
<< "] ["
<< link_title
<< "]...\n";
    add_link(
          reference_name
        , link_destination
        , link_title
        , true);

    it = et;
    return true;
}


bool commonmark::parse_reference_destination(
      character::string_t::const_iterator & et
    , std::string & link_destination
    , std::string & link_title)
{
    // inline link destination has to have an open parenthesis
    //
    if(et == f_last_line.cend())
    {
std::cerr << " ---- ref: EOF...\n";
        return false;
    }

    std::string destination;
    if(et->is_open_angle_bracket())
    {
        for(++et; ; ++et)
        {
            if(et != f_last_line.cend()
            || et->is_open_angle_bracket()
            || et->is_blank()
            || et->is_ctrl())
            {
std::cerr << " ---- ref: bad bracket/blank/ctrl...\n";
                return false;
            }
            if(et->is_close_angle_bracket())
            {
                ++et;
                break;
            }
            if(et->is_backslash())
            {
                ++et;
                if(et == f_last_line.cend()
                || !et->is_ascii_punctuation())
                {
                    --et;
                }
            }
            destination += et->to_utf8();
        }
    }
    else
    {
        for(; et != f_last_line.cend(); ++et)
        {
            if(et->is_blank()
            || et->is_ctrl())
            {
                break;
            }
            if(et->is_backslash())
            {
                ++et;
                if(et == f_last_line.cend()
                || !et->is_ascii_punctuation())
                {
                    --et;
                }
            }
            destination += et->to_utf8();
        }
    }

    // skip blanks
    //
    for(;; ++et)
    {
        if(et == f_last_line.cend())
        {
std::cerr << " ---- ref: EOF skipping blanks...\n";
            return false;
        }
        if(!et->is_blank())
        {
            break;
        }
    }

    std::string title;
    if(et->is_link_title_open_quote())
    {
        character quote(*et);
        if(quote.is_open_parenthesis())
        {
            quote.f_char = CHAR_CLOSE_PARENTHESIS;
        }

        for(++et; et != f_last_line.cend() && *et != quote; ++et)
        {
            if(quote.is_close_parenthesis()
            && et->is_open_parenthesis())
            {
std::cerr << " ---- ref: bad title quotes or ()?...\n";
                return false;
            }
            if(et->is_backslash())
            {
                ++et;
                if(et == f_last_line.cend()
                || !et->is_ascii_punctuation())
                {
                    --et;
                }
            }
            title += et->to_utf8();
        }
        if(et == f_last_line.cend())
        {
std::cerr << " ---- ref: title closing quotes missing...\n";
            return false;
        }
        ++et;
    }

    link_destination = destination;
    link_title = title;
    return true;
}


bool commonmark::process_header(character::string_t::const_iterator & it)
{
    character c(*it);
    auto st(it);

    int count(0);
    for(; st != f_last_line.end(); ++st, ++count)
    {
        if(!st->is_hash())
        {
            break;
        }
    }

    if(st != f_last_line.end()
    && !st->is_blank())
    {
        return false;
    }

    if(count < 1
    || count > 6)
    {
        return false;
    }
    it = st;

    for(; it != f_last_line.cend(); ++it)
    {
        if(!it->is_blank())
        {
            break;
        }
    }

    // this looks like a header, parse the end
    // and also remove spaces and #'s from there
    //
    auto et(f_last_line.cend());
    while(et != it)
    {
        --et;
        if(!et->is_blank())
        {
            break;
        }
    }
    if(et != f_last_line.cend())
    {
        auto kt(et + 1);
        if(et->is_hash())
        {
            for(; et != it; --et)
            {
                if(!et->is_hash())
                {
                    if(et->is_blank())
                    {
                        c.f_char = BLOCK_TYPE_HEADER_ENCLOSED;

                        for(;
                            et != it && et->is_blank();
                            --et);
                        ++et;
                    }
                    else
                    {
                        et = kt;
                    }
                    break;
                }
            }
        }
        else
        {
            et = kt;
        }
    }

    block::pointer_t b(std::make_shared<block>(c));
    b->number(count);
    b->append(character::string_t(it, et));
    f_working_block->link_child(b);
    f_working_block = b;

    return true;
}


bool commonmark::process_indented_code_block(character::string_t::const_iterator & it)
{
    // [REF] 4.4 Indented code blocks
    //

//    for(;;)
//    {
//        if(it == line.end())
//        {
//            return false;
//        }
//std::cerr << "it char: " << static_cast<int>(it->f_char)
//<< " at column " << it->f_column
//<< std::endl;
//        if(it->f_column > f_indentation * 4 + 4)
//        {
//            break;
//        }
//        if(!it->is_space()
//            && !it->is_tab())
//        {
//            return false;
//        }
//        ++it;
//    }

    // a tab that "leaks" over 4 characters gets removed altogether
    //
//std::cerr << "it is now: " << static_cast<int>(it->f_char)
//<< " -> column " << it->f_column
//<< " (indent: " << f_indentation << ")"
//<< std::endl;
//    if(it->is_tab()
//    && it->f_column <= f_indentation * 4 + 4)
//    {
//        ++it;
//    }

std::cerr << "+++ adding a CODE BLOCK here (indent: "
<< f_list_subblock
<< ")\n";
    character code_block(*it);
    code_block.f_char = BLOCK_TYPE_CODE_BLOCK_INDENTED;
    block::pointer_t b(std::make_shared<block>(code_block));
    //if(it != f_last_line.cbegin())
    //{
    //    auto st(it);
    //    --st;
    //    if(st->is_tab())
    //    {
    //        // tabs need special handling if the indentation is not
    //        // an exact match
    //        //
    //    }
    //}
    std::uint32_t indent(f_list_subblock);
    if(indent == 0
    && f_working_block->is_blockquote())
    {
std::cerr << " ---- blockquote ("
<< f_working_block->end_column()
<< ") + code block\n";
        indent = b->column() - f_working_block->end_column() + 1;
    }
std::cerr << " ---- got indent of " << indent << " vs column " << b->column() << "\n";
    if(indent > 0)
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        character const space{
            .f_char = CHAR_SPACE,
            .f_line = it->f_line,
            .f_column = it->f_column,
        };
#pragma GCC diagnostic pop

        for(std::uint32_t column(indent);
            column < it->f_column;
            ++column)
        {
            b->append(space);
        }
    }
    b->append(character::string_t(it, f_last_line.cend()));

    character c(*f_last_line.crbegin());
    c.f_char = CHAR_LINE_FEED;
    b->append(c);

    f_working_block->link_child(b);
    f_working_block = b;

    return true;
}


bool commonmark::process_fenced_code_block(character::string_t::const_iterator & it)
{
    // [REF] 4.5 Fenced code blocks
    //
    if(!it->is_fenced_code_block())
    {
        return false;
    }

    std::string::size_type count(1);
    auto et(it);
    for(++et; et != f_last_line.cend() && et->is_fenced_code_block(); ++et, ++count);

    if(count < 3)
    {
        return false;
    }

    // remove the blanks before the info-string
    //
    for(; et != f_last_line.cend() && et->is_blank(); ++et);

    if(*et == *it
    || et->is_grave())
    {
        return false;
    }

    character::string_t info_string;
    for(; et != f_last_line.cend(); ++et)
    {
        if(et->is_grave()
        && it->is_grave())
        {
            // a backtick (`) anywhere is viewed as a span code
            // (but not tilde)
            //
            return false;
        }

        if(*et != *it
        && !et->is_grave())
        {
            info_string += *et;
        }
    }

    //
    // this is a fenced code block
    //

    character code_block(*it);  // ~ or `
    block::pointer_t b(std::make_shared<block>(code_block));
    b->info_string(info_string);
    std::uint32_t const indent(it->f_column);
std::cerr << " --- adding info string [" << info_string
<< "] -- indentation: " << indent << "\n";

    bool const inside_list(f_working_block->is_list());
    bool const inside_blockquote(f_working_block->is_in_blockquote());

    auto original_top_working_block(f_top_working_block);
    auto original_working_block(f_working_block);

    for(;;)
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        f_top_working_block = std::make_shared<block>(character{
                .f_char = BLOCK_TYPE_LINE,
                .f_line = f_line,
                .f_column = 1,
            });
#pragma GCC diagnostic pop
        f_working_block = f_top_working_block;

        get_line();
std::cerr << " --- got next line [" << f_last_line << "]\n";

        if(inside_list
        || inside_blockquote)
        {
            it = parse_containers();

            if(inside_blockquote
            && !f_working_block->is_in_blockquote())
            {
                break;
            }
        }
        else
        {
            it = f_last_line.cbegin();
        }

        // end of file found?
        //
        // the spec. clearly says we do not want to back out of this one
        //
        if(f_working_block->is_line()
        && it == f_last_line.cend())
        //if(f_last_line.empty()
        //&& f_eos)
        {
            if(f_eos)
            {
std::cerr << " --- found eof?!\n";
                break;
            }
std::cerr << " --- what about an empty line?\n";
        }

        // skip indented code
        //
        //adjust = 0; // TODO: if inside blockquote or list?
std::cerr << " --- check indent: " << it->f_column << " vs " << indent << "\n";
        for(; it->f_column < indent && it->is_blank(); ++it);

        // end marker?
        //
        if(f_last_line.cend() - it >= static_cast<character::string_t::const_iterator::difference_type>(count))
        {
            std::string::size_type end_count(0);

            auto ec(it);
            for(;
                ec != f_last_line.cend()
                    && ec->is_blank();
                ++ec);

            if(ec->f_column - it->f_column < 4)
            {
                for(;
                    ec != f_last_line.cend()
                        && *ec == code_block;
                    ++ec, ++end_count);

std::cerr << " --- end count: " << end_count << "\n";
                if(end_count >= count)
                {
                    for(;
                        ec != f_last_line.cend()
                            && ec->is_blank();
                        ++ec);

                    if(ec == f_last_line.cend())
                    {
std::cerr << " --- found end mark?!\n";
                        break;
                    }
                }
            }
        }

        // however, we may now be too far and have to prepend spaces
        // (i.e. if there is a tab instead of a space we may "skip"
        // the correct column)
        //
        for(std::int32_t mismatch(it->f_column - indent);
            mismatch > 0;
            --mismatch)
        {
std::cerr << " --- mismatch: " << mismatch << "\n";
            character c(*it);
            c.f_char = CHAR_SPACE;
            c.f_column -= mismatch;
            b->append(c);
        }

std::cerr << " --- add: " << character::string_t(it, f_last_line.cend()) << "\n";
        b->append(character::string_t(it, f_last_line.cend()));

        character c(*f_last_line.crbegin());
        c.f_char = CHAR_LINE_FEED;
        ++c.f_column;
        b->append(c);
    }

std::cerr << " --- done\n";
    f_top_working_block = original_top_working_block;
    f_working_block = original_working_block;
    f_working_block->link_child(b);
    f_working_block = b;

    return true;
}


bool commonmark::process_html_blocks(character::string_t::const_iterator & it)
{
    // [REF] 4.6 HTML blocks
    //
std::cerr << "   it on entry: [" << reinterpret_cast<void const *>(&*it) << "]\n";
    if(!it->is_open_angle_bracket())
    {
        return false;
    }

    character tag_block(*it);  // <
    block::pointer_t b(std::make_shared<block>(tag_block));

    // keep the blanks before the tag (why is markcommond doing that?!?!?)
    //
    bool const is_list(f_working_block->is_list());
    block::pointer_t const blockquote(f_working_block->find_blockquote());
    std::uint32_t start_column(
            blockquote != nullptr
                ? static_cast<std::uint32_t>(blockquote->end_column())
                : (is_list
                    ? static_cast<std::uint32_t>(f_working_block->end_column()) + 1
                    : static_cast<std::uint32_t>(1)));
std::cerr << "   it column " << it->f_column << " vs " << start_column << "\n";
    for(; start_column < it->f_column; ++start_column)
    {
        character c;
        c.f_char = CHAR_SPACE;
        b->append(c);
    }

    auto et(it);
    ++et;
    if(et == f_last_line.cend())
    {
        return false;
    }

    if(et->is_exclamation_mark())
    {
        ++et;
        if(et != f_last_line.cend()
        && et->is_dash())
        {
            ++et;
            if(et != f_last_line.cend()
            && et->is_dash())
            {
                // comment, ends on a '-->'
                //
                character processing_block(*it);  // <
                int state(0);
std::cerr << " * checking for comment: " << f_last_line << "\n";
                for(++et;;)
                {
std::cerr << "  +---> state " << state
<< " current end of string: [" << character::string_t(et, f_last_line.cend()) << "]\n";
                    if(et == f_last_line.cend())
                    {
                        b->append(character::string_t(it, f_last_line.cend()));

                        if(f_last_line.empty())
                        {
                            character c;
                            c.f_char = CHAR_LINE_FEED;
                            b->append(c);
                        }
                        else
                        {
                            character c(*f_last_line.crbegin());
                            c.f_char = CHAR_LINE_FEED;
                            ++c.f_column;
                            b->append(c);
                        }

                        get_line();
                        if(f_last_line.empty()
                        && f_eos)
                        {
                            break;
                        }
                        it = f_last_line.cbegin();
                        et = it;
std::cerr << "  +---> checking next line: " << f_last_line << "\n";
                    }
                    else if(state == 0)
                    {
                        if(et->is_dash())
                        {
                            state = 1;
                        }
                        ++et;
                    }
                    else if(state == 1)
                    {
                        if(et->is_dash())
                        {
                            state = 2;
                        }
                        else
                        {
                            state = 0;
                        }
                        ++et;
                    }
                    else if(state == 2)
                    {
                        if(et->is_close_angle_bracket())
                        {
                            break;
                        }
                        else if(!et->is_dash())
                        {
                            state = 0;
                        }
                        ++et;
                    }
                }
                b->append(character::string_t(it, f_last_line.cend()));

                character c(*f_last_line.crbegin());
                c.f_char = CHAR_LINE_FEED;
                ++c.f_column;
                b->append(c);

                f_working_block->link_child(b);
                f_working_block = b;

                it = f_last_line.cend();

                return true;
            }
            --et;
        }
        if(et != f_last_line.cend()
        && et->is_open_square_bracket())
        {
            ++et;
            if(et != f_last_line.cend()
            && et->f_char != 'C')
            {
                return false;
            }
            ++et;
            if(et != f_last_line.cend()
            && et->f_char != 'D')
            {
                return false;
            }
            ++et;
            if(et != f_last_line.cend()
            && et->f_char != 'A')
            {
                return false;
            }
            ++et;
            if(et != f_last_line.cend()
            && et->f_char != 'T')
            {
                return false;
            }
            ++et;
            if(et != f_last_line.cend()
            && et->f_char != 'A')
            {
                return false;
            }
            ++et;
            if(et != f_last_line.cend()
            && !et->is_open_square_bracket())
            {
                return false;
            }

            // <![CDATA[ ... ]]>
            //
            character processing_block(*it);  // <
std::cerr << "* identity first line: " << f_last_line << "\n";
            int state(0);
            for(++et;;)
            {
                if(et == f_last_line.cend())
                {
                    b->append(character::string_t(it, f_last_line.cend()));

                    if(f_last_line.empty())
                    {
                        character c;
                        c.f_char = CHAR_LINE_FEED;
                        b->append(c);
                    }
                    else
                    {
                        character c(*f_last_line.crbegin());
                        c.f_char = CHAR_LINE_FEED;
                        ++c.f_column;
                        b->append(c);
                    }

                    get_line();
                    if(f_last_line.empty()
                    && f_eos)
                    {
                        break;
                    }
                    it = f_last_line.cbegin();
                    et = it;
std::cerr << "  +---> checking next line: " << f_last_line << "\n";
                }
                else if(state == 0)
                {
                    if(et->is_close_square_bracket())
                    {
                        state = 1;
                    }
                    ++et;
                }
                else if(state == 1)
                {
                    if(et->is_close_square_bracket())
                    {
                        state = 2;
                    }
                    else
                    {
                        state = 0;
                    }
                    ++et;
                }
                else if(state == 2)
                {
                    if(et->is_close_angle_bracket())
                    {
                        break;
                    }
                    else if(!et->is_close_square_bracket())
                    {
                        state = 0;
                    }
                    ++et;
                }
                else
                {
                    ++et;
                }
            }
            b->append(character::string_t(it, f_last_line.cend()));

            character c(*f_last_line.crbegin());
            c.f_char = CHAR_LINE_FEED;
            ++c.f_column;
            b->append(c);

            f_working_block->link_child(b);
            f_working_block = b;

            it = f_last_line.cend();

            return true;
        }
        if(et != f_last_line.cend()
        && et->is_ascii_letter())
        {
            // identity, ends on a '>'
            //
            character processing_block(*it);  // <
std::cerr << "* identity first line: " << f_last_line << "\n";
            for(++et;;)
            {
                if(et == f_last_line.cend())
                {
                    b->append(character::string_t(it, f_last_line.cend()));

                    if(f_last_line.empty())
                    {
                        character c;
                        c.f_char = CHAR_LINE_FEED;
                        b->append(c);
                    }
                    else
                    {
                        character c(*f_last_line.crbegin());
                        c.f_char = CHAR_LINE_FEED;
                        ++c.f_column;
                        b->append(c);
                    }

                    get_line();
                    if(f_last_line.empty()
                    && f_eos)
                    {
                        break;
                    }
                    it = f_last_line.cbegin();
                    et = it;
std::cerr << "  +---> checking next line: " << f_last_line << "\n";
                }
                else if(et->is_close_angle_bracket())
                {
                    break;
                }
                else
                {
                    ++et;
                }
            }
            b->append(character::string_t(it, f_last_line.cend()));

            character c(*f_last_line.crbegin());
            c.f_char = CHAR_LINE_FEED;
            ++c.f_column;
            b->append(c);

            f_working_block->link_child(b);
            f_working_block = b;

            it = f_last_line.cend();

            return true;
        }
        return false;
    }

    if(et->is_question_mark())
    {
        // <? ... ?>
        //
        character processing_block(*it);  // <
        bool got_question_mark(false);
        for(++et;;)
        {
            if(et == f_last_line.cend())
            {
                b->append(character::string_t(it, f_last_line.cend()));

                if(f_last_line.empty())
                {
                    character c;
                    c.f_char = CHAR_LINE_FEED;
                    b->append(c);
                }
                else
                {
                    character c(*f_last_line.crbegin());
                    c.f_char = CHAR_LINE_FEED;
                    ++c.f_column;
                    b->append(c);
                }

                get_line();
                if(f_last_line.empty()
                && f_eos)
                {
                    break;
                }
                it = f_last_line.cbegin();
                et = it;
                got_question_mark = false;
            }
            else if(!got_question_mark)
            {
                ++et;
                got_question_mark = true;
            }
            else if(et->is_close_angle_bracket())
            {
                // note: as per the docs, when we find the ?>, anything
                //       after that is also viewed as HTML so just add
                //       the whole line and return
                //
                break;
            }
            else
            {
                ++et;
                got_question_mark = false;
            }
        }
        b->append(character::string_t(it, f_last_line.cend()));

        if(f_last_line.empty())
        {
            character c;
            c.f_char = CHAR_LINE_FEED;
            b->append(c);
        }
        else
        {
            character c(*f_last_line.crbegin());
            c.f_char = CHAR_LINE_FEED;
            ++c.f_column;
            b->append(c);
        }

        f_working_block->link_child(b);
        f_working_block = b;

        it = f_last_line.cend();

        return true;
    }

    bool const closing(et->is_slash());
    if(closing)
    {
        ++et;
    }

    if(!et->is_first_tag())
    {
        return false;
    }

    // tag
    //
    std::string tag;
    if(et->is_ascii_letter())
    {
        tag += static_cast<char>(et->f_char) | 0x20;
    }
    else
    {
        tag += static_cast<char>(et->f_char);
    }
    for(++et; et != f_last_line.cend(); ++et)
    {
        if(!et->is_tag())
        {
            break;
        }
        if(et->is_ascii_letter())
        {
            // force lowercase so our tests below work with just lowercase
            //
            tag += static_cast<char>(et->f_char) | 0x20;
        }
        else
        {
            tag += static_cast<char>(et->f_char);
        }
    }
std::cerr << "   it after reading name \"" << tag << "\": ["
<< reinterpret_cast<void const *>(&*it)
<< "] ... et ["
<< reinterpret_cast<void const *>(&*et)
<< "]\n";

    // the tag name must be followed by
    //
    //    * a space
    //    * a tab
    //    * ">"
    //    * "/>"
    //    * EOL
    //
    if(et != f_last_line.cend()
    && !et->is_blank()
    && !et->is_close_angle_bracket())
    {
        if(et->is_slash())
        {
            ++et;
            if(!et->is_close_angle_bracket())
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    bool const closed(et != f_last_line.cend()
                   && et->is_close_angle_bracket());
    if(closed)
    {
        ++et;
    }

    // in this case the end of the tag is determined by its name
    //
    bool end_with_empty_line(true); // if false: search for </pre>, </script>, </style>, or </textarea>
    bool complete_tag(true);        // if true: verify that the tag is 100% "valid" (as per markdown)
    switch(tag[0])
    {
    case 'a':
        if(tag == "address"
        || tag == "article"
        || tag == "aside")
        {
            complete_tag = false;
        }
        break;

    case 'b':
        if(tag == "base"
        || tag == "basefont"
        || tag == "blockquote"
        || tag == "body")
        {
            complete_tag = false;
        }
        break;

    case 'c':
        if(tag == "caption"
        || tag == "center"
        || tag == "col"
        || tag == "colgroup")
        {
            complete_tag = false;
        }
        break;

    case 'd':
        if(tag == "dd"
        || tag == "details"
        || tag == "dialog"
        || tag == "dir"
        || tag == "div"
        || tag == "dl"
        || tag == "dt")
        {
            complete_tag = false;
        }
        break;

    case 'f':
        if(tag == "fieldset"
        || tag == "figcaption"
        || tag == "figure"
        || tag == "footer"
        || tag == "form"
        || tag == "frame"
        || tag == "frameset")
        {
            complete_tag = false;
        }
        break;

    case 'h':
        if(tag == "h1"
        || tag == "h2"
        || tag == "h3"
        || tag == "h4"
        || tag == "h5"
        || tag == "h6"
        || tag == "head"
        || tag == "header"
        || tag == "hr"
        || tag == "html")
        {
            complete_tag = false;
        }
        break;

    case 'i':
        if(tag == "iframe")
        {
            complete_tag = false;
        }
        break;

    case 'l':
        if(tag == "legend"
        || tag == "li"
        || tag == "link")
        {
            complete_tag = false;
        }
        break;

    case 'm':
        if(tag == "main"
        || tag == "menu"
        || tag == "menuitem")
        {
            complete_tag = false;
        }
        break;

    case 'n':
        if(tag == "nav"
        || tag == "noframes")
        {
            complete_tag = false;
        }
        break;

    case 'o':
        if(tag == "ol"
        || tag == "optgroup"
        || tag == "option")
        {
            complete_tag = false;
        }
        break;

    case 'p':
        if(tag == "p"
        || tag == "param")
        {
            complete_tag = false;
        }
        else if(!closing
             && tag == "pre")
        {
            end_with_empty_line = false;
            complete_tag = false;
        }
        break;

    case 's':
        if(tag == "section"
        || tag == "source"
        || tag == "summary")
        {
            complete_tag = false;
        }
        else if(!closing
             && (tag == "script"
             || tag == "style"))
        {
            end_with_empty_line = false;
            complete_tag = false;
        }
        break;

    case 't':
        if(tag == "table"
        || tag == "tbody"
        || tag == "td"
        || tag == "tfoot"
        || tag == "th"
        || tag == "thead"
        || tag == "title"
        || tag == "tr"
        || tag == "track")
        {
            complete_tag = false;
        }
        else if(!closing
             && tag == "textarea")
        {
            end_with_empty_line = false;
            complete_tag = false;
        }
        break;

    case 'u':
        if(tag == "ul")
        {
            complete_tag = false;
        }
        break;

    default:
        // the default is already set as expected (user defined tag name)
        break;

    }

    if(complete_tag)
    {
        if(f_last_block->is_paragraph()
        && !f_last_block->followed_by_an_empty_line())
        {
            // this is viewed as an inline tag
            //
            return false;
        }
    }

    b->append(character::string_t(it, et));
    auto st(et);
std::cerr << " ---- append tag intro to tag_block"
<< (end_with_empty_line ? " -- END WITH EMPTY LINE" : "")
<< (complete_tag ? " -- COMPLETE TAG" : "")
<< (closed ? " -- CLOSED" : "")
<< " -- column limit: " << start_column
<< (blockquote == nullptr ? "" : " from BLOCKQUOTE")
<< (is_list ? " LIST ITEM" : "")
<< "\n";
std::cerr << "- * -------------------------------- HTML BLOCK START:\n"
<< b->tree()
<< "- * -------------------------------- HTML BLOCK START END ---\n";

std::cerr << "- * -------------------------------- LAST BLOCK SO FAR:\n"
<< f_working_block->tree()
<< "- * -------------------------------- LAST BLOCK SO FAR END ---\n";

    // remove start of tag
    //
//std::cerr << " ---- restart line...\n";
//    f_last_line = character::string_t(et, f_last_line.cend());

    // user tags need to be "well defined" (as per commonmark)
    //
    // if already marked as closed, then it is already considered complete
    //
    if(!closed
    && complete_tag)
    {
std::cerr << " ---- tag needs to be complete ("
<< character::string_t(it, f_last_line.cend())
<< ")...\n";
        // WARNING: at this point, we can't back out of an invalid multi-line
        //          tag block definition
        //
        if(!verify_tag_attributes(f_last_line, et))
        {
            return false;
        }
    }

    if(complete_tag)
    {
        // a complete tag cannot be followed by anything other than blanks
        //
        for(; et != f_last_line.cend() && et->is_blank(); ++et);
        if(et != f_last_line.cend())
        {
std::cerr << " ---- not complete (followed by something other than blanks)\n";
            return false;
        }
    }

std::cerr << " ---- build tag block now\n";
    bool found(false);
    for(;;)
    {
        if(end_with_empty_line)
        {
            if(f_last_line.empty())
            {
                // finished with this tag block
                //
std::cerr << " ---- tag ended with an empty line\n";
                b->followed_by_an_empty_line(true);
                break;
            }
        }
        else
        {
            if(f_last_line.empty()
            && f_eos)
            {
                // found the end of the file, this is considered the end
                // of the tag block if nothing else ended the tag yet
                //
                break;
            }

            // </pre>, </script>, </style>, or </textarea>

            for(auto ci(st);
                ci != f_last_line.cend() && !found;
                ++ci)
            {
                while(ci != f_last_line.cend()
                   && ci->is_open_angle_bracket())
                {
                    ++ci;
                    if(ci->is_slash())
                    {
                        std::string closing_tag;
                        for(++ci; ci != f_last_line.cend(); ++ci)
                        {
                            if(!ci->is_ascii_letter())
                            {
                                for(;
                                    ci != f_last_line.cend()
                                            && ci->is_blank();
                                    ++ci);
                                break;
                            }
                            closing_tag += static_cast<char>(ci->f_char) | 0x20; // force lowercase
                        }
                        found = ci != f_last_line.cend()
                             && ci->is_close_angle_bracket()
                             && (closing_tag == "pre"
                                 || closing_tag == "script"
                                 || closing_tag == "style"
                                 || closing_tag == "textarea");
                        if(found)
                        {
                            //++ci;   // the '>'
                            //b->append(character::string_t(st, ci));
                            b->append(character::string_t(st, f_last_line.cend()));
                            it = ci;

                            // TBD: should we NOT add the \n if ci != cend()?
                            //
                            character c(*f_last_line.crbegin());
                            c.f_char = CHAR_LINE_FEED;
                            ++c.f_column;
                            b->append(c);

                            // TODO: re-inject end of line? (ci, f_last_line.cend())
                            //
                            f_working_block->link_child(b);
                            f_working_block = b;

                            return true;
                        }
                    }
                }
            }
        }
std::cerr << " ---- append whole line before closing tag or empty line...\n";
        b->append(character::string_t(st, f_last_line.cend()));

        if(f_last_line.empty())
        {
            character c;
            c.f_char = CHAR_LINE_FEED;
            b->append(c);
        }
        else
        {
            character c(*f_last_line.crbegin());
            c.f_char = CHAR_LINE_FEED;
            ++c.f_column;
            b->append(c);
        }
std::cerr << "- * -------------------------------- HTML BLOCK APPEND LOOP:\n"
<< b->tree()
<< "- * -------------------------------- HTML BLOCK APPEND LOOP END ---\n";

        input_status_t const saved_status(get_current_status());

        get_line();

        if(blockquote != nullptr
        || is_list)
        {
            // TODO; I don't think this is correct, it currently
            //       works with example 174 for now...
            //
            auto top_working_block(f_top_working_block);
            auto working_block(f_working_block);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
            f_top_working_block = std::make_shared<block>(character{
                    .f_char = BLOCK_TYPE_LINE,
                    .f_line = f_line,
                    .f_column = 1,
                });
#pragma GCC diagnostic pop
            f_working_block = f_top_working_block;

            it = parse_containers();
            bool breaking_block(!f_working_block->is_in_blockquote()
                             || f_working_block->is_list());

            f_top_working_block = top_working_block;
            f_working_block = working_block;

            if(breaking_block)
            {
                restore_status(saved_status);
                break;
            }
        }
        else
        {
            it = f_last_line.cbegin();
        }

        st = it;
    }

    f_working_block->link_child(b);
    f_working_block = b;

    it = f_last_line.cend();

    return true;
}


/** \brief Transform all the blocks in HTML.
 *
 * Go through all the blocks one by one and generate the corresponding
 * HTML.
 *
 * \param[in] b  The block to transform to HTML.
 */
void commonmark::generate(block::pointer_t b)
{
    for(;
        b != nullptr;
        b = b->next())
    {
        switch(b->type().f_char)
        {
        case BLOCK_TYPE_DOCUMENT:
            if(f_features.get_add_document_div())
            {
                if(f_features.get_add_classes())
                {
                    f_output += "<div class=\"cm-document\">";
                }
                else
                {
                    f_output += "<div>";
                }
                generate(b->first_child());
                f_output += "</div>";
            }
            else
            {
                generate(b->first_child());
            }
            break;

        case BLOCK_TYPE_PARAGRAPH:
            f_output += "<p>";
            generate_inline(b->content());
            f_output += "</p>\n";
            break;

        case BLOCK_TYPE_CODE_BLOCK_INDENTED:
        case BLOCK_TYPE_CODE_BLOCK_GRAVE:
        case BLOCK_TYPE_CODE_BLOCK_TILDE:
            f_output += "<pre>";
            generate_code(b);
            f_output += "</pre>\n";
            break;

        case BLOCK_TYPE_BLOCKQUOTE:
            // instead of adding blocks of type blockquote, we increase the
            // level; but here we have to generate L <blockquote> tags
            //
            for(int count(0); count < b->number(); ++count)
            {
                f_output += "<blockquote>\n";
            }
            generate(b->first_child());
            for(int count(0); count < b->number(); ++count)
            {
                f_output += "</blockquote>\n";
            }
            break;

        case BLOCK_TYPE_LIST_ASTERISK:
        case BLOCK_TYPE_LIST_PLUS:
        case BLOCK_TYPE_LIST_DASH:
        case BLOCK_TYPE_LIST_PERIOD:
        case BLOCK_TYPE_LIST_PARENTHESIS:
            generate_list(b);
            break;

        case BLOCK_TYPE_HEADER_OPEN:
        case BLOCK_TYPE_HEADER_ENCLOSED:
        case BLOCK_TYPE_HEADER_SINGLE:
        case BLOCK_TYPE_HEADER_DOUBLE:
            generate_header(b);
            break;

        case BLOCK_TYPE_BREAK_DASH:
        case BLOCK_TYPE_BREAK_ASTERISK:
        case BLOCK_TYPE_BREAK_UNDERLINE:
            generate_thematic_break(b);
            break;

        case BLOCK_TYPE_TAG:
            // copy verbatim
            //
            f_output += character::to_utf8(b->content());
            //f_output += '\n'; -- added when read
            break;

        default:
            throw commonmark_logic_error(
                      "unrecognized block type ("
                    + std::to_string(static_cast<int>(b->type().f_char))
                    + ") while generate HTML data"
                );

        }
    }
}


void commonmark::generate_list(block::pointer_t & b)
{
    // open the tag
    //
    std::string tag;
    if(b->is_ordered_list())
    {
        tag = "ol";
        f_output += "<ol";
        if(b->number() != 1)
        {
            f_output += " start=\"";
            f_output += std::to_string(b->number());
            f_output += '"';
        }
    }
    else
    {
        tag = "ul";
        f_output += "<ul";
    }

    if(f_features.get_add_classes())
    {
        switch(b->type().f_char)
        {
        case BLOCK_TYPE_LIST_ASTERISK:
            f_output += " class=\"cm-asterisk\"";
            break;

        case BLOCK_TYPE_LIST_PLUS:
            f_output += " class=\"cm-plus\"";
            break;

        case BLOCK_TYPE_LIST_DASH:
            f_output += " class=\"cm-dash\"";
            break;

        case BLOCK_TYPE_LIST_PERIOD:
            f_output += " class=\"cm-period\"";
            break;

        case BLOCK_TYPE_LIST_PARENTHESIS:
            f_output += " class=\"cm-parenthesis\"";
            break;

        }
    }

    f_output += ">\n";

    // create all the items
    //
    for(;; b = b->next())
    {
        // if the list item is not sparse, make sure to generate the
        // output as inline data instead of a paragraph
        //
        f_output += "<li>";
        if(!b->followed_by_an_empty_line()
        && b->first_child() != nullptr
        && b->first_child()->is_paragraph())
        {
            generate_inline(b->first_child()->content());

            // there can be more sub-items
            //
            if(b->first_child()->next() != nullptr)
            {
                f_output += f_features.get_line_feed();
                generate(b->first_child()->next());
            }
        }
        else
        {
            f_output += f_features.get_line_feed();
            generate(b->first_child());
        }
        f_output += "</li>";
        f_output += f_features.get_line_feed();

        if(b->next() == nullptr)
        {
            break;
        }

        if(!b->next()->is_list())
        {
            break;
        }
    }

    // close the tag
    //
    f_output += "</";
    f_output += tag;
    f_output += ">\n";
}


void commonmark::generate_header(block::pointer_t b)
{
    f_output += "<h";
    f_output += std::to_string(b->number());
    if(f_features.get_add_classes())
    {
        switch(b->type().f_char)
        {
        case BLOCK_TYPE_HEADER_OPEN:
            f_output += " class=\"cm-header-open\"";
            break;

        case BLOCK_TYPE_HEADER_ENCLOSED:
            f_output += " class=\"cm-header-enclosed\"";
            break;

        case BLOCK_TYPE_HEADER_SINGLE:
            f_output += " class=\"cm-header-underline cm-header-dash\"";
            break;

        case BLOCK_TYPE_HEADER_DOUBLE:
            f_output += " class=\"cm-header-underline cm-header-equal\"";
            break;

        }
    }
    f_output += ">";

    // note: an empty title is considered valid by commonmark
    //
    generate_inline(b->content());

    f_output += "</h";
    f_output += std::to_string(b->number());
    f_output += ">";
    f_output += f_features.get_line_feed();
}


void commonmark::generate_thematic_break(block::pointer_t b)
{
    f_output += "<hr";

    if(f_features.get_add_classes())
    {
        f_output += " class=\"";

        switch(b->type().f_char)
        {
        case BLOCK_TYPE_BREAK_DASH:
            f_output += "cm-break-dash";
            break;

        case BLOCK_TYPE_BREAK_UNDERLINE:
            f_output += "cm-break-underline";
            break;

        case BLOCK_TYPE_BREAK_ASTERISK:
            f_output += "cm-break-asterisk";
            break;

        }
        f_output += '"';
    }

    if(f_features.get_add_space_in_empty_tag())
    {
        f_output += ' ';
    }
    f_output += "/>";
    f_output += f_features.get_line_feed();
}


void commonmark::generate_inline(character::string_t const & line)
{
std::cerr << " ---- inline to parse: [" << line << "]\n";

    class inline_parser
    {
    public:
        inline_parser(
                  character::string_t const & line
                , features const & f
                , link::find_link_reference_t find_link_reference)
            : f_line(line)
            , f_it(f_line.cbegin())
            , f_features(f)
            , f_find_link_reference(find_link_reference)
        {
        }

        std::string run()
        {
            std::string result;
            while(f_it != f_line.cend())
            {
                result += convert_char();
            }
            return result;
        }

        std::string convert_char()
        {
            std::string result;
            character previous;
            if(f_it != f_line.cbegin())
            {
                previous = f_it[-1];
            }

            switch(f_it->f_char)
            {
            case CHAR_SPACE:
            case CHAR_TAB:
                // if we only find blanks to the end of the content, then
                // we can ignore it (trim to the right)
                {
                    auto et(f_it);
                    for(++et; et != f_line.cend() && et->is_blank(); ++et);
                    if(et == f_line.cend())
                    {
                        f_it = et;
                        break;
                    }
                }
                ++f_it;
                if(f_it != f_line.cend()
                && f_it->is_blank())
                {
                    ++f_it;
                    if(f_it != f_line.cend()
                    && f_it->is_eol())
                    {
                        // [REF] 6.7 Hard line breaks
                        //
                        if(f_features.get_add_space_in_empty_tag())
                        {
                            result += "<br />";
                        }
                        else
                        {
                            result += "<br/>";
                        }
                        //++f_it;
                        break;
                    }
                    --f_it;
                }
                result += (f_it - 1)->to_utf8();
                break;

            case CHAR_AMPERSAND:
                result += convert_ampersand(f_line, f_it, f_features.get_convert_entities());
                break;

            case CHAR_GRAVE: // inline code
                result += convert_inline_code();
                break;

            case CHAR_OPEN_ANGLE_BRACKET: // inline HTML tag (or not)
                result += convert_html_tag();
                break;

            case CHAR_ASTERISK: // italic, bold, strikethrough, underline
            case CHAR_UNDERSCORE:
                result += convert_span(previous);
                break;

            case CHAR_DASH:
            case CHAR_PLUS:
                if(f_features.get_ins_del_extension())
                {
                    result += convert_span(previous);
                }
                else
                {
                    result += convert_basic_char();
                }
                break;

            case CHAR_BACKSLASH:
                ++f_it;
                if(f_it != f_line.cend()
                && f_it->is_ascii_punctuation())
                {
                    result += convert_basic_char();
                }
                else if(f_it != f_line.end()
                     && f_it->is_eol())
                {
                    // [REF] 6.7 Hard line breaks
                    //
                    if(f_features.get_add_space_in_empty_tag())
                    {
                        result += "<br />";
                    }
                    else
                    {
                        result += "<br/>";
                    }
                    //++f_it; -- we want the \n to be added
                }
                else
                {
                    result += '\\';
                }
                break;

            case CHAR_OPEN_SQUARE_BRACKET:
                result += convert_link(false);
                break;

            case CHAR_EXCLAMATION_MARK:
                ++f_it;
                if(f_it != f_line.cend()
                && f_it->is_open_square_bracket())
                {
                    result += convert_link(true);
                }
                else
                {
                    // keep the '!' as is otherwise
                    //
                    result += '!';
                    --f_it;
                }
                break;

            default:
                result += convert_basic_char();
                break;

            }

            return result;
        }

        std::string convert_inline_code()
        {
            // [REF] 6.1 Code spans
            //
            std::string mark("`"); // the start & end mark must match in length, length which is not limited
            for(++f_it;
                f_it != f_line.cend() && f_it->is_grave();
                ++f_it)
            {
                mark += '`';
            }

            // WARNING: the following is NOT a span if we find an end mark
            //          since the characters within represent code (i.e. which
            //          means *blah* is not an emphasis, etc.)
            //
            for(auto code(f_it); code != f_line.cend(); ++code)
            {
                auto et(code);

                bool found_mark(true);
                for(auto c : mark)
                {
                    if(et != f_line.cend()
                    && et->f_char != static_cast<char32_t>(c))
                    {
                        found_mark = false;
                        break;
                    }
                    ++et;
                }

                if(found_mark)
                {
                    std::string result;

                    bool blank(true);
                    for(; f_it != code; ++f_it)
                    {
                        switch(f_it->f_char)
                        {
                        case CHAR_SPACE:
                        case CHAR_LINE_FEED:
                            result += ' ';
                            break;

                        case CHAR_TAB:
                            result += '\t';
                            break;

                        case CHAR_AMPERSAND:
                            blank = false;
                            result += "&amp;";
                            break;

                        case CHAR_OPEN_ANGLE_BRACKET:
                            blank = false;
                            result += "&lt;";
                            break;

                        case CHAR_CLOSE_ANGLE_BRACKET:
                            blank = false;
                            result += "&gt;";
                            break;

                        default:
                            blank = false;
                            result += f_it->to_utf8();
                            break;

                        }
                    }

                    // then jump after the mark
                    //
                    f_it = et;

                    // trim exactly one space if one is found on each side
                    //
                    if(!blank
                    && result.length() > 2
                    && result.front() == ' '
                    && result.back() == ' ')
                    {
                        result = result.substr(1, result.length() - 2);
                    }

                    return "<code>" + result + "</code>";
                }
            }

            return mark;
        }

        std::string convert_html_tag()
        {
            // [REF] 6.5 Autolinks
            //
            // we first try for an autolink which is a sort of a shorthand
            // for an anchor tag; if that matches, then it's not an HTML tag
            // anyway (because a tag name cannot include ':')
            //
            ++f_it;
            auto et(f_it);
            if(f_it->is_first_protocol())
            {
                int count(1);
                for(++et; et != f_line.cend(); ++et, ++count)
                {
                    if(!et->is_protocol())
                    {
                        break;
                    }
                }
                if(et->is_colon()   // protocol & path separated by a colon
                && count >= 2       // protocol is 2-32 chars
                && count <= 32)
                {
                    for(++et; et != f_line.cend() && et->is_uri(); ++et);
                    if(et->is_close_angle_bracket())
                    {
                        // valid!
                        //
                        std::string result("<a href=\"");

                        character::string_t uri(f_it, et);
                        result += convert_uri(uri);

                        result += "\">";
                        while(f_it != et)
                        {
                            result += convert_basic_char();
                        }
                        result += "</a>";

                        ++f_it; // and finally skip the '>'

                        return result;
                    }
                }
            }

            // [REF] 6.6 Raw HTML
            //
            bool const closing(f_it->is_slash());
            et = f_it + (closing ? 1 : 0);
            if(et != f_line.cend()
            && et->is_first_tag())
            {
                for(++et; et != f_line.cend() && et->is_tag(); ++et);

                for(; et != f_line.cend() && et->is_blank(); ++et);

                if(et != f_line.cend())
                {
                    if(closing)
                    {
                        // no attributes in a closing tag, we must have the
                        // closing angle bracket, though
                        //
                        if(et->is_close_angle_bracket())
                        {
                            ++et;
                            std::string const result(
                                character::to_utf8(character::string_t(f_it - 1, et)));
                            f_it = et;
                            return result;
                        }
                    }
                    else if(verify_tag_attributes(f_line, et))
                    {
                        std::string const result(
                            character::to_utf8(character::string_t(f_it - 1, et)));
                        f_it = et;
                        return result;
                    }
                }
            }

            return "&lt;";
        }

        std::string convert_span(character previous)
        {
// A left-flanking delimiter run is a delimiter run that is (1) not
// followed by Unicode whitespace, and either (2a) not followed by
// a Unicode punctuation character, or (2b) followed by a Unicode
// punctuation character and preceded by Unicode whitespace or a
// Unicode punctuation character. For purposes of this definition,
// the beginning and the end of the line count as Unicode whitespace.
//
// right is very similar, only invert the requirements

            character mark(*f_it);

std::cerr << " >>> found some mark...\n";
            std::size_t count(1);
            for(++f_it;
                f_it != f_line.cend() && *f_it == mark;
                ++f_it, ++count);

            if(f_it == f_line.cend()
            || !f_it->is_left_flanking(previous))
            {
std::cerr << " >>> just mark...\n";
                return std::string(count, static_cast<char>(mark.f_char));
            }

std::cerr << " >>> mark could be a left flanking span...\n";
            std::string result;

            while(f_it != f_line.cend())
            {
                auto et(f_it);

                for(;;)
                {
                    if(et == f_line.cend()
                    || *et != mark)
                    {
                        break;
                    }
                    ++et;
                }
                std::size_t end_count(et - f_it);
                character const p(f_it == f_line.cbegin() ? character() : f_it[-1]);
                if(end_count > 0
                && (et == f_line.cend() || et->is_right_flanking(p)))
                {
                    if(end_count > count)
                    {
                        end_count = count;
                    }
                    count -= end_count;
                    f_it += end_count;

character::string_t remain(f_it, f_line.cend());
std::cerr << " *** remain after 'span': ["
<< character::to_utf8(remain)
<< "]\n";

                    std::string open_tag;
                    std::string close_tag;
                    switch(mark_and_count(mark.f_char, end_count))
                    {
                    case mark_and_count(CHAR_ASTERISK, 1):
                    case mark_and_count(CHAR_UNDERSCORE, 1):
                        open_tag = "<em>";
                        close_tag = "</em>";
                        break;

                    case mark_and_count(CHAR_ASTERISK, 2):
                    case mark_and_count(CHAR_UNDERSCORE, 2):
                        open_tag = "<strong>";
                        close_tag = "</strong>";
                        break;

                    case mark_and_count(CHAR_ASTERISK, 3):
                    case mark_and_count(CHAR_UNDERSCORE, 3):
                        open_tag = "<em><strong>";
                        close_tag = "</strong></em>";
                        break;

                    case mark_and_count(CHAR_DASH, 1):
                        open_tag = "<s>";
                        close_tag = "</s>";
                        break;

                    case mark_and_count(CHAR_DASH, 2):
                        open_tag = "<del>";
                        close_tag = "</del>";
                        break;

                    case mark_and_count(CHAR_DASH, 3):
                        open_tag = "<s><del>";
                        close_tag = "</del></s>";
                        break;

                    case mark_and_count(CHAR_PLUS, 1):
                        open_tag = "<mark>";
                        close_tag = "</mark>";
                        break;

                    case mark_and_count(CHAR_PLUS, 2):
                        open_tag = "<ins>";
                        close_tag = "</ins>";
                        break;

                    case mark_and_count(CHAR_PLUS, 3):
                        open_tag = "<mark><ins>";
                        close_tag = "</ins></mark>";
                        break;

                    default:
                        throw commonmark_logic_error("The switch to generate the open/close span tags did not capture the current state."); // LCOV_EXCL_IGNORE

                    }

                    result = open_tag + result + close_tag;
                    if(count == 0)
                    {
                        return result;
                    }
                }
                else
                {
                    result += convert_char();
                }
            }

            // the mark ended up not being used, so we have to output it
            //
            std::string const ignored_mark(count, static_cast<char>(mark.f_char));
            return ignored_mark + result;
        }

        std::string convert_basic_char()
        {
            std::string result;

            switch(f_it->f_char)
            {
            case CHAR_QUOTE:
                result += "&quot;";
                break;

            case CHAR_AMPERSAND:
                result += "&amp;";
                break;

            case CHAR_OPEN_ANGLE_BRACKET:
                result += "&lt;";
                break;

            case CHAR_CLOSE_ANGLE_BRACKET:
                result += "&gt;";
                break;

            default:
                result += f_it->to_utf8();
                break;

            }
            ++f_it;

            return result;
        }

        std::string convert_link(bool is_image)
        {
            std::string result;
            std::string const error_result(is_image ? "![" : "[");
            auto et(f_it);
            ++f_it;

            std::string link_text;
            if(!parse_link_text(f_line, et, link_text))
            {
                return error_result;
            }

            std::string link_destination;
            std::string link_title;
            bool valid_destination(parse_link_destination(
                          f_line
                        , et
                        , link_destination
                        , link_title));

            bool reference(false);
            bool short_reference(false);
            if(!valid_destination)
            {
std::cerr << "not a valid link...\n";
                std::string link_reference;
                parse_link_long_reference(et, link_reference);

                if(link_reference.empty())
                {
                    // try with a short link reference
                    //
                    link_reference = link_text;
                    short_reference = true;
                }

                if(!link_reference.empty())
                {
                    link::pointer_t link(f_find_link_reference(link_reference));
                    if(link == nullptr)
                    {
std::cerr << ">>> not a link... return error\n";
                        return error_result;
                    }
                    auto const & uri(link->uri_details(0));
                    link_destination = uri.destination();
                    link_title = uri.title();

                    reference = true;
                }
            }

            // it is a valid link, generate it!
            //
            f_it = et;

            result += is_image ? "<img" : "<a";

            if(f_features.get_add_classes())
            {
                std::string class_names;
                if(reference)
                {
                    if(short_reference)
                    {
                        class_names += " short";
                    }
                    class_names += " reference";
                }
                if(!class_names.empty())
                {
                    result += " class=\"";
                    result += class_names.substr(1);    // ignore first space
                    result += "\"";
                }
            }

            result += is_image ? " src=\"" : " href=\"";
            result += convert_uri(character::to_character_string(generate_attribute(
                                  character::to_character_string(link_destination)
                                , f_features.get_convert_entities())));
            result += "\"";

            if(is_image && !link_text.empty())
            {
                result += " alt=\"";
                result += generate_attribute(
                                  character::to_character_string(link_text)
                                , f_features.get_convert_entities());
                result += "\"";
            }

            if(!link_title.empty())
            {
                result += " title=\"";
                result += generate_attribute(
                                  character::to_character_string(link_title)
                                , f_features.get_convert_entities());
                result += "\"";
            }

            result += ">";

            if(!is_image)
            {
                result += link_text;
                result += "</a>";
            }

            return result;
        }

        void parse_link_long_reference(
              character::string_t::const_iterator & et
            , std::string & link_reference)
        {
            if(et == f_line.cend()
            || !et->is_open_square_bracket())
            {
                return;
            }

            std::string reference;
            int reference_length(0);
            for(++et; et != f_line.cend(); ++et)
            {
                if(et->is_close_square_bracket())
                {
                    break;
                }

                if(et->is_backslash())
                {
                    ++et;
                    if(et == f_line.cend()
                    || !et->is_ascii_punctuation())
                    {
                        --et;
                    }
                    else
                    {
                        // TBD: does the backslash count as a character here?
                        //      probably not because it eventually gets removed
                        //
                        //++reference_length;

                        reference += '\\';
                    }
                }

                ++reference_length;
                if(reference_length >= 1'000)
                {
                    // interesting rule... this is the only one
                    // parameter which I've seen limited in length
                    //
                    return;
                }

                reference += et->to_utf8();
            }

            if(et != f_line.cend())
            {
                ++et;   // skip the ']'
                link_reference = reference;
            }
        }

    private:
        character::string_t const               f_line;
        character::string_t::const_iterator     f_it;
        std::string                             f_result = std::string();
        features                                f_features = features();
        link::find_link_reference_t             f_find_link_reference = link::find_link_reference_t();
    };

    inline_parser parser(
              line
            , f_features
            , std::bind(&commonmark::find_link_reference, this, std::placeholders::_1));
    f_output += parser.run();
}


void commonmark::generate_code(block::pointer_t b)
{
    f_output += "<code";
    character::string_t info(b->info_string());
    if(!info.empty())
    {
        // WARNING: this is to match the specs, they use the first parameter
        //          as the language whether we have a "lang:" or "lang="
        //          or such is ignored
        //
        character c;
        c.f_char = CHAR_SPACE;
        std::string::size_type const pos(info.find(c));
        character::string_t language(info);
        if(pos != character::string_t::npos)
        {
            language = info.substr(0, pos);
        }
        f_output += " class=\"language-";
        f_output += generate_attribute(language, f_features.get_convert_entities());
        f_output += "\"";
    }
    f_output += ">";
    character::string_t const & line(b->content());
    auto et(line.end());
    while(et != line.begin())
    {
        --et;
        if(!et->is_eol())
        {
            ++et;
            break;
        }
    }
    auto it(line.begin());
    for(; it != et; ++it)
    {
        switch(it->f_char)
        {
        case CHAR_AMPERSAND:
            f_output += "&amp;";
            break;

        case CHAR_OPEN_ANGLE_BRACKET:
            f_output += "&lt;";
            break;

        case CHAR_CLOSE_ANGLE_BRACKET:
            f_output += "&gt;";
            break;

        default:
            f_output += it->to_utf8();
            break;

        }
    }
    if(!line.empty())
    {
        f_output += '\n';
    }
    f_output += "</code>";
}



} // namespace cm
// vim: ts=4 sw=4 et
