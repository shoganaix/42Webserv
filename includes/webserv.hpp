/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: macastro <macastro@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/10 20:21:23 by msoriano          #+#    #+#             */
/*   Updated: 2026/04/12 18:28:55 by macastro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include "colors.hpp"
#include "httpRequest.hpp"
#include "httpResponse.hpp"

#include <exception>

#include <iostream>  // std::cout, std::cerr
#include <string>    // std::string
#include <vector>    // std::vector (for Configs & pollfds)
#include <map>       // std::map (for Locations & Error Pages)
#include <algorithm> // std::find, std::sort
#include <fstream>   // std::ifstream (reading files from config & root)
#include <sstream>   // std::stringstream (parsing headers)
#include <stdexcept> // td::runtime_error, std::exception

#include <unistd.h>   // close(), read(), write(), fork(), pipe()
#include <fcntl.h>    // fcntl(), O_NONBLOCK, F_SETFL (flags and flag utilities)
#include <sys/wait.h> // waitpid()
#include <sys/stat.h> // stat() (retrieves file info; whether a path exists or directory)
#include <signal.h>   // signal() (ignore SIGPIPE without server dying)

#include <cstring> // memset(), strerror()
#include <cstdlib> // atoi(), exit(), getenv()
#include <cstdio>  // perror() (prints system error message)
#include <cctype>  // std::isdigit(), std::isspace()

#include <netdb.h> // getaddrinfo(), freeaddrinfo() (Translates hostnames and services info into socket address structures)
#include <dirent.h> // opendir(), readdir()

#include <sys/socket.h> // socket(), bind(), listen(), accept(), etc.
#include <netinet/in.h> // struct sockaddr_in (IPv4 socket address structure & utilities)
#include <arpa/inet.h>  // inet_addr(), htons() (IP address conversion utilities)
#include <sys/types.h>  // basic data types for sockets
#include <sys/epoll.h>  // epoll_create1(), epoll_ctl(), epoll_wait()

#define MAX_BODY_SIZE_DFLT 0

class CgiHandler;

struct CgiContext
{
    // Context to manage CGI execution state, including pipes and process ID
    int clientFd; // Client socket file descriptor associated with this CGI execution
    int inFd;     // Pipe to send request body to CGI (the CGI reads from this fd as its stdin)
    int outFd;    // Pipe to read the response from CGI (what the CGI writes to its stdout)
    std::string writeBuffer;
    size_t bytesWritten;
    bool inputFinished;
    bool inputRegistered;
    std::string rawResponse;
    pid_t pid;

    CgiContext()
        : clientFd(-1), inFd(-1), outFd(-1), bytesWritten(0), inputFinished(false),
          inputRegistered(false), pid(-1)
    {
    }
};

struct Config
{
    // In C++98, if you don't define a constructor, those fields can remain undefined
    Config()
        : port(8080), max_size(0), client_max_body_size(MAX_BODY_SIZE_DFLT), host("0.0.0.0"),
          root(""), index("index.html"), server_name("")
    {
    }

    int port;
    int max_size;
    size_t client_max_body_size;
    std::string host;
    std::string root;
    std::string index;
    std::string server_name;
    std::vector<Location> locations;
    std::map<int, std::string> error_pages;
};

struct ClientState
{
    int fd;
    Config config;           // La configuración que le toca
    std::string readBuffer;  // Lo que vamos recibiendo (por si llega por trozos)
    std::string writeBuffer; // Lo que tenemos pendiente de enviar
    bool isRequestFinished;  // <-- Añade esto para saber cuándo parar de leer
    HttpRequest request;     // <-- Donde guardarás los datos parseados
    size_t bytesSent;
    bool headersLogged;
    size_t lastBodyLogCheckpoint;
    bool requestMetaParsed;
    size_t requestHeadersEnd;
    std::string requestMethod;
    std::string requestPath;
    std::string requestQuery;
    std::string requestVersion;
    std::map<std::string, std::string> requestHeaders;
    bool requestHasContentLength;
    size_t requestBodyLength;
    bool requestIsChunked;
    bool cgiStreaming;
    size_t cgiReceivedBody;
    CgiContext* cgiCtx;

    ClientState()
        : fd(-1), isRequestFinished(false), headersLogged(false), lastBodyLogCheckpoint(0),
          requestMetaParsed(false), requestHeadersEnd(0), requestHasContentLength(false),
          requestBodyLength(0), requestIsChunked(false), cgiStreaming(false), cgiReceivedBody(0),
          cgiCtx(NULL)
    {
    }

    void resetRequestCache()
    {
        requestMetaParsed = false;
        requestHeadersEnd = 0;
        requestMethod.clear();
        requestPath.clear();
        requestQuery.clear();
        requestVersion.clear();
        requestHeaders.clear();
        requestHasContentLength = false;
        requestBodyLength = 0;
        requestIsChunked = false;
    }

    void resetCgiStreamState()
    {
        cgiStreaming = false;
        cgiReceivedBody = 0;
        cgiCtx = NULL;
    }
};

class Webserv
{
    int epollFd;
    std::vector<int> fds;
    std::vector<Config> config;
    std::map<int, Config> fdToConfig;   // Mapea socket_escucha -> Config
    std::map<int, ClientState> clients; // Centraliza los clientes
    std::map<int, CgiContext*> _cgiFds;
    CgiHandler* _cgiHandler;

  public:
    Webserv(const std::string& configFile);
    ~Webserv() {};
    void run();

  private:
    void setSockets();
    bool isListeningFd(int fd);
    void acceptNewConnection(int fd);
    void destroyCgiContext(CgiContext* ctx, bool killProcess);
    void finalizeCgiResponse(CgiContext* ctx, int fd);
    void handleCgiEvent(int fd, uint32_t events);
    void handleClientData(int fd);
    void handleClientWrite(int fd);
    void closeConnection(int fd);
    HttpResponse routeRequest(const HttpRequest& req, const Config& server);
};

#endif
