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
 * \brief Declaration of the block class.
 *
 * A block represents a set of lines organized together in a paragraph,
 * list, blockquote and other similar objects.
 */


// self
//
#include    "commonmarkcpp/character.h"


// libutf8 lib
//


// C++ lib
//
#include    <memory>



namespace cm
{



constexpr char32_t const    BLOCK_TYPE_DOCUMENT = U'\x1F5CE'; // document symbol
constexpr char32_t const    BLOCK_TYPE_LINE = U'\x2104'; // center line symbol
constexpr char32_t const    BLOCK_TYPE_PARAGRAPH = U'\x00B6'; // paragraph symbol
constexpr char32_t const    BLOCK_TYPE_TEXT = U'\x200B'; // zero width space
constexpr char32_t const    BLOCK_TYPE_CODE_BLOCK_INDENTED = U'\t';
constexpr char32_t const    BLOCK_TYPE_CODE_BLOCK_GRAVE = U'`';
constexpr char32_t const    BLOCK_TYPE_CODE_BLOCK_TILDE = U'~';
constexpr char32_t const    BLOCK_TYPE_LIST_ASTERISK = U'*';
constexpr char32_t const    BLOCK_TYPE_LIST_PLUS = U'+';
constexpr char32_t const    BLOCK_TYPE_LIST_DASH = U'-';
constexpr char32_t const    BLOCK_TYPE_LIST_PERIOD = U'.';
constexpr char32_t const    BLOCK_TYPE_LIST_PARENTHESIS = U')';
constexpr char32_t const    BLOCK_TYPE_TAG = U'<';
constexpr char32_t const    BLOCK_TYPE_BLOCKQUOTE = U'>';
constexpr char32_t const    BLOCK_TYPE_HEADER_OPEN = U'#';
constexpr char32_t const    BLOCK_TYPE_HEADER_ENCLOSED = U'\x1F157'; // circle enclosed H
constexpr char32_t const    BLOCK_TYPE_HEADER_SINGLE = U'_'; // source is '-', but '-' is used for lists
constexpr char32_t const    BLOCK_TYPE_HEADER_DOUBLE = U'=';
constexpr char32_t const    BLOCK_TYPE_BREAK_DASH = U'\x2022'; // bullet point (cicle)
constexpr char32_t const    BLOCK_TYPE_BREAK_ASTERISK = U'\x2023'; // bullet point (triangle)
constexpr char32_t const    BLOCK_TYPE_BREAK_UNDERLINE = U'\x2024'; // bullet point (small circle)



std::string                 type_to_string(char32_t type);



class block
    : public std::enable_shared_from_this<block>
{
public:
    typedef std::shared_ptr<block>      pointer_t;
    typedef std::weak_ptr<block>        weak_t;

                            block(character const & type);

    character               type() const;
    bool                    is_document() const;
    bool                    is_line() const;
    bool                    is_paragraph() const;
    bool                    is_code_block() const;
    bool                    is_indented_code_block() const;
    bool                    is_fenced_code_block() const;
    bool                    is_list() const;
    bool                    is_in_list() const;
    pointer_t               find_list() const;
    bool                    is_tight_list() const;
    bool                    is_ordered_list() const;
    bool                    is_unordered_list() const;
    bool                    is_blockquote() const;
    bool                    is_in_blockquote() const;
    pointer_t               find_blockquote() const;
    bool                    is_header() const;
    bool                    is_thematic_break() const;
    bool                    is_tag() const;

    int                     line() const;
    int                     column() const;
    void                    end_column(int n);
    int                     end_column() const;
    int                     get_blockquote_end_column();
    void                    number(int n);
    int                     number() const;
    void                    followed_by_an_empty_line(bool followed);
    bool                    followed_by_an_empty_line() const;
    bool                    includes_blocks_with_empty_lines(bool recursive) const;
    void                    info_string(character::string_t const & info);
    character::string_t const &
                            info_string() const;

    void                    link_child(pointer_t child);
    void                    link_sibling(pointer_t sibling);
    pointer_t               unlink();

    pointer_t               next() const;
    pointer_t               previous() const;
    pointer_t               parent() const;
    pointer_t               first_child() const;
    pointer_t               last_child() const;
    std::size_t             children_size() const;

    void                    append(character const & c);
    void                    append(character::string_t const & content);
    character::string_t const &
                            content() const;

    std::string             tree() const;
    std::string             to_string(int indentation = 0, bool children = false) const;

private:
    pointer_t               f_next = pointer_t();
    weak_t                  f_previous = weak_t();
    weak_t                  f_parent = weak_t();
    pointer_t               f_first_child = pointer_t();
    pointer_t               f_last_child = pointer_t();

    character const         f_type;
    std::uint32_t           f_end_column = 0;
    character::string_t     f_content = character::string_t();
    character::string_t     f_info_string = character::string_t();
    int                     f_number = -1;
    bool                    f_followed_by_an_empty_line = false;
};



} // namespace cm
// vim: ts=4 sw=4 et
