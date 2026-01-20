/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: angnavar <angnavar@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/10 20:21:23 by msoriano          #+#    #+#             */
/*   Updated: 2026/01/20 16:01:25 by angnavar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <exception>
#include <iostream>
#include "colors.hpp"
#include <map>
#include <vector>

struct Location
{
    bool autoindex;
    std::string path;
    std::string root;
    std::string index;
    std::string redir;
    std::string upload_path;
    std::vector<std::string> allow_methods;
    std::map<std::string, std::string> cgi_needs;
};

struct Config
{
    int port;
    int max_size;
	long client_max_body_size;
    std::string host;
    std::string root;
    std::string index;
    std::string server_name;
	std::vector<Location> locations;
    std::map<int, std::string> error_pages;
};

class Webserv
{
    std::vector<Config> config;
	std::vector<int> fds;

public:

    Webserv(const std::string &configFile);
    ~Webserv() {};

    void run();

    
};

#endif
