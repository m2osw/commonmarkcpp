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
 * \brief Read the entities.json and convert them to a C++ table.
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


// libutf8 lib
//
#include    <libutf8/json_tokens.h>
#include    <libutf8/libutf8.h>


// snapdev lib
//
#include    <snapdev/file_contents.h>
#include    <snapdev/not_reached.h>
#include    <snapdev/pathinfo.h>


// eventdispatcher lib
//
#include    <eventdispatcher/signal_handler.h>


// boost lib
//
#include    <boost/preprocessor/stringize.hpp>


// C++ lib
//
#include    <iomanip>


// last include
//
#include    <snapdev/poison.h>



namespace
{


const advgetopt::option g_options[] =
{
    advgetopt::define_option(
          advgetopt::Name("verbose")
        , advgetopt::ShortName('v')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("display various verbose messages.")
    ),
    advgetopt::define_option(
          advgetopt::Name("output")
        , advgetopt::ShortName('o')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("display various verbose messages.")
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





class entity_t
{
public:
    typedef std::shared_ptr<entity_t>   pointer_t;
    typedef std::vector<pointer_t>      vector_t;

                                    entity_t(std::string const & name);

    std::string                     get_name() const;
    void                            add_code(int code);
    std::string                     get_codes() const;

private:
    std::string                     f_name = std::string();
    std::string                     f_codes = std::string();
};


class entities
{
public:
                                    entities(int argc, char * argv[]);

    int                             run();

private:
    int                             read();
    int                             parse();
    int                             output_table();

    advgetopt::getopt               f_opt;
    std::string                     f_json_entities = std::string();
    std::string                     f_output_filename = std::string();
    entity_t::vector_t              f_entities = entity_t::vector_t();
    bool                            f_verbose = false;
};



entity_t::entity_t(std::string const & name)
    : f_name(name)
{
}


std::string entity_t::get_name() const
{
    return f_name;
}


void entity_t::add_code(int code)
{
    f_codes += libutf8::to_u8string(static_cast<char32_t>(code));
}


std::string entity_t::get_codes() const
{
    return f_codes;
}







entities::entities(int argc, char * argv[])
    : f_opt(g_options_environment)
{
    f_opt.finish_parsing(argc, argv);

    f_verbose = f_opt.is_defined("verbose");
    if(f_opt.is_defined("output"))
    {
        f_output_filename = f_opt.get_string("output");
    }
    if(f_output_filename.empty())
    {
        std::cerr << "error: output filename is required.\n";
        exit(1);
    }
}


int entities::run()
{
    int r(read());
    if(r != 0)
    {
        return r;
    }

    r = parse();
    if(r != 0)
    {
        return r;
    }

    r = output_table();
    if(r != 0)
    {
        return r;
    }

    return 0;
}


int entities::read()
{
    std::size_t const max(f_opt.size("filenames"));
    for(std::size_t idx(0); idx < max; ++idx)
    {
        std::string const filename(f_opt.get_string("filenames", idx));
        snap::file_contents input(filename);
        if(!input.read_all())
        {
            std::cerr << "error: could not read \""
                << filename
                << "\".\n";
            return 1;
        }
        f_json_entities += input.contents();
    }

    if(f_json_entities.empty())
    {
        std::cerr << "error: the JSON is not expected to be empty.\n";
        return 1;
    }

    return 0;
}


int entities::parse()
{
    libutf8::json_tokens json(f_json_entities);

    if(json.next_token() != libutf8::token_t::TOKEN_OPEN_OBJECT)
    {
        std::cerr << "error:"
            << json.line()
            << ": the JSON is expected to be an object of objects.\n";
        return 1;
    }

    for(;;)
    {
        // one line is a lable and an object with codepoints and characters
        // "&AElig": { "codepoints": [198], "characters": "\u00C6" },
        //
        if(json.next_token() != libutf8::token_t::TOKEN_STRING)
        {
            std::cerr << "error:"
                << json.line()
                << ": expected a string with the name of an entity.\n";
            return 1;
        }

        if(json.string().empty())
        {
            std::cerr << "error: the name of an entity cannot be empty.\n";
            return 1;
        }
        entity_t::pointer_t e(std::make_shared<entity_t>(json.string()));

        if(json.next_token() != libutf8::token_t::TOKEN_COLON)
        {
            std::cerr << "error:"
                << json.line()
                << ": expected a ':' character after the entity name.\n";
            return 1;
        }

        if(json.next_token() != libutf8::token_t::TOKEN_OPEN_OBJECT)
        {
            std::cerr << "error:"
                << json.line()
                << ": expected a sub-object definition for this entity.\n";
            return 1;
        }

        for(;;)
        {
            if(json.next_token() != libutf8::token_t::TOKEN_STRING)
            {
                std::cerr << "error:"
                    << json.line()
                    << ": the sub-object is expected to be composed of fields.\n";
                return 1;
            }

            bool const codepoints(json.string() == "codepoints");
            bool const characters(json.string() == "characters");

            if(!codepoints
            && !characters)
            {
                std::cerr << "error:"
                    << json.line()
                    << ": unexpected sub-object field name \""
                    << json.string()
                    << "\".\n";
                return 1;
            }

            if(json.next_token() != libutf8::token_t::TOKEN_COLON)
            {
                std::cerr << "error:"
                    << json.line()
                    << ": sub-object name is expected to be followed by a colon.\n";
                return 1;
            }

            if(codepoints)
            {
                if(json.next_token() != libutf8::token_t::TOKEN_OPEN_ARRAY)
                {
                    std::cerr << "error:"
                        << json.line()
                        << ": codepoints are expected to be defined in an array.\n";
                    return 1;
                }

                for(;;)
                {
                    if(json.next_token() != libutf8::token_t::TOKEN_NUMBER)
                    {
                        std::cerr << "error:"
                            << json.line()
                            << ": the codepoints array is expected to be composed of numbers.\n";
                        return 1;
                    }

                    e->add_code(json.number());

                    auto const sep(json.next_token());
                    if(sep == libutf8::token_t::TOKEN_CLOSE_ARRAY)
                    {
                        break;
                    }

                    if(sep != libutf8::token_t::TOKEN_COMMA)
                    {
                        std::cerr << "error:"
                            << json.line()
                            << ": the codepoint numbers are expected to be separated by commas.\n";
                        return 1;
                    }
                }
            }
            else
            {
                // if not "codepoints":, then "characters":, that's it for now
                //
                if(json.next_token() != libutf8::token_t::TOKEN_STRING)
                {
                    std::cerr << "error:"
                        << json.line()
                        << ": the characters field is expected to be a string.\n";
                    return 1;
                }
                // ignore this string in our processes
            }

            auto const more(json.next_token());
            if(more == libutf8::token_t::TOKEN_CLOSE_OBJECT)
            {
                // we reached the end
                //
                break;
            }

            if(more != libutf8::token_t::TOKEN_COMMA)
            {
                std::cerr << "error:"
                    << json.line()
                    << ": each entity is expected to be separated by commas.\n";
                return 1;
            }
        }

        if(e->get_name().back() == ';')
        {
            f_entities.push_back(e);
        }
        else if(f_verbose)
        {
            std::cerr << "info: entity definition \""
                << e->get_name()
                << "\" is missing the ';', ignoring.\n";
        }

        auto const n(json.next_token());
        if(n == libutf8::token_t::TOKEN_CLOSE_OBJECT)
        {
            // we reached the end
            //
            if(f_verbose)
            {
                std::cerr << "info: found "
                    << f_entities.size()
                    << " entities.\n";
            }
            return 0;
        }

        if(n != libutf8::token_t::TOKEN_COMMA)
        {
            std::cerr << "error:"
                << json.line()
                << ": expected a comma between sub-objects.\n";
            return 1;
        }
    }
}


int entities::output_table()
{
    std::sort(
          f_entities.begin()
        , f_entities.end()
        , [](auto const & a, auto const & b)
        {
            return a->get_name() < b->get_name();
        });

    std::string header_filename(snap::pathinfo::replace_suffix(f_output_filename, ".cpp", ".h"));
    std::ofstream hdr(header_filename);

    hdr << "#include <cstddef>\n"
        << "struct entity_t {\n"
        << "char const * const f_name;\n"
        << "char const * const f_codes;\n"
        << "};\n"
        << "constexpr std::size_t const ENTITY_COUNT = "
                                << f_entities.size() << ";\n";

    std::ofstream out(f_output_filename);

    out << std::hex;

    out << "#include \"" << header_filename << "\"\n"
        << "constexpr entity_t g_entities[] = {\n";
    for(auto e : f_entities)
    {
        std::string name(e->get_name());
        out << "{ \""
            << name.substr(1, name.length() - 2)
            << "\", \"";

        std::string codes(e->get_codes());
        std::size_t const max(codes.length());
        for(std::size_t idx(0); idx < max; ++idx)
        {
            out << "\\x"
                << std::setw(2)
                << std::setfill('0')
                << static_cast<int>(static_cast<std::uint8_t>(codes[idx]));
        }

        out << "\" },\n";
    }
    out << "};\n";

    return 0;
}





int main(int argc, char * argv[])
{
    ed::signal_handler::create_instance();

    try
    {
        entities ge(argc, argv);
        return ge.run();
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
    snap::NOT_REACHED();
}


// vim: ts=4 sw=4 et
