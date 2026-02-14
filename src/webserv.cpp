/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msoriano <msoriano@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/08 18:51:13 by angnavar          #+#    #+#             */
/*   Updated: 2026/02/14 13:44:43 by msoriano         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*-----------------------------------------------------------------------
 *                          🧠WEBSERV BRAIN🧠
 *
 * This class represents the main Webserver engine
 *
 * Responsibilities:
 *  - Load and store parsed configuration
 *  - Initialize listening sockets
 *  - Manage the main event loop
 *  - Handle client connections
 * 	- ...
 *
 * The run() method starts the infinite event loop
 * where the server waits for incoming connections
 * and processes client events.
 * -----------------------------------------------------------------------
 */

#include "webserv.hpp"
#include "configParser.hpp"

/*
 * - Receives path to conf at startup
 * - Loads and parses using ConfigParser
 * - Stores the resulting configurations internally for later use 
 * - ...
*/
Webserv::Webserv(const std::string &configFile)
{
	std::cout << BLUE << "Webserv initialized with config: " << RESET << configFile << std::endl;
	
	ConfigParser parser;
    this->config = parser.parse(configFile);
}


void Webserv::setSockets()
{
	
}


void Webserv::run()
{
	setSockets();
	
	std::cout << GREEN << "Webserv running..." << RESET <<std::endl;
	while(true)
	{
		/*int ret = poll(fds.data(), fds.size(), -1); 
        if (ret < 0) throw std::runtime_error("poll error");

        for (size_t i = 0; i < fds.size(); i++) {
            if (fds[i].revents == 0) continue;
        }*/
	}
}
