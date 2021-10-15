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
 * \brief Declaration of the commonmarkcpp class.
 *
 * The commonmarkcpp is a state machine that accepts Markdown data as input
 * and spits out the corresponding HTML.
 */


// self
//


// libutf8 lib
//
#include    <libutf8/iterator.h>


// C++ lib
//
#include    <memory>
#include    <string>



namespace cm
{



class commonmarkcpp
{
public:
    typedef std::shared_ptr<commonmarkcpp>
                            pointer_t;

                            commonmarkcpp();

    void                    add_input(std::string const & input);
    void                    flush();
    std::string             get_output();

private:
    char32_t                getc();

    std::string             f_input = std::string();
    libutf8::utf8_iterator  f_iterator;  // must be defined in constructors
    int                     f_unget_pos = 0;
    char32_t                f_unget[2] = {};
    char32_t                f_last_char = U'\0';

    bool                    f_flushed = false;

    std::string             f_output = std::string();
};



} // namespace cm
// vim: ts=4 sw=4 et
