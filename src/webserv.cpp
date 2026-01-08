/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: angnavar <angnavar@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/08 18:51:13 by angnavar          #+#    #+#             */
/*   Updated: 2026/01/08 19:00:14 by angnavar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"

Webserv::Webserv(const std::string &configFile)
{
	std::cout << "Webserv initialized with config: " << configFile << std::endl;
}

void Webserv::run()
{
	std::cout << "Webserv running..." << std::endl;
	while(true)
	{

	}
}
