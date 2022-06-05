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
#include    "commonmarkcpp/block.h"
#include    "commonmarkcpp/features.h"
#include    "commonmarkcpp/link.h"


// libutf8 lib
//


// C++ lib
//
#include    <memory>
#include    <string>
#include    <vector>



namespace cm
{



enum class indentation_t
{
    INDENTATION_PARAGRAPH,
    INDENTATION_CODE_BLOCK,
    INDENTATION_CONTINUATION,
};



class commonmark
{
public:
    typedef std::shared_ptr<commonmark>
                            pointer_t;

                            commonmark();

    void                    set_features(features const & features);
    features const &        get_features() const;

    std::string             process(std::string const & input);

    void                    add_link(
                                  std::string const & name
                                , std::string const & destination
                                , std::string const & title
                                , bool reference);
    link::pointer_t         find_link_reference(std::string const & name);

private:
    struct input_status_t
    {
        //typedef std::vector<input_status_t>     vector_t;

        libutf8::utf8_iterator  f_iterator;
        std::uint32_t           f_line = 1;
        std::uint32_t           f_column = 1;
        character::string_t     f_last_line = character::string_t();
    };

    character               getc();
    void                    get_line();
    input_status_t          get_current_status();
    void                    restore_status(input_status_t const & status);
    //static bool             is_empty(character::string_t const & str);

    void                    parse();
    character::string_t::const_iterator
                            parse_containers();
    bool                    parse_blank(character::string_t::const_iterator & it);
    bool                    parse_blockquote(character::string_t::const_iterator & it);
    bool                    parse_list(character::string_t::const_iterator & it);
    bool                    is_thematic_break(character::string_t::const_iterator it);

    void                    process_empty_line(bool blockquote_followed_by_empty);
    void                    append_line();
    void                    append_list_as_text(block::pointer_t dst_list_item, block::pointer_t src_list);

    bool                    process_paragraph(character::string_t::const_iterator & it);
    int                     process_thematic_break_or_setext_heading(character::string_t::const_iterator & it);
    bool                    process_reference_definition(character::string_t::const_iterator & it);
    bool                    parse_reference_destination(
                                  character::string_t::const_iterator & et
                                , std::string & link_destination
                                , std::string & link_title);
    bool                    parse_reference_title(
                                  character::string_t::const_iterator & et
                                , std::string & title);
    bool                    process_header(character::string_t::const_iterator & it);
    bool                    process_indented_code_block(character::string_t::const_iterator & it);
    bool                    process_fenced_code_block(character::string_t::const_iterator & it);
    bool                    process_html_blocks(character::string_t::const_iterator & it);

    void                    generate(block::pointer_t b);
    void                    generate_list(block::pointer_t & b);
    void                    generate_header(block::pointer_t b);
    std::string             to_identifier(character::string_t const & line);
    void                    generate_thematic_break(block::pointer_t b);
    void                    generate_inline(character::string_t const & line);
    void                    generate_code(block::pointer_t b);

    std::string             f_input = std::string();
    libutf8::utf8_iterator  f_iterator;  // must be defined in constructors
    std::uint32_t           f_line = 1;
    std::uint32_t           f_column = 1;
    //input_status_t::vector_t
    //                        f_input_status = input_status_t::vector_t();

    //int                     f_unget_pos = 0;
    //character_t             f_unget[2] = {};
    bool                    f_eos = false;
    bool                    f_code_block = false;
    std::uint32_t           f_list_subblock = 0;
    features                f_features = features();
    character::string_t     f_last_line = character::string_t();
    //indentation_t           f_indentation = indentation_t::INDENTATION_PARAGRAPH;
    int                     f_current_gap = 0;
    block::pointer_t        f_document = block::pointer_t();
    block::pointer_t        f_last_block = block::pointer_t();
    block::pointer_t        f_top_working_block = block::pointer_t();
    block::pointer_t        f_working_block = block::pointer_t();

    link::map_t             f_links = link::map_t();

    std::string             f_output = std::string();
};



} // namespace cm
// vim: ts=4 sw=4 et
