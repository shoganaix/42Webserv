/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: macastro <macastro@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/10 20:21:26 by msoriano          #+#    #+#             */
/*   Updated: 2026/04/11 13:43:01 by macastro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*-----------------------------------------------------------------------
 *                          📂MAIN PROGRAM📂
 *
 *  - Validates []args
 *  - Determines which config file to load
 *  - Parses and validates configuration
 *  - (DEBUG mode)
 *      -> Print parsed configuration
 *      -> ...
 *  - Initializes Webserv engine
 *
 * If any exception occurs during parsing or runtime,
 * it is caught and displayed before exiting safely.
 * -----------------------------------------------------------------------
 */

#include "../includes/webserv.hpp"
#include "../includes/logger.hpp"
#include "../includes/colors.hpp"
#include "../includes/configParser.hpp"

#include "../includes/debug.hpp"
#ifdef DEBUG
#endif

void run_tests()
{
    testlocationMatches();
    testmatchLocation();
    // printAllConfigs(cfgs);
    // // debugTestLocationMatching(cfgs);
    // // debugTestPathResolution(cfgs);
    // debugTestRoutingAndResolution(cfgs);
    // debugTestCgiDetection(cfgs);
    // debugTestCgiEnv(cfgs[0]);
    // debugTestCgiExecution(cfgs[0]);
}

int main(int argc, char** argv)
{
    // run_tests();
    // return (0);

    if (argc != 1 && argc != 2)
    {
        std::cerr << RED << "ERROR:\n- ./webserv\n- ./webserv <config_file>\n" << RESET;
        return (1);
    }
    const std::string configFile = (argc == 1 ? "configs/default.conf" : argv[1]);
    try
    {
        ConfigParser parser;
        std::vector<Config> cfgs = parser.parse(configFile);
        Webserv server(configFile);
        server.run();
        return (0);
    }
    catch (const std::exception& e)
    {
        std::cerr << RED << "Error: " << e.what() << RESET << "\n";
        return (1);
    }
}
