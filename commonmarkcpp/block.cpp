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
 * \brief Implementation of the block class.
 *
 * This file is the implementation of the block class allowing one to
 * hold the data of one block.
 *
 * Blocks are organized in a tree where there is a start block, siblings
 * (next/previous) and children (parent/child).
 *
 * A leaf block, such as a paragraph, cannot have children.
 *
 * A container block, such as a list, can have children.
 */

// self
//
#include    "commonmarkcpp/block.h"


// C++ lib
//
//#include    <algorithm>
//#include    <cstring>
#include    <iostream>
//#include    <sstream>


// last include
//
#include    <snapdev/poison.h>



namespace cm
{



/** \brief Initialize the block.
 *
 * The constructor creates a block of the given type. The type of a block
 * cannot be changed later so it needs to be known at the time you create
 * the block. If the type is not quite known at first, parse as much as
 * necessary, saving that information to variables in the meantime.
 *
 * By default the type is set to PARAGRAPH, which is probably not that
 * useful.
 *
 * \param[in] type  The type of this block.
 */
block::block(character const & type)
    : f_type(type)
    , f_end_column(type.f_column)
{
}


/** \brief Retrieve the type of the block.
 *
 * This function returns the type of this block. The type is saved at the
 * time a block is create and cannot be modified later.
 *
 * The type being saved in a character, it includes the location (line
 * and column) where it was found.
 *
 * There are helper functions one can use to quickly test whether the
 * block is of a given type.
 *
 * \return The type of this block.
 *
 * \sa is_paragraph()
 * \sa is_code_block()
 * \sa is_list()
 * \sa is_ordered_list()
 * \sa is_unordered_list()
 * \sa is_blockquote()
 */
character block::type() const
{
    return f_type;
}


bool block::is_document() const
{
    return f_type == BLOCK_TYPE_DOCUMENT;
}


bool block::is_line() const
{
    return f_type == BLOCK_TYPE_LINE;
}


/** \brief Check whether this block represents a paragraph.
 *
 * The function checks the type to see whether it represents a paragraph
 * or not. If so, it returns true.
 *
 * A paragrah block is inline content that is to appear inside a \<p\>
 * tag.
 *
 * \return true if the type is BLOCK_TYPE_PARAGRAPH.
 */
bool block::is_paragraph() const
{
    return f_type.f_char == BLOCK_TYPE_PARAGRAPH;
}


/** \brief Check whether this block represents a block of code.
 *
 * The function checks the type to see whether it represents a block of code
 * or not. If so, it returns true.
 *
 * A block of code is content rendered within a \<pre\> tag. All the content
 * is viewed as verbatim, which means that it is not parsed for any kind of
 * special anything (no bold, no italic, no underline, no strikethrough...)
 *
 * \return true if the type is BLOCK_TYPE_CODE_BLOCK.
 */
bool block::is_code_block() const
{
    return f_type.f_char == BLOCK_TYPE_CODE_BLOCK;
}


/** \brief Check whether this block represents a list item.
 *
 * The function checks the type to see whether it represents a list item
 * or not. If so, it returns true.
 *
 * There are currently four types of list items. Two are for ordered lists
 * and two are for unordered lists. You can distinguish such by using the
 * is_ordered_list() and is_unordered_list() functions instead.
 *
 * List items are defined inside an \<li\> tag.
 *
 * \return true if the type is one of the BLOCK_TYPE_LIST_....
 */
bool block::is_list() const
{
    return f_type.f_char == BLOCK_TYPE_LIST_ASTERISK
        || f_type.f_char == BLOCK_TYPE_LIST_DASH
        || f_type.f_char == BLOCK_TYPE_LIST_PERIOD
        || f_type.f_char == BLOCK_TYPE_LIST_PARENTHESIS;
}


/** \brief Check whether this block represents an ordered list item.
 *
 * The function checks the type to see whether it represents an ordered
 * list item or not. If so, it returns true.
 *
 * List items are defined inside an \<li\> tag. Ordered lists are defined
 * between \<ol\>.
 *
 * \return true if the type is one of the BLOCK_TYPE_LIST_... representing
 * an ordered list item.
 */
bool block::is_ordered_list() const
{
    return f_type.f_char == BLOCK_TYPE_LIST_PERIOD
        || f_type.f_char == BLOCK_TYPE_LIST_PARENTHESIS;
}


/** \brief Check whether this block represents an unordered list item.
 *
 * The function checks the type to see whether it represents an unordered
 * list item or not. If so, it returns true.
 *
 * List items are defined inside an \<li\> tag. Unordered lists are defined
 * between \<ul\>.
 *
 * \return true if the type is one of the BLOCK_TYPE_LIST_... representing
 * an unordered list item.
 */
bool block::is_unordered_list() const
{
    return f_type.f_char == BLOCK_TYPE_LIST_ASTERISK
        || f_type.f_char == BLOCK_TYPE_LIST_DASH;
}


/** \brief Check whether this block represents a blockquote.
 *
 * The function checks the type to see whether it represents a blockquote
 * or not. If so, it returns true.
 *
 * A blockquote is content rendered within a \<pre\> tag. All the content
 * is viewed as verbatim, which means that it is not parsed for any kind of
 * special anything (no bold, no italic, no underline, no strikethrough...)
 *
 * \return true if the type is BLOCK_TYPE_BLOCKQUOTE.
 */
bool block::is_blockquote() const
{
    return f_type.f_char == BLOCK_TYPE_BLOCKQUOTE;
}


/** \brief Check whether this block represents a header.
 *
 * The function checks the type of this block to see whether it represents
 * a header or not. If so, it returns true.
 *
 * A header is content rendered within an \<hN\> tag where N is the number()
 * representing the header level. The content is viewed as inline.
 *
 * \return true if the type is one of the BLOCK_TYPE_HEADER_....
 */
bool block::is_header() const
{
    return f_type.f_char == BLOCK_TYPE_HEADER_OPEN
        || f_type.f_char == BLOCK_TYPE_HEADER_ENCLOSED
        || f_type.f_char == BLOCK_TYPE_HEADER_UNDERLINED
        || f_type.f_char == BLOCK_TYPE_HEADER_DOUBLE;
}


/** \brief Check whether this block represents a thematic break.
 *
 * The function checks the type of this block to see whether it represents
 * a thematic break or not. If so, it returns true.
 *
 * A thematic break is a stand alone tag rendered with an \<hr/\> tag.
 * If classes are allowed, the type of break is added as a class.
 *
 * \return true if the type is one of the BLOCK_TYPE_BREAK_....
 */
bool block::is_thematic_break() const
{
    return f_type.f_char == BLOCK_TYPE_BREAK_DASH
        || f_type.f_char == BLOCK_TYPE_BREAK_UNDERLINE
        || f_type.f_char == BLOCK_TYPE_BREAK_ASTERISK;
}


/** \brief Define the line at which the block started.
 *
 * This function returns the line found in the type used to create the
 * block. This line number represents the first line on which the block
 * started.
 *
 * \return The line as defined in the type used to create the block.
 */
int block::line() const
{
    return f_type.f_line;
}


/** \brief Define the start column.
 *
 * Whenever a block is created, you can use the first character as the type
 * and that character will hold the position (line & column) of the start
 * of the block. This is useful in case you want to indicate an error at
 * a precise location.
 *
 * \return The start column as defined in the character representing the type.
 */
int block::column() const
{
    return f_type.f_column;
}


/** \brief Define the end column.
 *
 * Whenever a block introducer is read, it can span on multiple columns
 * and this value indicates the end column. The start column is defined
 * along the type character and you can read that value with the column()
 * function.
 *
 * \param[in] n  The new value for the end column.
 */
void block::end_column(int n)
{
    f_end_column = n;
}


/** \brief Column in which this block introducer ends.
 *
 * In many cases we need to know whether the following block is indented
 * or not (i.e. a code block or a regular block). This column indicates
 * the last column in which the introducer ends which tells us how to
 * interpret the following text. This is particularly useful in list
 * items and blockquotes.
 *
 * \return The end column as defined by the end_column(int n) function.
 */
int block::end_column() const
{
    return f_end_column;
}


/** \brief Define the list start number.
 *
 * List items defined as a number followed by either a period (`'.'`) or a
 * closing parenthesis (`')'`) represent an ordered list. The given number
 * is considered to be the start number of the list. The character following
 * the period or closing parenthesis represent the list item type.
 *
 * The type will be reflected in the tags using a class.
 *
 * \exception commonmark_logic_error
 * The start point of a list can only be defined for those two types of list.
 * If the block represents anything else, then this error is raised.
 *
 * \param[in] start  The list start point.
 */
void block::number(int n)
{
    if(!is_ordered_list()
    && !is_header())
    {
        throw commonmark_logic_error("number() called on a non-compatible type of block.");
    }

    f_number = n;
}


/** \brief Retrieve the block number.
 *
 * If this block represents an ordered list, then this number represents
 * the start number of the list item.
 *
 * If this block represents a header, then this number represents the
 * header number (1 to 6).
 *
 * The number is set to -1 otherwise.
 *
 * \return The number of this block or -1.
 */
int block::number() const
{
    if(!is_ordered_list()
    && !is_header())
    {
        throw commonmark_logic_error("number() called on a non-compatible type of block.");
    }

    return f_number;
}


/** \brief Marked the block as being followed by an empty line.
 *
 * This function is called to set the empty line flag to true. This is
 * important to distinguish a block direct continuation or a separate
 * paragraph or list item.
 *
 * \code
 *     The first item is followed by more data but no empty line, so it is
 *     just view as all inline characters:
 *
 *     * Item 1
 *       continuation of Item 1
 *     * Item 2
 *
 *     ---
 *
 *     The following list uses <li> only (no <p>):
 *
 *     * Item 1
 *     * Item 2
 *
 *     ---
 *
 *     The following list uses <li> and <p>:
 *
 *     * Item 1
 *
 *     * Item 2
 *
 *     ---
 *
 *     The following list item is followed by a block representing a
 *     sub-paragraph, not a block of code:
 *
 *     * Item 1
 *
 *         This is a sub-block, it's separated by an empty line.
 * \endcode
 *
 * \param[in] followed  Whether the block is followed by an empty line or not.
 */
void block::followed_by_an_empty_line(bool followed)
{
    f_followed_by_an_empty_line = followed;
}


/** \brief Check whether the block is followed by an empty line.
 *
 * There are more information in the followed_by_an_empty_line(bool followed)
 * function description.
 *
 * \return true if the block was found to be followed by an empty line.
 */
bool block::followed_by_an_empty_line() const
{
    return f_followed_by_an_empty_line;
}


/** \brief Link new block as a child.
 *
 * This function adds the specified block, \p child, as a child of `this`
 * block.
 *
 * If `this` block already has children, then this is equivalent to calling
 * link_sibling() with any one of the siblings. It will always be linked at
 * the end of the list.
 *
 * \param[in] child  The child to be linked to this block.
 */
void block::link_child(pointer_t child)
{
    if(child->parent() != nullptr
    || child->previous() != nullptr
    || child->next() != nullptr)
    {
        throw commonmark_logic_error("new sibling already has a parent, next, or previous block.");
    }

    if(first_child() == nullptr)
    {
        f_first_child = child;
        f_last_child = child;
        child->f_parent = shared_from_this();
    }
    else
    {
        if(last_child() == nullptr)
        {
            throw commonmark_logic_error("f_last_child is nullptr when f_first_child is not.");
        }

        last_child()->link_sibling(child);
    }
}


/** \brief Link a new sibling at the end of the list.
 *
 * This function links the specified block \p sibling at the end of the list of
 * siblings pointed by `this` block (which is represented as the last child
 * found in the parent block). `this` block does not need to be the last
 * child in the list.
 *
 * It is not possible to re-link a block without first unlinking it.
 *
 * \param[in] sibling  The block to link at the end of this sibling list.
 */
void block::link_sibling(pointer_t sibling)
{
    if(sibling->parent() != nullptr
    || sibling->previous() != nullptr
    || sibling->next() != nullptr)
    {
        throw commonmark_logic_error("new sibling already has a parent, next, or previous block.");
    }

    pointer_t p(parent());
    if(p == nullptr)
    {
        throw commonmark_logic_error("parent less blocks (line & document) cannot have siblings.");
    }

    pointer_t lc(p->last_child());
    if(lc->next() != nullptr)
    {
        throw commonmark_logic_error("last child already has a next block.");
    }

    sibling->f_parent = f_parent;
    sibling->f_previous = lc;
    lc->f_next = sibling;
    p->f_last_child = sibling;
}


/** \brief Unlink this block from the tree.
 *
 * This function unlinks `this` block from the tree.
 *
 * \note
 * Unlinking the block does not involve the children of `this` block.
 * So its children will still be present and can be accessed with the
 * first_child() and then next()/previous() to find the sibling.
 *
 * \return The next, previous or parent pointer, whichever is not a nullptr
 * first.
 */
block::pointer_t block::unlink()
{
    pointer_t n(next());
    pointer_t p(previous());
    pointer_t u(parent());

    if(n != nullptr)
    {
        if(p != nullptr)
        {
            // inside block, it can be the first/last child in the parent
            //
            p->f_next = n;
            n->f_previous = p;

#ifdef _DEBUG
            if(u != nullptr
            && (u->f_first_child == shared_from_this()
                || u->f_last_child == shared_from_this()))
            {
                throw commonmark_logic_error("unlink found an invalid parent/child link.");
            }
#endif
        }
        else if(u != nullptr)
        {
#ifdef _DEBUG
            if(u->f_first_child != shared_from_this())
            {
                throw commonmark_logic_error("unlink did not find this as the first child.");
            }
#endif

            u->f_first_child = n;
            n->f_previous.reset();
        }
    }
    else if(p != nullptr)
    {
        p->f_next.reset();
        if(u != nullptr)
        {
#ifdef _DEBUG
            if(u->f_last_child != shared_from_this())
            {
                throw commonmark_logic_error("unlink did not find this as the last child.");
            }
#endif

            u->f_last_child = p;
        }
    }
    else if(u != nullptr)
    {
#ifdef _DEBUG
        if(u->f_first_child != shared_from_this()
        || u->f_last_child != shared_from_this())
        {
            throw commonmark_logic_error("unlink found an invalid parent/child link.");
        }
#endif

        u->f_first_child.reset();
        u->f_last_child.reset();
    }

    f_next.reset();
    f_previous.reset();
    f_parent.reset();

    return n != nullptr
            ? n
            : (p != nullptr
                ? p
                : u);
}


block::pointer_t block::next() const
{
    return f_next;
}


block::pointer_t block::previous() const
{
    return f_previous.lock();
}


block::pointer_t block::parent() const
{
    return f_parent.lock();
}


block::pointer_t block::first_child() const
{
    return f_first_child;
}


block::pointer_t block::last_child() const
{
    return f_last_child;
}


void block::append(character const & c)
{
    f_content += c;
}


void block::append(character::string_t const & content)
{
    f_content += content;
}


character::string_t const & block::content() const
{
    return f_content;
}



} // namespace cm
// vim: ts=4 sw=4 et

