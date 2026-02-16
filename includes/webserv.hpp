/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: angnavar <angnavar@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/10 20:21:23 by msoriano          #+#    #+#             */
/*   Updated: 2026/02/16 22:59:27 by angnavar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include "colors.hpp"
#include <exception>

#include <iostream>      // std::cout, std::cerr
#include <string>        // std::string
#include <vector>        // std::vector (para tus Configs y pollfds)
#include <map>           // std::map (para las Locations y Error Pages)
#include <algorithm>     // std::find, std::sort
#include <fstream>       // std::ifstream (para leer archivos de config y el root)
#include <sstream>       // std::stringstream (útil para parsear headers y números)

#include <unistd.h>      // close(), read(), write(), fork(), pipe()
#include <fcntl.h>       // fcntl(), O_NONBLOCK, F_SETFL
#include <sys/wait.h>    // waitpid() (necesario para limpiar procesos CGI)
#include <sys/stat.h>    // stat() (para saber si un archivo existe o es un directorio)
#include <signal.h>      // signal() (para ignorar SIGPIPE y que el server no muera)

#include <cstring>       // memset(), strerror()
#include <cstdlib>       // atoi(), exit(), getenv()
#include <cstdio>        // perror()

#include <sys/socket.h>  // socket(), bind(), listen(), accept(), etc.
#include <netinet/in.h>  // struct sockaddr_in
#include <arpa/inet.h>   // inet_addr(), htons()
#include <sys/types.h>   // Tipos de datos básicos para sockets
#include <sys/epoll.h>   // epoll_create1(), epoll_ctl(), epoll_wait()


struct Location
{
    //In C++98, if you don't define a constructor, those fields can remain undefined
    // (Memory garbage → random errors)
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
