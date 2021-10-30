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

        // TBD: can we have a header inside a list?
        //      the specs asks the question but no direct answer...
        {
            // header introducer
            //
            if(process_header(it))
            {
std::cerr << " ---- header\n";
                append_line();
                continue;
            }
        }

        if(process_thematic_break_or_setext_heading(it))
        {
std::cerr << " ---- break\n";
            append_line();
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

    int ident(-1);
    if(f_last_block->is_list())
    {
        ident = f_working_block->end_column() + 1;
    }

std::cerr << "+++ checking for containers...\n";
    f_code_block = false;
    while(it != f_last_line.cend())
    {
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
//        if(f_current_gap >= 4)
//        {
//            // TODO: write code to compute the indentation constraints
//            //
//            //if(f_working_block->is_line())
//            //{
//            //}
//std::cerr << "+++ got a gap of 4+ blanks! (working column: "
//<< f_working_block->end_column()
//<< ", it column "
//<< it->f_column
//<< ", diff: "
//<< (f_working_block->end_column() - it->f_column)
//<< ")\n";
//
//            f_code_block = true;
//            break;
//        }

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
<< "\n";
        if(!f_working_block->is_list()
        && parse_list(it))
        {
std::cerr << "+++ found a list...\n";
            continue;
        }

        if(f_working_block == f_top_working_block)
        {
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
    if(!it->is_greater())
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
    f_last_block->followed_by_an_empty_line(true);
    if(f_last_block->is_code_block())
    {
        // with blocks of code, we actually keep empty lines (we'll trim
        // the list later if necessary)
        //
        character code_block;
        code_block.f_char = BLOCK_TYPE_CODE_BLOCK;
        block::pointer_t b(std::make_shared<block>(code_block));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
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
    character paragraph;
    paragraph.f_char = BLOCK_TYPE_PARAGRAPH;
    paragraph.f_line = it->f_line;
    paragraph.f_column = it->f_column;

    block::pointer_t b(std::make_shared<block>(paragraph));
    b->append(character::string_t(it, f_last_line.cend()));
    f_working_block->link_child(b);

    return true;
}


void commonmark::append_line()
{
    block::pointer_t b(f_top_working_block->first_child());
    if(b == nullptr)
    {
        throw commonmark_logic_error("append_line() called with an empty line.");
    }

    if((f_last_block->is_code_block() && b->is_code_block())
    || (f_last_block->is_paragraph()  && b->is_paragraph()))
    {
std::cerr << "+++ append or what?\n";
        f_last_block->append(b->content());
    }
    else
    {
std::cerr << "+++ link new? " << (reinterpret_cast<void *>(b.get())) << "\n";
        b->unlink();
        f_document->link_child(b);
        f_last_block = f_working_block;
std::cerr << "+++ doc first child " << (reinterpret_cast<void *>(f_document->first_child().get())) << "\n";
std::cerr << "+++ doc first child next sibling " << (reinterpret_cast<void *>(f_document->first_child()->next().get())) << "\n";
std::cerr << "+++ doc last child " << (reinterpret_cast<void *>(f_document->last_child().get())) << "\n";
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
    code_block.f_char = BLOCK_TYPE_CODE_BLOCK;
    block::pointer_t b(std::make_shared<block>(code_block));
    b->append(character::string_t(it, f_last_line.cend()));

    character c(*f_last_line.crbegin());
    c.f_char = '\n';
    b->append(c);

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
        if(b->is_document())
        {
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
        }
        else if(b->is_paragraph())
        {
            f_output += "<p>";
            generate_inline(b->content());
            f_output += "</p>\n";
        }
        else if(b->is_code_block())
        {
            f_output += "<pre>";
            generate_code(b->content());
            f_output += "</pre>\n";
        }
        else if(b->is_list())
        {
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
            for(;; b = b->next())
            {
                f_output += "<li>\n";
                generate(b->first_child());
                f_output += "</li>\n";

                if(b->next() == nullptr)
                {
                    break;
                }

                if(!b->next()->is_list())
                {
                    break;
                }
            }
            f_output += "</";
            f_output += tag;
            f_output += ">\n";
        }
        else if(b->is_header())
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
        }
        else if(b->is_thematic_break())
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

            f_output += "/>";
        }
        else
        {
            throw commonmark_logic_error(
                      "unrecognized block type ("
                    + std::to_string(static_cast<int>(b->type().f_char))
                    + ") while generate HTML data"
                );
        }
    }
}


void commonmark::generate_inline(character::string_t const & line)
{
    auto it(line.begin());
    for(; it != line.end(); ++it)
    {
        switch(it->f_char)
        {
        case CHAR_QUOTE:
            f_output += "&quote;";
            break;

        case CHAR_AMPERSAND:
            {
                bool valid(false);
                std::string name;
                ++it;
                if(it->is_hash())
                {
                    // expected a decimal or hexadecimal number
                    //
                    name += '#';
                    char32_t code(0);
                    ++it;
                    if(it->f_char == 'x'
                    || it->f_char == 'X')
                    {
                        name += static_cast<char>(it->f_char);
                        for(++it; it->is_hexdigit(); ++it)
                        {
                            name += static_cast<char>(it->f_char);
                            code *= 16;
                            code += it->hexdigit_number() - '0';
                            if(code >= 0x110000)
                            {
                                break;
                            }
                        }
                    }
                    else
                    {
                        for(; it->is_digit(); ++it)
                        {
                            name += static_cast<char>(it->f_char);
                            code *= 10;
                            code += it->digit_number();
                            if(code >= 0x110000)
                            {
                                break;
                            }
                        }
                    }
                    if(it->is_semicolon())
                    {
                        valid = true;

                        if(f_convert_entities)
                        {
                            f_output += libutf8::to_u8string(code);
                        }
                        else
                        {
                            std::stringstream sd;
                            sd << static_cast<int>(code);
                            std::stringstream sh;
                            sh << std::hex << static_cast<int>(code);
                            f_output += '&';
                            f_output += '#';
                            if(sd.str().length() > sh.str().length())
                            {
                                f_output += sh.str();
                            }
                            else
                            {
                                f_output += sd.str();
                            }
                            f_output += ';';
                        }
                    }
                }
                else
                {
                    // expect a name
                    //
                    for(++it; it != line.end(); ++it)
                    {
                        if(it->f_char == ';')
                        {
                            valid = true;
                            break;
                        }
                        if(it->f_char >= '0' && it->f_char <= '9')
                        {
                            if(name.empty())
                            {
                                // needs to start with a letter
                                break;
                            }
                            name += static_cast<char>(it->f_char);
                        }
                        else if((it->f_char >= 'a' && it->f_char <= 'z')
                             || (it->f_char >= 'A' && it->f_char <= 'Z'))
                        {
                            name += static_cast<char>(it->f_char);
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
                        if(entity == end)
                        {
                            valid = false;
                        }
                        else if(f_convert_entities)
                        {
                            // f_codes is already a UTF-8 string
                            //
                            f_output += entity->f_codes;
                        }
                        else
                        {
                            // this is a valid entity, keep it as is
                            //
                            f_output += '&';
                            f_output += name;
                            f_output += ';';
                        }
                    }
                }
                if(!valid)
                {
                    // what looks like an entity but is not is added as is
                    //
                    f_output += "&amp;";
                    f_output += name;
                }
            }
            break;

        case CHAR_LESS:
            f_output += "&lt;";
            break;

        case CHAR_GREATER:
            f_output += "&gt;";
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


void commonmark::generate_code(character::string_t const & line)
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

        case CHAR_GREATER:
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
