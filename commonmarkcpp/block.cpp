// Copyright (c) 2021-2025  Made to Order Software Corp.  All Rights Reserved
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


// snapdev
//
#include    <snapdev/trim_string.h>


// C++
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace cm
{



std::string type_to_string(char32_t type)
{
    switch(type)
    {
    case BLOCK_TYPE_DOCUMENT:
        return "DOCUMENT";

    case BLOCK_TYPE_LINE:
        return "LINE";

    case BLOCK_TYPE_PARAGRAPH:
        return "PARAGRAPH";

    case BLOCK_TYPE_TEXT:
        return "TEXT";

    case BLOCK_TYPE_CODE_BLOCK_INDENTED:
        return "CODE_BLOCK_INDENTED";

    case BLOCK_TYPE_CODE_BLOCK_GRAVE:
        return "CODE_BLOCK_GRAVE";

    case BLOCK_TYPE_CODE_BLOCK_TILDE:
        return "CODE_BLOCK_TILDE";

    case BLOCK_TYPE_LIST_ASTERISK:
        return "LIST_ASTERISK";

    case BLOCK_TYPE_LIST_PLUS:
        return "LIST_PLUS";

    case BLOCK_TYPE_LIST_DASH:
        return "LIST_DASH";

    case BLOCK_TYPE_LIST_PERIOD:
        return "LIST_PERIOD";

    case BLOCK_TYPE_LIST_PARENTHESIS:
        return "LIST_PARENTHESIS";

    case BLOCK_TYPE_TAG:
        return "TAG";

    case BLOCK_TYPE_BLOCKQUOTE:
        return "BLOCKQUOTE";

    case BLOCK_TYPE_HEADER_OPEN:
        return "HEADER_OPEN";

    case BLOCK_TYPE_HEADER_ENCLOSED:
        return "HEADER_ENCLOSED";

    case BLOCK_TYPE_HEADER_SINGLE:
        return "HEADER_SINGLE";

    case BLOCK_TYPE_HEADER_DOUBLE:
        return "HEADER_DOUBLE";

    case BLOCK_TYPE_BREAK_DASH:
        return "BREAK_DASH";

    case BLOCK_TYPE_BREAK_ASTERISK:
        return "BREAK_ASTERISK";

    case BLOCK_TYPE_BREAK_UNDERLINE:
        return "BREAK_UNDERLINE";

    default:
        return "<unknown type>";

    }
    snapdev::NOT_REACHED();
}





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
 * \sa is_indented_code_block()
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
 * \return true if the type is one of BLOCK_TYPE_CODE_BLOCK_....
 */
bool block::is_code_block() const
{
    return f_type == BLOCK_TYPE_CODE_BLOCK_INDENTED
        || f_type == BLOCK_TYPE_CODE_BLOCK_GRAVE
        || f_type == BLOCK_TYPE_CODE_BLOCK_TILDE;
}


/** \brief Check whether this block represents an indented block of code.
 *
 * The function checks the type to see whether it represents an indented
 * block of code or not. If so, it returns true.
 *
 * The indented code blocks can be continued which is why we need to be
 * able to distinguish this one type from the other.
 *
 * \return true if the type is BLOCK_TYPE_CODE_BLOCK_INDENTED.
 */
bool block::is_indented_code_block() const
{
    return f_type == BLOCK_TYPE_CODE_BLOCK_INDENTED;
}


/** \brief Check whether this block represents a fenced block of code.
 *
 * The function checks the type to see whether it represents a fenced
 * block of code or not. If so, it returns true.
 *
 * The fenced code blocks can appear in a list with the standard list
 * indentation which is why we need to be able to distinguish this one
 * type from the other.
 *
 * \return true if the type is one of BLOCK_TYPE_CODE_BLOCK_GRAVE or
 * BLOCK_TYPE_CODE_BLOCK_TILDE.
 */
bool block::is_fenced_code_block() const
{
    return f_type == BLOCK_TYPE_CODE_BLOCK_GRAVE
        || f_type == BLOCK_TYPE_CODE_BLOCK_TILDE;
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
    return f_type == BLOCK_TYPE_LIST_ASTERISK
        || f_type == BLOCK_TYPE_LIST_DASH
        || f_type == BLOCK_TYPE_LIST_PERIOD
        || f_type == BLOCK_TYPE_LIST_PARENTHESIS;
}


/** \brief Check whether this block or a parent is a list.
 *
 * This function checks this block and all of its parents for one that
 * represents a list. If so, then the function returns true.
 *
 * \return true if this block or one of its parent is a list.
 */
bool block::is_in_list() const
{
    for(pointer_t b(const_cast<block *>(this)->shared_from_this());
        b != nullptr;
        b = b->parent())
    {
        if(b->is_list())
        {
            return true;
        }
    }
    return false;
}


/** \brief Get the first block representing a list.
 *
 * This function looks at `this` block and through the list of its parents
 * for a block representing a list (see is_list()). If one such block is
 * found, then it gets returned. Otherwise the function returns a nullptr.
 *
 * \return The list in the parent lineage or nullptr.
 */
block::pointer_t block::find_list() const
{
    for(pointer_t b(const_cast<block *>(this)->shared_from_this());
        b != nullptr;
        b = b->parent())
    {
        if(b->is_list())
        {
            return b;
        }
    }
    return block::pointer_t();
}


bool block::is_tight_list() const
{
    pointer_t b(const_cast<block *>(this)->shared_from_this());
    if(b == nullptr)
    {
        throw std::logic_error("can't shared_from_this() in is_tight_list()");
    }

    char32_t const type(f_type.f_char);

    // special case were we have a single item
    //
    if(b->children_size() == 1
    && (b->next() == nullptr
        || b->next()->type().f_char != type))
    {
//std::cerr << "- * ---------------------------- DOCUMENT TREE LIST TIGHT LIST FAILED AT:\n";
//std::cerr << "- followed by empty line?! " << std::boolalpha << b->followed_by_an_empty_line() << "\n";
//std::cerr << b->tree();
//std::cerr << "- includes empty lines?! " << b->first_child()->includes_blocks_with_empty_lines(false) << "\n";
//std::cerr << "- * ---------------------------- DOCUMENT TREE LIST TIGHT LIST FAILED AT END\n";
        return true;
        //return !b->first_child()->includes_blocks_with_empty_lines(false);
    }

    for(; b != nullptr && b->type().f_char == type; b = b->next())
    {
        if(b->followed_by_an_empty_line())
        //|| (b->first_child() != nullptr
        //        && b->first_child()->includes_blocks_with_empty_lines(false)))
        {
            if(b->next() != nullptr
            && b->next()->type().f_char != type)
            {
                return true;
            }
            return false;
        }
    }

    return true;
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
    return f_type == BLOCK_TYPE_LIST_PERIOD
        || f_type == BLOCK_TYPE_LIST_PARENTHESIS;
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
    return f_type == BLOCK_TYPE_LIST_ASTERISK
        || f_type == BLOCK_TYPE_LIST_DASH;
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
    return f_type == BLOCK_TYPE_BLOCKQUOTE;
}


/** \brief Check whether this block or one of its parent is a blockquote.
 *
 * The function checks the type of this block to see whether it represents
 * a blockquote. If not, then it tries again with its parent. It repeats
 * so until the parent is nullptr. If any of the blocks was a blockquote,
 * then the function returns true.
 *
 * \return true if block is a blockquote or at least one of the parents
 * is a blockquote.
 */
bool block::is_in_blockquote() const
{
    if(is_blockquote())
    {
        return true;
    }

    for(block::pointer_t p(parent()); p != nullptr; p = p->parent())
    {
        if(p->is_blockquote())
        {
            return true;
        }
    }

    return false;
}


block::pointer_t block::find_blockquote() const
{
    if(is_blockquote())
    {
        return const_cast<block *>(this)->shared_from_this();
    }

    for(block::pointer_t p(parent()); p != nullptr; p = p->parent())
    {
        if(p->is_blockquote())
        {
            return p;
        }
    }

    return block::pointer_t();
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
    return f_type == BLOCK_TYPE_HEADER_OPEN
        || f_type == BLOCK_TYPE_HEADER_ENCLOSED
        || f_type == BLOCK_TYPE_HEADER_SINGLE
        || f_type == BLOCK_TYPE_HEADER_DOUBLE;
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
    return f_type == BLOCK_TYPE_BREAK_DASH
        || f_type == BLOCK_TYPE_BREAK_UNDERLINE
        || f_type == BLOCK_TYPE_BREAK_ASTERISK;
}


/** \brief Check whether this block represents a tag.
 *
 * The function checks the type of this block to see whether it represents
 * a tag or not. If so, it returns true.
 *
 * A tag is a set of verbatim HTML strings saved in the block contents.
 *
 * \return true if the type is one of the BLOCK_TYPE_TAG.
 */
bool block::is_tag() const
{
    return f_type == BLOCK_TYPE_TAG;
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


/** \brief Search for a blockquote and return its end column.
 *
 * This function searches for a blockquote. If one is found, then its
 * end column is returned. If no blockquote is found, the function
 * returns 0.
 *
 * \return The end column of the blockquote if on exists.
 */
int block::get_blockquote_end_column()
{
    for(pointer_t b(shared_from_this());
        b != nullptr;
        b = b->parent())
    {
        if(b->is_blockquote())
        {
            return b->end_column();
        }
    }

    return 0;
}


/** \brief Define the list starting number.
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
 * \param[in] n  The list starting number.
 */
void block::number(int n)
{
    if(!is_ordered_list()
    && !is_blockquote()
    && !is_header())
    {
        throw commonmark_logic_error(
              "number(int) called on a non-compatible type of block ("
            +  type_to_string(f_type.f_char)
            + ").");
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
    && !is_blockquote()
    && !is_header())
    {
        throw commonmark_logic_error(
              "number() called on a non-compatible type of block ("
            + type_to_string(f_type.f_char)
            + ").");
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


/** \brief Check whether a block is followed by a new line.
 *
 * This function is recursive. It will check whether this block, any of
 * its following siblings (not preceeding), or any of its children is
 * followed by an empty line.
 *
 * If such an empty line if found in this tree, then the function returns
 * true. In all other cases, it returns false.
 *
 * \param[in] recursive  Whether to recurse in the children of the children.
 *
 * \return true if the tree starting with 'this' block includes at least
 * one container followed by an empty line.
 */
bool block::includes_blocks_with_empty_lines(bool recursive) const
{
    pointer_t b(const_cast<block *>(this)->shared_from_this());
    if(b == nullptr)
    {
        throw std::logic_error("can't shared_from_this() in is_tight_list()");
    }

    for(; b != nullptr; b = b->next())
    {
        // TBD: we may need to limit to tests to blocks of type LIST and
        //      BLOCKQUOTE...
        //
        if(b->followed_by_an_empty_line()
        || (recursive
                && b->first_child() != nullptr
                && b->first_child()->includes_blocks_with_empty_lines(true)))
        {
            return true;
        }
    }

    return false;
}


/** \brief Save the info string in this block.
 *
 * Blocks of code can include an information string. This is that string.
 * Since it comes from the input, we expect it to be in a character
 * string (if not add a new function with std::string).
 *
 * \note
 * The function will trim the info string from starting and ending spaces
 * before saving it.
 *
 * \param[in] info  The info string as found in the input document.
 *
 * \sa info_string() const
 */
void block::info_string(character::string_t const & info)
{
    f_info_string = info;
}


/** \brief Retrieve the info string.
 *
 * The info string is whatever string appears after the fenced code block
 * marker. For example:
 *
 * \code
 *     ``` lang:html
 * \endcode
 *
 * the info string is going to be "lang:html". This function will trim
 * starting and ending spaces.
 *
 * Note that the commonmark specification does not specify what the
 * info_string is expected to be. It only acknowledge that it exists
 * and how to retrieve it from the source document.
 *
 * But at the same time they have examples where they convert the first
 * part of the info string (up to the first space) into a language name
 * as follow:
 *
 * \code
 *     ``` ruby
 *     ... ruby code ...
 *     ```
 * \endcode
 *
 * I that example, it converts the word `ruby` in a `class="language-ruby"`
 * of the code tag.
 *
 * \return The info string as it is; an empty string by default.
 */
character::string_t const & block::info_string() const
{
    return f_info_string;
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


std::size_t block::children_size() const
{
    std::size_t count(0);

    for(pointer_t b(f_first_child);
        b != nullptr;
        b = b->next(), ++count);

    return count;
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


std::string block::tree() const
{
    return to_string(0, true);
}


std::string block::to_string(int indentation, bool children) const
{
    std::string indent(indentation, ' ');
    std::string output;

    output += indent;
    output += "+ ";
    output += type_to_string(f_type.f_char);
    if(f_type.f_line != 0
    && f_type.f_column != 0)
    {
        output += " (line/column: ";
        output += std::to_string(f_type.f_line);
        output += '/';
        output += std::to_string(f_type.f_column);
        if(f_end_column > f_type.f_column)
        {
            output += '-';
            output += std::to_string(f_end_column);
        }
        output += ')';
    }
    output += "\n";

    pointer_t pr(previous());
    pointer_t up(parent());

    if(f_next != nullptr
    || pr != nullptr
    || up != nullptr
    || f_first_child != nullptr)
    {
        output += indent;
        output += "  - ";
        bool first(true);
        if(f_next != nullptr)
        {
            if(first)
            {
                first = false;
            }
            else
            {
                output += ", ";
            }
            output += "Next Sibling (";
            output += type_to_string(f_next->f_type.f_char);
            output += ')';
        }
        if(pr != nullptr)
        {
            if(first)
            {
                first = false;
            }
            else
            {
                output += ", ";
            }
            output += "Previous Sibling (";
            output += type_to_string(pr->f_type.f_char);
            output += ')';
        }
        if(up != nullptr)
        {
            if(first)
            {
                first = false;
            }
            else
            {
                output += ", ";
            }
            output += "Parent (";
            output += type_to_string(up->f_type.f_char);
            output += ')';
        }
        if(f_first_child != nullptr)
        {
            if(first)
            {
                first = false;
            }
            else
            {
                output += ", ";
            }
            output += "Has Children (First: ";
            output += type_to_string(f_first_child->f_type.f_char);
            if(f_first_child != f_last_child)
            {
                output += ", Last: ";
                output += type_to_string(f_last_child->f_type.f_char);
            }
            output += ", Count: ";
            output += std::to_string(children_size());
            output += ')';
        }
        output += '\n';
    }

    if(!f_content.empty())
    {
        output += indent;
        std::string const intro("  - Content: \"");
        output += intro;
        std::string const content(character::to_utf8(f_content));
        std::size_t const limit(std::max(20UL, 77 - intro.length() - indent.length()));
        if(limit >= content.length())
        {
            output += content;
        }
        else
        {
            std::string const more(" [...]");
            output += content.substr(0, limit - more.length());
            output += more;
        }
        output += "\"\n";
    }

    if(!f_info_string.empty())
    {
        output += indent;
        std::string const info_intro("  - String Info: \"");
        output += info_intro;
        std::string const info_string(character::to_utf8(f_info_string));
        std::size_t const info_limit(std::max(20UL, 77 - info_intro.length() - indent.length()));
        if(info_limit >= info_string.length())
        {
            output += info_string;
        }
        else
        {
            std::string const more(" [...]");
            output += info_string.substr(0, info_limit - more.length());
            output += more;
        }
        output += "\"\n";
    }

    if(f_number >= 0)
    {
        output += indent;
        output += "  - Number: ";
        output += std::to_string(f_number);
        output += '\n';
    }

    if(f_followed_by_an_empty_line)
    {
        output += indent;
        output += "  - Block is followed by at least one empty line\n";
    }

    if(children)
    {
        for(auto b(f_first_child);
            b != nullptr;
            b = b->f_next)
        {
            output += b->to_string(indentation + 2, true);
        }
    }

    return output;
}




} // namespace cm
// vim: ts=4 sw=4 et
