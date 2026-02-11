/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/10 20:21:23 by msoriano          #+#    #+#             */
/*   Updated: 2026/02/11 20:19:40 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <exception>
#include <iostream>
#include "colors.hpp"
#include <map>
#include <vector>
#include <sys/socket.h>

struct Location
{
    //In C++98, if you don't define a constructor, those fields can remain undefined
    Location(): autoindex(false), path("/"), root(""), index(""), redir(""), upload_path("")
    {}

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
    //In C++98, if you don't define a constructor, those fields can remain undefined
    Config()
        : port(8080), max_size(0), client_max_body_size(1000000), host("0.0.0.0"),
        root(""), index("index.html"), server_name("")
        {}

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

private:

    void setSockets();
    
};

#endif
