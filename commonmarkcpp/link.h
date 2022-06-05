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
 * \brief Declaration of the link class.
 *
 * The commonmark manages a set of links found in the document. Some links
 * are references, those can be referenced within the markdown code.
 *
 * The links can be checked later since they are made available from the
 * commonmark object. You could, for example, have a utility which verifies
 * that the links are valid.
 */


// self
//
#include    "commonmarkcpp/character.h"


// libutf8 lib
//
#include    <libutf8/caseinsensitivestring.h>


// C++ lib
//
#include    <functional>
#include    <map>
#include    <memory>



namespace cm
{



class uri
{
public:
    typedef std::vector<uri>
                            vector_t;

    void                    mark_as_reference();
    bool                    is_reference() const;

    void                    destination(std::string const & d);
    std::string const &     destination() const;

    void                    title(std::string const & t);
    std::string const &     title() const;

private:
    std::string             f_destination = std::string();  // URI
    std::string             f_title = std::string();        // title="..."
    bool                    f_reference = false;
};


class link
{
public:
    typedef std::shared_ptr<link>
                            pointer_t;

    typedef std::map<libutf8::case_insensitive_string, pointer_t>
                            map_t;

    typedef std::function<pointer_t (std::string const & name)>
                            find_link_reference_t;

                            link(std::string const & name);

    std::string const &     name() const;

    std::size_t             uri_count() const;
    uri const &             uri_details(int idx) const;
    void                    add_uri(uri const & u);

private:
    std::string const       f_name;                 // label/alt
    uri::vector_t           f_uris = uri::vector_t();
};



} // namespace cm
// vim: ts=4 sw=4 et
