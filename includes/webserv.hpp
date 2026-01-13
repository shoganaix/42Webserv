/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kpineda- <kpineda-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/10 20:21:23 by msoriano          #+#    #+#             */
/*   Updated: 2026/01/13 18:28:15 by kpineda-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <exception>
#include <iostream>
#include "colors.hpp"

struct location
{
    
};

struct Config
{
    int port;
    int max_size;
    std::string host;
    std::string server_name;
    std::string root;
    std::string index;
    std::string error_page;
    location *locations;
};

class Webserv
{
    Config config;

public:

    Webserv(const std::string &configFile);
    ~Webserv() {};

    void run();

    
};

#endif
