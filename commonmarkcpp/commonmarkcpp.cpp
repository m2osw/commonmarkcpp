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
 * \brief Implementation of the CommonMark specification in C++.
 *
 * This file is the implementation of the commonmarkcpp class which allows
 * one to convert an input string in HTML.
 */

// self
//
#include    "commonmarkcpp/commonmarkcpp.h"


// C++ lib
//
#include    <algorithm>
#include    <cstring>


// last include
//
#include    <snapdev/poison.h>



namespace cm
{




/** \brief Setup the UTF-8 iterator to go through the input string data.
 *
 * The constructor initializes the UTF-8 iterator which we use to read
 * the characters as char32_t values. We then reconvert them to UTF-8
 * on output. This process allows us to at least fix any invalid
 * UTF-8 characters (later we also want to look at making sure it
 * is also canonicalized).
 */
commonmarkcpp::commonmarkcpp()
    : f_iterator(f_input)
{
}


/** \brief Add some input to the parser.
 *
 * This function can be used to add input data to the CommonMark parser.
 * The output can be retrieved with the get_output() function. In some
 * cases, the output is not immediately available. To flush the input,
 * make sure to call the flush() function once before the get_output().
 *
 * \param[in] input  The UTF-8 input string to convert to HTML.
 */
void commonmarkcpp::add_input(std::string const & input)
{
    // don't waste any time if the input string is empty
    //
    if(input.empty())
    {
        return;
    }

    f_input += input;
}


/** \brief Flush the stream.
 *
 * This function forces all remaining input to be passed out as required.
 */
void commonmarkcpp::flush()
{
}


/** \brief Retrieve the currently available output.
 *
 * This function returns the currently available output. As you call the
 * add_input() function, output becomes available. Output which gets sent
 * to you via this function.
 *
 * The function clears the output buffer on each call.
 *
 * \return A string with the currently available output.
 */
std::string commonmarkcpp::get_output()
{
    std::string const result(f_output);

    f_output.clear();

    return result;
}


char32_t commonmarkcpp::getc()
{
    if(f_pos >= f_input.size())
    {
        f_pos = 0;
        f_input.clear();
        f_last_char = U'\0';
    }
    else
    {
    }

    return f_last_char;
}



} // namespace cm
// vim: ts=4 sw=4 et
