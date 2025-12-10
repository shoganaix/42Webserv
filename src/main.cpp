/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msoriano <msoriano@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/10 20:21:26 by msoriano          #+#    #+#             */
/*   Updated: 2025/12/10 20:26:46 by msoriano         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/webserv.hpp"   // 
#include "../includes/logger.hpp"    // 
#include "../includes/colors.hpp"    // 

int main(int argc, char **argv)
{
	// Logger::setState(OFF);
	if (argc == 1 || argc == 2) 
    {
		try 
		{
            std::string configFile = argv[1];
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