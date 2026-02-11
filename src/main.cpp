/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/10 20:21:26 by msoriano          #+#    #+#             */
/*   Updated: 2026/02/11 21:14:40 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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
    //--------------------------DEBUG PARSER------------------------
	try 
	{
        #ifdef DEBUG
            ConfigParser parser;
            std::vector<Config> cfgs = parser.parse(configFile);
            printAllConfigs(cfgs);
            return (0);
        #else
            Webserv server(configFile);
            server.run();
            return (0);
        #endif
    }
    catch (const std::exception& e)
    {
        std::cerr << RED << "Error: " << RESET << e.what() << "\n";
         return (1);
    }
}