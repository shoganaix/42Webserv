/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/08 18:51:13 by angnavar          #+#    #+#             */
/*   Updated: 2026/02/11 20:36:25 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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
