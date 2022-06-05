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
 * \brief Implementation of the link class.
 *
 * This file is the implementation of the link class allowing one to
 * hold the data of one link.
 *
 * Links are composed of a name, a label (also called anchor text), and a
 * destination (the URI).
 *
 * The name of a link is also its reference. However, a link will be marked
 * as being a reference or not. If not, then that link was found inline.
 */

// self
//
#include    "commonmarkcpp/link.h"


// snapdev lib
//
//#include    <snapdev/trim_string.h>


// C++ lib
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace cm
{



void uri::mark_as_reference()
{
    f_reference = true;
}


bool uri::is_reference() const
{
    return f_reference;
}


void uri::destination(std::string const & d)
{
    f_destination = d;
}


std::string const & uri::destination() const
{
    return f_destination;
}


void uri::title(std::string const & t)
{
    f_title = t;
}


std::string const & uri::title() const
{
    return f_title;
}





/** \brief Initialize the link.
 *
 * The constructor creates a link with the given name.
 *
 * \param[in] name  The name of this link.
 */
link::link(std::string const & name)
    : f_name(name)
{
}


std::string const & link::name() const
{
    return f_name;
}


std::size_t link::uri_count() const
{
    return f_uris.size();
}


uri const & link::uri_details(int idx) const
{
    if(static_cast<std::size_t>(idx) >= f_uris.size())
    {
        throw commonmark_out_of_range("index out of range to retrieve URI");
    }

    return f_uris[idx];
}


void link::add_uri(uri const & u)
{
    if(f_uris.size() > 0
    && f_uris[0].is_reference())
    {
        f_uris.push_back(u);
    }
    else
    {
        f_uris.insert(f_uris.begin(), u);
    }
}



} // namespace cm
// vim: ts=4 sw=4 et

