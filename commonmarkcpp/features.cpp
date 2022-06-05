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
#include    "commonmarkcpp/features.h"


// C++ lib
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace cm
{



void features::set_commonmark_compatible()
{
    f_add_document_div = false;
    f_add_classes = false;
    f_add_space_in_empty_tag = true;
    f_convert_entities = true;
    f_ins_del_extension = false;
    f_line_feed = "\n";
}


void features::set_compressed()
{
    f_add_document_div = false;
    f_add_classes = false;
    f_add_space_in_empty_tag = false;
    f_convert_entities = true;
    f_line_feed.clear();
}


/** \brief Add a `<div ...>` tag around the whole document.
 *
 * The markdown specs do not add any tag around the whole document but it is
 * generally going to be very useful. If you call this function with true,
 * the output will include a div with a class set to just `"cm"`.
 *
 * \param[in] add  Whether to add `<div class=...>` around the whole document.
 */
void features::set_add_document_div(bool add_document_div)
{
    f_add_document_div = add_document_div;
}


bool features::get_add_document_div() const
{
    return f_add_document_div;
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
void features::set_add_classes(bool add_classes)
{
    f_add_classes = add_classes;
}


bool features::get_add_classes() const
{
    return f_add_classes;
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
void features::set_add_space_in_empty_tag(bool add_space_in_empty_tag)
{
    f_add_space_in_empty_tag = add_space_in_empty_tag;
}


bool features::get_add_space_in_empty_tag() const
{
    return f_add_space_in_empty_tag;
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
void features::set_convert_entities(bool convert_entities)
{
    f_convert_entities = convert_entities;
}


bool features::get_convert_entities() const
{
    return f_convert_entities;
}


void features::set_ins_del_extension(bool ins_del_extension)
{
    f_ins_del_extension = ins_del_extension;
}


bool features::get_ins_del_extension() const
{
    return f_ins_del_extension;
}


void features::set_remove_unknown_references(bool remove)
{
    f_remove_unknown_references = remove;
}


bool features::get_remove_unknown_references() const
{
    return f_remove_unknown_references;
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
void features::set_line_feed(std::string const & line_feed)
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
std::string const & features::get_line_feed() const
{
    return f_line_feed;
}



} // namespace cm
// vim: ts=4 sw=4 et
