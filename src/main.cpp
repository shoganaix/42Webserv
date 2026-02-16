/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/10 20:21:26 by msoriano          #+#    #+#             */
/*   Updated: 2026/02/16 13:40:02 by usuario          ###   ########.fr       */
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

#ifdef DEBUG
# include "../includes/debug.hpp"
#endif


//-----------------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	if (argc != 1 && argc != 2) 
    {
        Logger::logMsg(RED, CONSOLE_OUTPUT, "ERROR:\n- ./webserv\n- ./webserv <config_file>\n");
        return (1);
    }
    const std::string configFile = (argc == 1 ? "configs/default.conf" : argv[1]);
	try 
	{
        ConfigParser parser;
        std::vector<Config> cfgs = parser.parse(configFile);
        //--------------------------DEBUG PARSER------------------------
        #ifdef DEBUG
            printAllConfigs(cfgs);
            //debugTestLocationMatching(cfgs);
            //debugTestPathResolution(cfgs);
            debugTestRoutingAndResolution(cfgs);
        #endif
        //--------------------------------------------------------------
        Webserv server(configFile);
        server.run();
        return (0);
    }
    catch (const std::exception& e)
    {
        std::cerr << RED << "Error: " << RESET << e.what() << "\n";
         return (1);
    }
}