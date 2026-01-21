/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/10 20:21:26 by msoriano          #+#    #+#             */
/*   Updated: 2026/01/21 01:29:50 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/webserv.hpp"      
#include "../includes/logger.hpp"       
#include "../includes/colors.hpp"       
#include "../includes/configParser.hpp" 

// TESTING CONF PARSER (DELETE LATER)
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
static void printLocation(const Location& loc, size_t index)
{
    std::cout << "\n--- Location " << index << " ---\n";
    std::cout << "Path: " << loc.path << std::endl;
    std::cout << "Root: " << loc.root << std::endl;
    std::cout << "Index: " << loc.index << std::endl;
    std::cout << "Autoindex: " << (loc.autoindex ? "on" : "off") << std::endl;
    std::cout << "Redirection: " << loc.redir << std::endl;
    std::cout << "Upload path: " << loc.upload_path << std::endl;

    std::cout << "Allowed methods: ";
    for (size_t i = 0; i < loc.allow_methods.size(); i++)
        std::cout << loc.allow_methods[i] << " ";
    std::cout << std::endl;

    std::cout << "CGI handlers:\n";
    for (std::map<std::string, std::string>::const_iterator it = loc.cgi_needs.begin();
         it != loc.cgi_needs.end(); ++it)
    {
        std::cout << "  " << it->first << " -> " << it->second << std::endl;
    }
}
void printConfig(const Config& cfg)
{
    std::cout << "\n========== SERVER CONFIG ==========\n";
    std::cout << "Server name: " << cfg.server_name << std::endl;
    std::cout << "Host: " << cfg.host << std::endl;
    std::cout << "Port: " << cfg.port << std::endl;
    std::cout << "Root: " << cfg.root << std::endl;
    std::cout << "Index: " << cfg.index << std::endl;
    std::cout << "Client max body size: " << cfg.client_max_body_size << std::endl;

    std::cout << "\nError pages:\n";
    for (std::map<int, std::string>::const_iterator it = cfg.error_pages.begin();
         it != cfg.error_pages.end(); ++it)
    {
        std::cout << "  " << it->first << " -> " << it->second << std::endl;
    }

    std::cout << "\nLocations (" << cfg.locations.size() << "):\n";
    for (size_t i = 0; i < cfg.locations.size(); i++)
        printLocation(cfg.locations[i], i);

    std::cout << "===================================\n";
}
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	if (argc == 1 || argc == 2) 
    {
		try 
		{
			std::string		configFile;
			configFile = (argc == 1 ? "configs/default.conf" : argv[1]);
            Webserv server(configFile);

			//TESTING CONF FILE PARSER
			//-----------------------------------------------------------------------------------------
			ConfigParser parser;
    		Config cfg = parser.parse(configFile);
			printConfig(cfg);
			//-----------------------------------------------------------------------------------------

            server.run();	
		}
		catch (std::exception &e) 
        {
			std::cerr << RED << "[EXCEPTION]: " << e.what() << std::endl;
			return (1);
		}
    }
    else 
	{
        Logger::logMsg(RED, CONSOLE_OUTPUT, "ERROR: \n- ./webserv \n- ./webserv <config_file> \n");
		return (1);
	}
    return (0);
}