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
#include    "commonmarkcpp/character.h"


// libutf8 lib
//


// C++ lib
//
#include    <memory>
#include    <string>



namespace cm
{





class commonmark
{
public:
    typedef std::shared_ptr<commonmark>
                            pointer_t;

                            commonmark();

    void                    add_classes(bool add = true);
    void                    add_input(std::string const & input);
    void                    process();
    void                    flush();
    std::string             get_output();

private:
    enum block_type_t
    {
        BLOCK_TYPE_PARAGRAPH,
        BLOCK_TYPE_CODE_BLOCK,
    };

    character               getc();
    character::string_t     get_line();

    void                    block();
    static bool             is_empty(character::string_t const & str);
    bool                    process_thematic_break_or_setext_heading(character::string_t const & line);
    bool                    process_header(character::string_t const & line);
    bool                    process_indented_code_block(character::string_t const & line);
    void                    process_inline(character::string_t const & line);
    void                    process_code(character::string_t const & line);

    std::string             f_input = std::string();
    libutf8::utf8_iterator  f_iterator;  // must be defined in constructors
    //int                     f_unget_pos = 0;
    //character_t             f_unget[2] = {};
    bool                    f_eos = false;
    std::uint32_t           f_line = 1;
    std::uint32_t           f_column = 1;
    //character::string_t     f_last_line = character::string_t();
    std::uint32_t           f_indentation = 0;  // i.e. within a list, we get indented paragraphs
    block_type_t            f_block_type = BLOCK_TYPE_PARAGRAPH;
    character::string_t     f_block = character::string_t();

    bool                    f_add_classes = false;
    bool                    f_flushed = false;

    std::string             f_output = std::string();
};



} // namespace cm
// vim: ts=4 sw=4 et
