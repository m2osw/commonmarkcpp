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

#include    "commonmarkcpp/exception.h"


// C++ lib
//
#include    <algorithm>
#include    <cstring>
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace cm
{




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


/** \brief Add some input to the parser.
 *
 * This function can be used to add input data to the CommonMark parser.
 * The output can be retrieved with the get_output() function. In some
 * cases, the output is not immediately available. To flush the input,
 * make sure to call the flush() function once before the get_output().
 *
 * \param[in] input  The UTF-8 input string to convert to HTML.
 */
void commonmark::add_input(std::string const & input)
{
    // don't waste any time if the input string is empty
    //
    if(input.empty())
    {
        return;
    }

    if(f_flushed)
    {
        throw already_flushed("you cannot add more input after calling flush()");
    }

    f_input += input;
}


/** \brief Process the existing data.
 *
 * After an add_input(), call this function to process the data.
 */
void commonmark::process()
{
    block();
}


/** \brief Flush the stream.
 *
 * This function forces all remaining input to be passed out as required.
 * Once you are done adding input, you can call this function.
 */
void commonmark::flush()
{
    if(f_block.empty())
    {
        return;
    }

    switch(f_block_type)
    {
    case BLOCK_TYPE_PARAGRAPH:
        f_output += "<p>";
        process_inline(f_block);
        f_output += "</p>";
        break;

    case BLOCK_TYPE_CODE_BLOCK:
        f_output += "<pre>";
        process_code(f_block);
        f_output += "</pre>";
        break;

    }
}


/** \brief Retrieve the currently available output.
 *
 * This function returns the currently available output. As you call the
 * add_input() function, output becomes available. Output which gets sent
 * to you via this function.
 *
 * The function clears the output buffer on each call.
 *
 * \return A string with the currently available output.
 */
std::string commonmark::get_output()
{
    std::string const result(f_output);

    f_output.clear();

    return result;
}


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
        .f_flags = 0,
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

        // move to the next line for next getc()
        //
        ++f_line;
        f_column = 1;
    }

    return c;   // returns libutf8::EOS at the end of the input
}


character::string_t commonmark::get_line()
{
    character::string_t line;
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
        line += c;
    }

    return line;
}


void commonmark::block()
{
    f_eos = false;
    for(;;)
    {
        character::string_t line(get_line());

        if(line.empty()     // not is_empty() here!
        && f_eos)
        {
            return;
        }

        if(process_thematic_break_or_setext_heading(line))
        {
            continue;
        }

        if(process_header(line))
        {
            continue;
        }

        if(process_indented_code_block(line))
        {
            continue;
        }

std::cerr << "--- NOT PROCESSED?!?\n";
    }
}


/** \brief Check whether the line is considered empty.
 *
 * Empty lines have special effects on some paragraphs and blocks.
 * For example, they close a standard paragraph.
 *
 * An empty line is a string composed exclusively of spaces and tabs
 * or no characters at all.
 *
 * \return true if the line is considered empty.
 */
bool commonmark::is_empty(character::string_t const & str)
{
    for(auto c : str)
    {
        if(!c.is_space()
        || !c.is_tab())
        {
            return false;
        }
    }

    return true;
}


bool commonmark::process_thematic_break_or_setext_heading(character::string_t const & line)
{
    // [REF] 4.1 Thematic breaks
    //
    auto it(line.begin());

    for(int i(0); i < 3; ++i, ++it)
    {
        if(it == line.end()
        || it->f_column >= f_indentation * 4 + 5)
        {
            return false;
        }
        if(!it->is_space())
        {
            break;
        }
    }

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
    for(++it; it != line.end(); ++it, ++count)
    {
        if(it->is_space()
        || it->is_tab())
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
    && !f_block.empty()
    && f_block_type == BLOCK_TYPE_PARAGRAPH)
    {
        // [REF] 4.3 Setext headings
        //
        int const h(c.is_dash() ? 2 : 1);

        f_output += "<h";
        f_output += std::to_string(h);
        if(f_add_classes)
        {
            f_output += " class=\"cm-underlined\"";
        }
        f_output += ">";

        // note: an empty title is considered valid by commonmark
        //
        process_inline(f_block);

        f_output += "</h";
        f_output += std::to_string(h);
        f_output += ">";

        return true;
    }

    if(count < 3)
    {
        return false;
    }

    flush();

    f_output += "<hr";

    if(f_add_classes)
    {
        f_output += " class=\"";

        switch(c.f_char)
        {
        case U'-':
            f_output += "cm-break-dash";
            break;

        case U'_':
            f_output += "cm-break-underline";
            break;

        case U'*':
            f_output += "cm-break-asterisk";
            break;

        }
        f_output += '"';
    }

    f_output += "/>";

    return true;
}


bool commonmark::process_header(character::string_t const & line)
{
    auto it(line.begin());

    for(int i(0); i < 3; ++i, ++it)
    {
        if(it == line.end())
        {
            return false;
        }
        if(!it->is_space())
        {
            break;
        }
    }

    int count(0);
    for(; it != line.end(); ++it, ++count)
    {
        if(!it->is_hash())
        {
            break;
        }
    }

    if(!it->is_space()
    && !it->is_tab())
    {
        return false;
    }

    if(count < 1
    || count > 6)
    {
        return false;
    }

    for(; it != line.end(); ++it)
    {
        if(!it->is_space()
        && !it->is_tab())
        {
            break;
        }
    }

    // this looks like a header, parse the end
    // and also remove spaces and #'s from there
    //
    auto et(line.end());
    while(et != it)
    {
        --et;
        if(!et->is_space()
        && !et->is_tab())
        {
            break;
        }
    }
    bool has_end_hash(false);
    for(; et != it; --et)
    {
        if(!et->is_hash())
        {
            if(et->is_space()
            || et->is_tab())
            {
                has_end_hash = true;
            }
            else
            {
                ++et;
            }
            break;
        }
    }

    flush();

    f_output += "<h";
    f_output += std::to_string(count);
    if(f_add_classes)
    {
        if(has_end_hash)
        {
            f_output += " class=\"cm-enclosed\"";
        }
        else
        {
            f_output += " class=\"cm-open\"";
        }
    }
    f_output += ">";

    // note: an empty title is considered valid by commonmark
    //
    process_inline(character::string_t(it, et));

    f_output += "</h";
    f_output += std::to_string(count);
    f_output += ">";

    return true;
}


bool commonmark::process_indented_code_block(character::string_t const & line)
{
    // [REF] 4.4 Indented code blocks
    //
    auto it(line.begin());

    do
    {
        if(it == line.end()
        || (!it->is_space()
            && !it->is_tab()))
        {
            return false;
        }
        ++it;
    }
    while(it->f_column < f_indentation * 4 + 1);

    if(f_block_type != BLOCK_TYPE_PARAGRAPH)
    {
        flush();
    }

    f_block_type = BLOCK_TYPE_CODE_BLOCK;

    f_block += character::string_t(it, line.end());

    return true;
}


void commonmark::process_inline(character::string_t const & line)
{
    auto it(line.begin());
    for(; it != line.end(); ++it)
    {
        // TODO
        switch(it->f_char)
        {
        case CHAR_AMPERSAND:
            break;

        case CHAR_BACKSLASH:
            ++it;
            if(it != line.end()
            && it->is_ascii_punctuation())
            {
                f_output += static_cast<char>(it->f_char);
            }
            else
            {
                f_output += '\\';
                --it;
            }
            break;

        default:
            f_output += it->to_utf8();
            break;

        }
    }
}


void commonmark::process_code(character::string_t const & line)
{
    f_output += "<code>";
    auto it(line.begin());
    for(; it != line.end(); ++it)
    {
        switch(it->f_char)
        {
        case CHAR_AMPERSAND:
            f_output += "&amp;";
            break;

        case CHAR_LESS:
            f_output += "&lt;";
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
