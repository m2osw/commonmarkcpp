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
 * \brief Declaration of the features class.
 *
 * The markdown parser handles many different cases and some are features
 * that are not compatible with the commonmark specification. Also we
 * wanted to support various extensions such as having tables and class
 * attributes.
 *
 * This class allows us to define all of these features in one place
 * which we can share with all our subclasses as required.
 */


// C++ lib
//
#include    <memory>
#include    <string>



namespace cm
{



class features
{
public:
    typedef std::shared_ptr<features>      pointer_t;

    void                    set_commonmark_compatible();
    void                    set_compressed();

    void                    set_add_document_div(bool add_document_div = true);
    bool                    get_add_document_div() const;

    void                    set_add_classes(bool add_classes = true);
    bool                    get_add_classes() const;

    void                    set_add_space_in_empty_tag(bool add_space_in_empty_tag = true);
    bool                    get_add_space_in_empty_tag() const;

    void                    set_convert_entities(bool convert_entities = true);
    bool                    get_convert_entities() const;

    void                    set_ins_del_extension(bool ins_del_extension);
    bool                    get_ins_del_extension() const;

    void                    set_remove_unknown_references(bool remove);
    bool                    get_remove_unknown_references() const;

    void                    set_line_feed(std::string const & line_feed);
    std::string const &     get_line_feed() const;

private:
    bool                    f_add_document_div = false;
    bool                    f_add_classes = false;
    bool                    f_add_space_in_empty_tag = false;
    bool                    f_convert_entities = true;
    bool                    f_ins_del_extension = true;
    bool                    f_remove_unknown_references = true;
    std::string             f_line_feed = std::string();
};



} // namespace cm
// vim: ts=4 sw=4 et
