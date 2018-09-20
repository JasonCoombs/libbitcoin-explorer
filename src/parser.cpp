/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/explorer/parser.hpp>

#include <iostream>
#include <string>
#include <boost/program_options.hpp>
#include <bitcoin/explorer/command.hpp>
#include <bitcoin/explorer/define.hpp>
#include <bitcoin/bitcoin.hpp>

using namespace boost::filesystem;
using namespace boost::program_options;
using namespace boost::system;

namespace libbitcoin {
namespace explorer {

parser::parser(command& instance)
  : help_(false), instance_(instance)
{
}

bool parser::help() const
{
    return help_;
}

options_metadata parser::load_options()
{
    return instance_.load_options();
}

arguments_metadata parser::load_arguments()
{
    return instance_.load_arguments();
}

options_metadata parser::load_settings()
{
    options_metadata settings("settings");
    instance_.load_settings(settings);
    return settings;
}

options_metadata parser::load_environment()
{
    options_metadata environment("environment");
    instance_.load_environment(environment);
    return environment;
}

bool parser::parse(std::string& out_error, std::istream& input,
    int argc, const char* argv[])
{
    try
    {
        variables_map variables;

        // Must store before environment in order for commands to supercede.
//        load_command_variables(variables, input, argc, argv);
// note: this previously called config::parser::load_command_variables
        options_metadata description("options");
        description.add_options()
        (
         "version,v",
         value<bool>(&version_)->
         default_value(false)->zero_tokens(),
         "Display version information."
         );

        const auto arguments = load_arguments();
        
        // command_line_parser documentation:
        // https://www.boost.org/doc/libs/1_68_0/doc/html/program_options/tutorial.html
        auto command_parser = command_line_parser(argc, argv).options(description)
        /*.allow_unregistered()*/.positional(arguments);
        
        // Boost.ProgramOptions explained:
        // https://theboostcpplibraries.com/boost.program_options
        boost::program_options::store(command_parser.run(), variables);
        
        notify(variables);
        
        // Don't load rest if help is specified.
        // For variable with stdin or file fallback load the input stream.
        if (!get_option(variables, BX_HELP_VARIABLE))
            instance_.load_fallbacks(input, variables);

        // Don't load rest if help is specified.
        if (!get_option(variables, BX_HELP_VARIABLE))
        {
            // Must store before configuration in order to specify the path.
            load_environment_variables(variables,
                BX_ENVIRONMENT_VARIABLE_PREFIX, &description);

            // Is lowest priority, which will cause confusion if there is
            // composition between them, which therefore should be avoided.
            /* auto file = */ load_configuration_variables(variables,
                BX_CONFIG_VARIABLE, &description);

            // Set variable defaults, send notifications and update bound vars.
            notify(variables);

            // Set the instance defaults from config values.
            instance_.set_defaults_from_config(variables);
        }
        else
        {
            help_ = true;
        }
    }
    catch (const po::error& e)
    {
        // This is obtained from boost, which circumvents our localization.
        out_error = e.what();
        return false;
    }

    return true;
}

} // namespace explorer
} // namespace libbitcoin
