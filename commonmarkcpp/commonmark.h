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
#include    "commonmarkcpp/block.h"
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

    void                    add_document_div(bool add = true);
    void                    add_classes(bool add = true);
    void                    add_space_in_empty_tag(bool add = true);
    void                    set_line_feed(std::string const & line_feed);
    std::string const &     get_line_feed() const;
    void                    convert_entities(bool convert);
    void                    add_link(
                                  std::string const & name
                                , std::string const & destination
                                , std::string const & title
                                , bool reference);
    link::pointer_t         find_link_reference(std::string const & name);

    std::string             process(std::string const & input);

private:
    character               getc();
    void                    get_line();
    static bool             is_empty(character::string_t const & str);

    void                    parse();
    character::string_t::const_iterator
                            parse_containers();
    bool                    parse_blank(character::string_t::const_iterator & it);
    bool                    parse_blockquote(character::string_t::const_iterator & it);
    bool                    parse_list(character::string_t::const_iterator & it);
    bool                    is_thematic_break(character::string_t::const_iterator it);

    void                    process_empty_line();
    void                    append_line();

    bool                    process_paragraph(character::string_t::const_iterator & it);
    bool                    process_thematic_break_or_setext_heading(character::string_t::const_iterator & it);
    bool                    process_reference_definition(character::string_t::const_iterator & it);
    bool                    parse_reference_destination(
                                  character::string_t::const_iterator & et
                                , std::string & link_destination
                                , std::string & link_title);
    bool                    process_header(character::string_t::const_iterator & it);
    bool                    process_indented_code_block(character::string_t::const_iterator & it);
    bool                    process_fenced_code_block(character::string_t::const_iterator & it);
    bool                    process_html_blocks(character::string_t::const_iterator & it);

    void                    generate(block::pointer_t b);
    void                    generate_list(block::pointer_t b);
    void                    generate_header(block::pointer_t b);
    void                    generate_thematic_break(block::pointer_t b);
    void                    generate_inline(character::string_t const & line);
    std::string             generate_attribute(std::string const & line);
    void                    generate_code(block::pointer_t b);

    std::string             f_input = std::string();
    libutf8::utf8_iterator  f_iterator;  // must be defined in constructors
    //int                     f_unget_pos = 0;
    //character_t             f_unget[2] = {};
    bool                    f_eos = false;
    bool                    f_code_block = false;
    std::uint32_t           f_list_subblock = 0;
    std::uint32_t           f_line = 1;
    std::uint32_t           f_column = 1;
    character::string_t     f_last_line = character::string_t();
    //indentation_t           f_indentation = indentation_t::INDENTATION_PARAGRAPH;
    int                     f_current_gap = 0;
    block::pointer_t        f_document = block::pointer_t();
    block::pointer_t        f_last_block = block::pointer_t();
    block::pointer_t        f_top_working_block = block::pointer_t();
    block::pointer_t        f_working_block = block::pointer_t();

    link::map_t             f_links = link::map_t();

    std::string             f_line_feed = std::string();
    bool                    f_add_document_div = false;
    bool                    f_add_classes = false;
    bool                    f_add_space_in_empty_tag = false;
    bool                    f_convert_entities = true;
    bool                    f_flushed = false;

    std::string             f_output = std::string();
};



} // namespace cm
// vim: ts=4 sw=4 et
