// Copyright (c) 2013-2021  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/commonmark
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
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#pragma once

/** \file
 * \brief Commonmark exceptions.
 *
 * This files declares a few exceptions that the commonmark library
 * uses when a parameter is wrong and the text to HTML conversion fails.
 */


// libexcept lib
//
#include    <libexcept/exception.h>


namespace cm
{



DECLARE_LOGIC_ERROR(commonmark_logic_error);

DECLARE_MAIN_EXCEPTION(commonmark_error);

DECLARE_EXCEPTION(commonmark_error, already_flushed);
//DECLARE_EXCEPTION(commonmark_error, duplicate_error);
//DECLARE_EXCEPTION(commonmark_error, invalid_variable);
//DECLARE_EXCEPTION(commonmark_error, invalid_parameter);
//DECLARE_EXCEPTION(commonmark_error, invalid_severity);
//DECLARE_EXCEPTION(commonmark_error, not_a_message);



} // namespace cm
// vim: ts=4 sw=4 et
