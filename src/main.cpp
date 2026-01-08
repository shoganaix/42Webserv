/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: angnavar <angnavar@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/10 20:21:26 by msoriano          #+#    #+#             */
/*   Updated: 2026/01/08 18:57:18 by angnavar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/webserv.hpp"   // 
#include "../includes/logger.hpp"    // 
#include "../includes/colors.hpp"    // 

int main(int argc, char **argv)
{
	if (argc == 1 || argc == 2) 
    {
		try 
		{
			std::string		configFile;
			configFile = (argc == 1 ? "configs/default.conf" : argv[1]);
            Webserv server(configFile);
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