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
#include    <snapdev/string_replace_many.h>
#include    <snapdev/hexadecimal_string.h>


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

    // skip the '('
    //
    ++et;
    if(et == line.cend())
    {
        return false;
    }

    std::string destination;
    if(et->is_open_angle_bracket())
    {
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

    if(et->is_close_parenthesis())
    {
        ++et;
        link_destination = destination;
        link_title = title;
        return true;
    }

    return false;
}


template<class StringT>
std::string convert_ampersand(
      StringT line
    , StringT::const_iterator & it
    , bool convert_entities)
{
    std::string result;

    // skip the '&'
    //
    ++it;
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
std::cerr << "--- code = " << static_cast<int>(code) << "] -- "
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
                 && name != "gt")
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

}
// no name namespace



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


/** \brief Add a `<div ...>` tag around the whole document.
 *
 * The markdown specs do not add any tag around the whole document but it is
 * generally going to be very useful. If you call this function with true,
 * the output will include a div with a class set to just `"cm"`.
 *
 * \param[in] add  Whether to add `<div class=...>` around the whole document.
 */
void commonmark::add_document_div(bool add)
{
    f_add_document_div = add;
}


/** \brief Add some `class=...` to the HTML tags.
 *
 * Many markdown entries can be distinguished using HTML classes. For example,
 * the `<hr/>` tag can be defined with `***`, `---`, or `___`. Our parser
 * knows what the source character was and can translate that in a class
 * name which allows us to tweak the CSS if we want to.
 *
 * This flag is false by default (i.e. do not add the classes). Call this
 * function to set it to true.
 *
 * \param[in] add  Whether to add `class=...` to the HTML tags.
 */
void commonmark::add_classes(bool add)
{
    f_add_classes = add;
}


/** \brief Whether to add an extra, useless space in an empty tag.
 *
 * For some really odd reasons, people tend to add a space in an empty
 * tag such as:
 *
 * \code
 *     <br />
 *     <hr />
 * \endcode
 *
 * That space is not only utterly useless, it's a waste of RAM, disk
 * space, internet bandwidth, etc. By default, our parser does not
 * generate those spaces. To simplify comparing the results with
 * the commonmark spec., we have a flag to have that space added
 * in those tags.
 *
 * \param[in] add  Whether to add the extra, useless space to empty tags.
 */
void commonmark::add_space_in_empty_tag(bool add)
{
    f_add_space_in_empty_tag = add;
}


/** \brief Change the line feed between tags.
 *
 * By default, our implementation uses "" as the default line feed between
 * tags (i.e. no line feed).
 *
 * However, the commonmark spec uses "\\n" heavily between tags and to
 * replicate that behavior, we offer this function.
 *
 * You can set the line feed to one of the following strings:
 *
 * * "" -- empty string (this is the default)
 * * "\\n" -- one line feed character (this is what is expected by the
 *   commonmark specifications)
 * * "\\r" -- one carriage return; this is not conventional for HTML
 * data but will be accepted here
 * * "\\r\\n" -- one carriage return followed by one line feed; this is
 * again not conventional for HTML data
 *
 * The last two are unconventional. The carriage return may be used on
 * MacOS and the carriage return plus line feed may be used on MS-Windows.
 *
 * \note
 * The location of the new line feeds is defined by the commonmark spec.
 * It is not 100% conventional HTML formatting. Also we do not add any
 * indentation.
 *
 * \param[in] line_feed  The new line feed to use between tags.
 *
 * \sa get_line_feed()
 */
void commonmark::set_line_feed(std::string const & line_feed)
{
    f_line_feed = line_feed;
}


/** \brief Retrieve the current line feed.
 *
 * This function returns the line feed used to separate each item as per
 * the commonmark specs.
 *
 * In most cases, no line feed is necessary and thus by default this
 * function returns an empty string. You can use the set_line_feed()
 * function to change the line feed to something else than the default.
 *
 * \return The current line feed used between tags.
 *
 * \sa set_line_feed()
 */
std::string const & commonmark::get_line_feed() const
{
    return f_line_feed;
}


/** \brief Whether entities should be converted or not.
 *
 * The commonmark expects entities to be converted to their UTF-8 equivalent.
 * This flag allows you to turn that feature off and output the entities
 * as entities instead. So, for example, the \&copy; entity would normal
 * be converted to the copyright sign in the HTML output. If you set this
 * flag to false, then the conversion doesn't happen and the HTML will
 * include the entity as is: \&copy;.
 *
 * \param[in] convert  Whether to convert (true) or not (false) the entities.
 */
void commonmark::convert_entities(bool convert)
{
    f_convert_entities = convert;
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
    if(c.f_char == CHAR_CARRIAGE_RETURN)
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

    if(c.f_char == CHAR_LINE_FEED)
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

std::cerr << "any containers (other than line)?\n";
        if(f_working_block->is_line()
        && it == f_last_line.cend())
        {
            // we found an empty line
            //
std::cerr << "found last line (" << (f_eos ? "EOS" : "not eos?!") << ")\n";
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

        // TODO: the f_code_block flag should be set by the parse_contrainers
        //       but we need to fix the algorithm in there
        //
        if(f_code_block)
        {
            if(process_indented_code_block(it))
            {
std::cerr << " ---- got code block\n";
                append_line();
                continue;
            }
        }

        if(process_fenced_code_block(it))
        {
            append_line();
            continue;
        }

        if(process_html_blocks(it))
        {
            append_line();
            continue;
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

        if(process_thematic_break_or_setext_heading(it))
        {
std::cerr << " ---- break\n";
            append_line();
            continue;
        }

        if(process_reference_definition(it))
        {
std::cerr << " ---- link reference\n";
            continue;
        }

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

    bool const previous_line_is_indented_code_block(f_last_block->is_indented_code_block());
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
std::cerr << "   column: " << it->f_column << " indent " << list_indent << "\n";
        if(parse_blank(it))
        {
            continue;
        }

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
        if((has_empty_line || previous_line_is_indented_code_block)
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
<< "\n";
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
<< "\n";
        if(!f_working_block->is_list()
        && parse_list(it))
        {
std::cerr << "+++ found a list...\n";
            continue;
        }
std::cerr << "but after calling parse_list() we have list_indent = "
<< list_indent
<< "\n";

        if(f_last_block->parent() != nullptr
        && f_last_block->parent()->is_list()
        && has_empty_line)
        {
std::cerr << "then compare "
<< it->f_column
<< " vs list_indent + 4 = "
<< (list_indent + 4)
<< "\n";
            if(it->f_column >= list_indent + 4)
            {
std::cerr << "+++ this looks like a list item followed by this block code: "
<< it->f_column
<< " >= "
<< (list_indent + 4)
<< " blanks! (working column: "
<< f_working_block->end_column()
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
<< ")\n";

                f_list_subblock = list_indent;
            }
        }

std::cerr << "+++ not a list or blockquote?\n";
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
        for(; et->is_digit(); ++et)
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
    else if(!et->is_unordered_list_bullet()
         || is_thematic_break(et))
    {
std::cerr << " >> not a list mark...\n";
        return false;
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
        // TODO: maybe we could have that '\n' in the existing block?
        //
        // with blocks of code, we actually keep empty lines (we'll trim
        // the list later if necessary)
        //
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        character code_block{
            .f_char = BLOCK_TYPE_CODE_BLOCK_INDENTED,
            .f_line = f_line,   // TBD: probably f_line - 1?
            .f_column = 1,
        };
        block::pointer_t b(std::make_shared<block>(code_block));
        character c{
            .f_char = '\n',
            .f_line = f_line,   // TBD: probably f_line - 1?
            .f_column = 1,
        };
#pragma GCC diagnostic pop
        b->append(c);    // the empty line is represented by a '\n' in a `block` object
        f_last_block->link_sibling(b);
    }
}


bool commonmark::process_paragraph(character::string_t::const_iterator & it)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    character const paragraph{
        .f_char = BLOCK_TYPE_PARAGRAPH,
        .f_line = it->f_line,
        .f_column = it->f_column,
    };

    block::pointer_t b(std::make_shared<block>(paragraph));
    b->append(character::string_t(it, f_last_line.cend()));
    f_working_block->link_child(b);
    f_working_block = b;

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
    else if((f_last_block->is_indented_code_block() && b->is_indented_code_block())
         || (f_last_block->is_paragraph() && b->is_paragraph()))
    {
std::cerr << "+++ append or what?\n";

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

    // get the first character, we can't have a mix, so we need to make sure
    // the other ones we find on the line are the same
    //
    character const c(*it);
    if(!c.is_thematic_break())
    {
        return false;
    }

    bool internal_spaces(false);
    int count(1);
    for(++it; it != f_last_line.cend(); ++it, ++count)
    {
        if(it->is_blank())
        {
            internal_spaces = true;
        }
        else if(*it != c)
        {
            return false;
        }
    }

    if(c.is_setext()
    && !internal_spaces
    && f_last_block != nullptr
    && f_last_block->is_paragraph())
    {
        // [REF] 4.3 Setext headings
        //
        block::pointer_t b(std::make_shared<block>(c));
        b->number(c.is_dash() ? 2 : 1);
        b->append(f_working_block->content());
        block::pointer_t q(f_working_block->unlink());
        q->link_sibling(b);
        f_working_block = b;

        return true;
    }

    if(count < 3)
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

    int count(0);
    for(; it != f_last_line.end(); ++it, ++count)
    {
        if(!it->is_hash())
        {
            break;
        }
    }

    if(!it->is_blank())
    {
        return false;
    }

    if(count < 1
    || count > 6)
    {
        return false;
    }

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
        if(et->is_hash())
        {
            for(; et != it; --et)
            {
                if(!et->is_hash())
                {
                    if(et->is_blank())
                    {
                        c.f_char = BLOCK_TYPE_HEADER_ENCLOSED;
                    }
                    else
                    {
                        ++et;
                    }
                    break;
                }
            }
        }
        else
        {
            ++et;
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

std::cerr << "+++ adding a CODE BLOCK here\n";
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

    // this is a fenced code block
    //
    character::string_t const info_string(et, f_last_line.cend());

    character code_block(*it);  // ~ or `
    block::pointer_t b(std::make_shared<block>(code_block));
    b->info_string(info_string);

    for(;;)
    {
        get_line();

        // end of file found?
        //
        // the spec. clearly say we do not want to back out of this one
        //
        if(f_last_line.empty()
        && f_eos)
        {
            break;
        }

        // end marker?
        //
        if(f_last_line.length() >= count)
        {
            std::string::size_type end_count(1);
            for(auto ec(f_last_line.cbegin());
                ec != f_last_line.cend()
                    && ec->is_fenced_code_block()
                    && end_count < count;
                ++ec, ++end_count);

            if(end_count >= count)
            {
                break;
            }
        }

        b->append(f_last_line);

        character c(*f_last_line.crbegin());
        c.f_char = CHAR_LINE_FEED;
        b->append(c);
    }

    f_working_block->link_child(b);
    f_working_block = b;

    return true;
}


bool commonmark::process_html_blocks(character::string_t::const_iterator & it)
{
    // [REF] 4.6 HTML blocks
    //
    if(!it->is_open_angle_bracket())
    {
        return false;
    }

    bool closing(false);
    auto et(it);
    ++et;
    if(et->is_exclamation_mark())
    {
        ++et;
        if(et->is_dash())
        {
            ++et;
            if(et->is_dash())
            {
                // comment, ends on a '-->'
                //
throw std::runtime_error("comment not implemented yet");
                return false;
            }
            --et;
        }
        if(et->is_open_square_bracket())
        {
            ++et;
            if(et->f_char != 'C')
            {
                return false;
            }
            ++et;
            if(et->f_char != 'D')
            {
                return false;
            }
            ++et;
            if(et->f_char != 'A')
            {
                return false;
            }
            ++et;
            if(et->f_char != 'T')
            {
                return false;
            }
            ++et;
            if(et->f_char != 'A')
            {
                return false;
            }
            ++et;
            if(!et->is_open_square_bracket())
            {
                return false;
            }

            // <![CDATA[ ... ]]>
            //
throw std::runtime_error("CDATA not implemented yet");
            return false;
        }
        if(et->is_ascii_letter())
        {
            // identity, ends on a '>'
            //
throw std::runtime_error("identity not implemented yet");
            return false;
        }
        return false;
    }

    if(et->is_question_mark())
    {
        // <? ... ?>
        //
        ++et;

throw std::runtime_error("processing instruction not implemented yet");
        return false;
    }

    if(et->is_slash())
    {
        closing = true;
        ++et;
    }

    if(!et->is_first_tag())
    {
        return false;
    }

    // tag
    //
    std::string tag;
    tag += static_cast<char>(et->f_char);
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
        else if(tag == "pre")
        {
            end_with_empty_line = false;
        }
        break;

    case 's':
        if(tag == "section"
        || tag == "source"
        || tag == "summary")
        {
            complete_tag = false;
        }
        else if(tag == "script"
             || tag == "style")
        {
            end_with_empty_line = false;
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
        else if(tag == "textarea")
        {
            end_with_empty_line = false;
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

    character tag_block(*it);  // <
    block::pointer_t b(std::make_shared<block>(tag_block));

std::cerr << " ---- append tag intro to tag_block...\n";
    b->append(character::string_t(it, et));

    // remove start of tag
    //
std::cerr << " ---- restart line...\n";
    f_last_line = character::string_t(et, f_last_line.cend());

    // user tags need to be "well defined" (as per commonmark)
    //
    if(!closed
    && complete_tag)
    {
std::cerr << " ---- tag needs to be complete...\n";
        // WARNING: at this point, we can't back out of an invalid multi-line
        //          tag block definition
        //
        // states:
        //
        //   * 0 -- attribute name or ">" or "/"
        //   * 1 -- attribute name or "=" or ">" or "/"
        //   * 2 -- attribute value
        //   * 3 -- ">"
        //
        int state(0);
        et = f_last_line.cbegin();
        for(;;)
        {
std::cerr << " ---- tag innards state: " << state << "...\n";
            if(et == f_last_line.cend())
            {
std::cerr << " ---- append whole line...\n";
                b->append(f_last_line);
                get_line();
                et = f_last_line.cbegin();
            }

            if(state == 3)
            {
                if(et->is_close_angle_bracket())
                {
                    // tag is considered valid
                    //
std::cerr << " ---- tag innards got '>'\n";
                    break;
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

            if(state == 0 || state == 1)
            {
                if(et->is_first_attribute())
                {
                    for(++et; et != f_last_line.cend(); ++et)
                    {
                        if(!et->is_attribute())
                        {
                            break;
                        }
                    }
std::cerr << " ---- tag innards found attribute name\n";
                    state = 1;
                    continue;
                }
                else if(et->is_slash())
                {
std::cerr << " ---- tag innards found slash\n";
                    ++et;
                    state = 3;
                    continue;
                }
                else if(et->is_close_angle_bracket())
                {
std::cerr << " ---- tag innards found '>', valid!\n";
                    break;
                }
            }

            if(state == 1
            && et->is_equal())
            {
std::cerr << " ---- tag innards got '=' before value\n";
                state = 2;
                ++et;
                continue;
            }

            if(state == 2)
            {
                if(et->is_quote()
                || et->is_apostrophe())
                {
std::cerr << " ---- tag innards got ' or \" around value\n";
                    // TBD: can these spill on multiple lines?
                    char32_t quote(et->f_char);
                    for(++et;
                        et != f_last_line.cend()
                            && et->f_char != quote;
                        ++et);
                    if(et == f_last_line.cend())
                    {
                        // TODO: fix input if multi-line
                        //
std::cerr << " ---- tag innards ' or \" not closed?\n";
                        return false;
                    }
                    ++et;
                }
                else
                {
                    for(; et->is_attribute_standalone_value(); ++et);
                    if(et != f_last_line.cend()
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

                state = 0;
                continue;
            }

            // state / current character mismatch, we've got an error
            //
            // TODO: fix input if multi-line
            //
            return false;
        }
    }

std::cerr << " ---- build tag block now\n";
    for(;;)
    {
        if(end_with_empty_line)
        {
            if(f_last_line.empty())
            {
                // finished with this tag block
                //
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

            bool found(false);
            for(auto ci(f_last_line.cbegin());
                ci != f_last_line.cend() && !found;
                ++ci)
            {
                while(ci->is_open_angle_bracket())
                {
                    ++ci;
                    if(ci->is_slash())
                    {
                        std::string closing_tag;
                        for(++ci; ci != f_last_line.cend(); ++ci)
                        {
                            if(!ci->is_ascii_letter())
                            {
                                while(ci->is_blank())
                                {
                                    ++ci;
                                }
                                break;
                            }
                            closing_tag += static_cast<char>(ci->f_char) | 0x20; // force lowercase
                        }
                        found = ci->is_close_angle_bracket()
                                && (closing_tag == "pre"
                                 || closing_tag == "script"
                                 || closing_tag == "style"
                                 || closing_tag == "textarea");
                        if(found)
                        {
                            ++ci;   // the '>'
                            b->append(character::string_t(f_last_line.cbegin(), ci));

                            // TODO: re-inject end of line? (ci + 1, f_last_line.cend())
                            //
                            break;
                        }
                    }
                }
            }
        }
std::cerr << " ---- append whole line before closing tag or empty line...\n";
        b->append(f_last_line);
        get_line();
    }

    f_working_block->link_child(b);
    f_working_block = b;

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
            if(f_add_document_div)
            {
                f_output += "<div class=\"cm-document\">";
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
        case BLOCK_TYPE_HEADER_UNDERLINED:
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
            f_output += '\n';
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


void commonmark::generate_list(block::pointer_t b)
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

    if(f_add_classes)
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
                f_output += f_line_feed;
                generate(b->first_child()->next());
            }
        }
        else
        {
            f_output += f_line_feed;
            generate(b->first_child());
        }
        f_output += "</li>";
        f_output += f_line_feed;

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
    if(f_add_classes)
    {
        switch(b->type().f_char)
        {
        case BLOCK_TYPE_HEADER_OPEN:
            f_output += " class=\"cm-open\"";
            break;

        case BLOCK_TYPE_HEADER_ENCLOSED:
            f_output += " class=\"cm-enclosed\"";
            break;

        case BLOCK_TYPE_HEADER_UNDERLINED:
            f_output += " class=\"cm-underline cm-underscore\"";
            break;

        case BLOCK_TYPE_HEADER_DOUBLE:
            f_output += " class=\"cm-underline cm-equal\"";
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
    f_output += f_line_feed;
}


void commonmark::generate_thematic_break(block::pointer_t b)
{
    f_output += "<hr";

    if(f_add_classes)
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

    if(f_add_space_in_empty_tag)
    {
        f_output += ' ';
    }
    f_output += "/>";
    f_output += f_line_feed;
}


void commonmark::generate_inline(character::string_t const & line)
{
std::cerr << " ---- inline to parse: [" << line << "]\n";

    class inline_parser
    {
    public:
        inline_parser(
                  character::string_t const & line
                , bool add_space_in_empty_tag
                , bool convert_entities
                , bool add_classes
                , link::find_link_reference_t find_link_reference)
            : f_line(line)
            , f_it(line.cbegin())
            , f_add_space_in_empty_tag(add_space_in_empty_tag)
            , f_convert_entities(convert_entities)
            , f_add_classes(add_classes)
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

            switch(f_it->f_char)
            {
            case CHAR_SPACE:
            case CHAR_TAB:
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
                        if(f_add_space_in_empty_tag)
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
                result += convert_ampersand(f_line, f_it, f_convert_entities);
                break;

            case CHAR_GRAVE: // inline code
                result += convert_inline_code();
                break;

            case CHAR_OPEN_ANGLE_BRACKET: // inline HTML tag (or not)
                result += convert_html_tag();
                break;

            case CHAR_ASTERISK: // italic (1) or bold (2)
                ++f_it;
                if(f_it != f_line.cend()
                && f_it->is_asterisk())
                {
                    result += convert_span("**", "<b>", "</b>");
                }
                else
                {
                    --f_it;
                    result += convert_span("*", "<em>", "</em>");
                }
                break;

            case CHAR_UNDERSCORE: // italic
                result += convert_span("_", "<em>", "</em>");
                break;

            case CHAR_DASH: // strikethrough
                result += convert_span("-", "<s>", "</s>");
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
                    if(f_add_space_in_empty_tag)
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
                            result += ' ';
                            break;

                        case CHAR_TAB:
                            result += '\t';
                            break;

                        case CHAR_LINE_FEED:
                            result += '\n';
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
            ++f_it;
            if(f_it->is_first_protocol())
            {
                int count(1);
                auto et(f_it);
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

                        std::string uri;
                        auto it(f_it);
                        for(; it != et; ++it)
                        {
                            uri += it->to_utf8();
                        }

                        for(auto c(uri.begin()); c != uri.end(); ++c)
                        {
                            switch(*c)
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
                                result += snap::int_to_hex(*c, true);
                                break;

                            case '%':       // 25
                                // if already followed by 2 hexdigits, do
                                // nothing, otherwise replace the % with %25
                                //
                                ++c;
                                if(c != uri.end()
                                && snap::is_hexdigit(*c))
                                {
                                    ++c;
                                    if(c != uri.end()
                                    && snap::is_hexdigit(*c))
                                    {
                                        result += '%';
                                        result += static_cast<char>(c[-1]);
                                        result += static_cast<char>(*c);
                                        break;
                                    }
                                    --c;
                                }
                                --c;
                                result += "%25";
                                break;

                            default:
                                if(static_cast<unsigned  char>(*c) >= 128)
                                {
                                    // upper UTF-8 codes are all encoded
                                    //
                                    result += snap::int_to_hex(*c, true);
                                }
                                else
                                {
                                    result += *c;
                                }
                                break;

                            }
                        }

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

            return "&lt;";
        }

        std::string convert_span(
              std::string const & mark
            , std::string const & open_tag
            , std::string const & close_tag)
        {
            std::string result;

            ++f_it;
            while(f_it != f_line.cend())
            {
                auto et(f_it);

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
                    f_it = et;
                    return open_tag + result + close_tag;
                }

                result += convert_char();
            }

            // the mark ended up not being used, so we have to output it
            //
            return mark + result;
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

            if(f_add_classes)
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
            result += link_destination;
            result += "\"";

            if(is_image && !link_text.empty())
            {
                result += " alt=\"";
                result += link_text;
                result += "\"";
            }

            if(!link_title.empty())
            {
                result += " title=\"";
                result += link_title;
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
        character::string_t const &             f_line;
        character::string_t::const_iterator     f_it;
        std::string                             f_result = std::string();
        bool                                    f_add_space_in_empty_tag = false;
        bool                                    f_convert_entities = false;
        bool                                    f_add_classes = false;
        link::find_link_reference_t             f_find_link_reference = link::find_link_reference_t();
    };

    inline_parser parser(
              line
            , f_add_space_in_empty_tag
            , f_convert_entities
            , f_add_classes
            , std::bind(&commonmark::find_link_reference, this, std::placeholders::_1));
    f_output += parser.run();
}


std::string commonmark::generate_attribute(std::string const & line)
{
    std::string result;
    for(auto it(line.cbegin());
        it != line.cend();
        ++it)
    {
        switch(*it)
        {
        case '\\':
            ++it;
            if(it == line.cend())
            {
                --it;
                result += '\\';
            }
            else if(static_cast<unsigned char>(*it) < 0x80)
            {
                character c;
                c.f_char = *it;
                result += c.to_utf8();
            }
            break;

        case '<':
            result += "&lt;";
            break;

        case '>':
            result += "&gt;";
            break;

        case '"':
            result += "&quot;";
            break;

        case '&':
            result += convert_ampersand(
                          line
                        , it
                        , f_convert_entities);
            break;

        default:
            result += *it;
            break;

        }
    }
    return result;
}


void commonmark::generate_code(block::pointer_t b)
{
    character::string_t const & line(b->content());
    f_output += "<code";
    std::string info(b->info_string());
    if(!info.empty())
    {
        // WARNING: this is to match the specs, they use the first parameter
        //          as the language whether we have a "lang:" or "lang="
        //          or such is ignored
        //
        std::string::size_type const pos(info.find(' '));
        std::string language(info);
        if(pos != std::string::npos)
        {
            language = info.substr(0, pos);
        }
        f_output += " class=\"language-";
        f_output += generate_attribute(language);
        f_output += "\"";
    }
    f_output += ">";
    auto it(line.begin());
    for(; it != line.end(); ++it)
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
    f_output += "</code>";
}



} // namespace cm
// vim: ts=4 sw=4 et
