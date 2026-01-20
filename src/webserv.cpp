/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kpineda- <kpineda-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/08 18:51:13 by angnavar          #+#    #+#             */
/*   Updated: 2026/01/20 18:08:33 by kpineda-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"

Webserv::Webserv(const std::string &configFile)
{
	std::cout << BLUE << "Webserv initialized with config: " << RESET << configFile << std::endl;
	
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
