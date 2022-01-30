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
 * \brief Implementation of the markdown command line tool.
 *
 * This file represents an example of use of the commonmarkcpp library as
 * a command line tool. Although the library is meant to be used in your
 * HTML servers, this is useful to quickly test validity of various
 * syntactical data without having to run a heavy server.
 */


// commonmarkcpp lib
//
#include    "commonmarkcpp/commonmark.h"

#include    "commonmarkcpp/version.h"


// getopt lib
//
#include    <advgetopt/advgetopt.h>
#include    <advgetopt/exception.h>


// snapdev lib
//
#include    <snapdev/not_reached.h>


// eventdispatcher lib
//
#include    <eventdispatcher/signal_handler.h>


// boost lib
//
#include    <boost/preprocessor/stringize.hpp>


// last include
//
#include    <snapdev/poison.h>



namespace
{


const advgetopt::option g_options[] =
{
    advgetopt::define_option(
          advgetopt::Name("extensions")
        , advgetopt::ShortName('x')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("allow our markdown extensions.")
    ),
    advgetopt::define_option(
          advgetopt::Name("filenames")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_DEFAULT_OPTION
            , advgetopt::GETOPT_FLAG_MULTIPLE
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("list of input files to convert to HTML.")
    ),
    advgetopt::end_options()
};

advgetopt::group_description const g_group_descriptions[] =
{
    advgetopt::define_group(
          advgetopt::GroupNumber(advgetopt::GETOPT_FLAG_GROUP_COMMANDS)
        , advgetopt::GroupName("command")
        , advgetopt::GroupDescription("Commands:")
    ),
    advgetopt::define_group(
          advgetopt::GroupNumber(advgetopt::GETOPT_FLAG_GROUP_OPTIONS)
        , advgetopt::GroupName("option")
        , advgetopt::GroupDescription("Options:")
    ),
    advgetopt::end_groups()
};

constexpr char const * const g_configuration_files[] =
{
    "/etc/commonmarkcpp/commonmarkcpp.conf",
    nullptr
};

// TODO: once we have stdc++20, remove all defaults & pragma
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "commonmarkcpp",
    .f_group_name = "commonmarkcpp",
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "COMMONMARKCPP",
    .f_section_variables_name = nullptr,
    .f_configuration_files = g_configuration_files,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>]\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = COMMONMARKCPP_VERSION_STRING,
    .f_license = "GPL v2 or newer",
    .f_copyright = "Copyright (c) 2021-" BOOST_PP_STRINGIZE(UTC_BUILD_YEAR) "  Made to Order Software Corporation",
    .f_build_date = UTC_BUILD_DATE,
    .f_build_time = UTC_BUILD_TIME,
    .f_groups = g_group_descriptions
};
#pragma GCC diagnostic pop

}
// noname namespace






class markdown
{
public:
                                    markdown(int argc, char * argv[]);

    int                             run();

private:
    advgetopt::getopt               f_opt;
};




markdown::markdown(int argc, char * argv[])
    : f_opt(g_options_environment)
{
    f_opt.finish_parsing(argc, argv);
}


int markdown::run()
{
    return 0;
}





int main(int argc, char * argv[])
{
    ed::signal_handler::create_instance();

    try
    {
        markdown md(argc, argv);
        return md.run();
    }
    catch(advgetopt::getopt_exit const & e)
    {
        exit(e.code());
    }
    catch(std::exception const & e)
    {
        std::cerr << "error: an exception occurred (1): " << e.what() << std::endl;
        exit(1);
    }
    catch(...)
    {
        std::cerr << "error: an unknown exception occurred (2)." << std::endl;
        exit(2);
    }
    snapdev::NOT_REACHED();
}


// vim: ts=4 sw=4 et
