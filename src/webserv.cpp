/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: angnavar <angnavar@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/08 18:51:13 by angnavar          #+#    #+#             */
/*   Updated: 2026/03/01 12:03:35 by angnavar         ###   ########.fr       */
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
#include "validation.hpp"

/*
* - Receives path to conf at startup
* - Loads and parses using ConfigParser
* - Normalizes + validates
* - Stores the resulting configurations internally for later use 
* - ... 
*/
Webserv::Webserv(const std::string &configFile)
{
	std::cout << BLUE << "Webserv initialized with config: " << RESET << configFile << std::endl;
	
	// 1) Parse configuration
	ConfigParser parser;
	this->config = parser.parse(configFile);

	// 2) Normalize + validate configuration
	validateAllServers(this->config);
}

/*
* - 📌TO DO:📌 [REVISAR]
* - Create listening sockets based on server blocks
* - Add listening sockets to poll()
* - Store mapping
* - ....
*/
void Webserv::setSockets()
{
	// for each Config in this->config:
	//     1. Create socket, set O_NONBLOCK, bind host:port, listen
	//     2. Add to poll fd vector
	//     3. Keep a map listeningFd -> serverIndex
	epollFd = epoll_create1(0); 
	if (epollFd < 0)
	{
		std::cerr << RED << "Error epoll create: " << strerror(errno) << RESET << std::endl;
	}

	for (size_t i = 0; i < this->config.size(); ++i)
	{
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0) continue;
		int opt = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
		fcntl(fd, F_SETFL, O_NONBLOCK);

		addrinfo hints, *res;
		std::memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;

		if (getaddrinfo(config[i].host.c_str(), NULL, &hints, &res) != 0)
		{
			std::cerr << RED << "Error binding port " << config[i].port << " " << strerror(errno) << RESET << std::endl;
			continue;
		}
		
		sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
		addr->sin_port = htons(config[i].port);

		if (bind(fd, res->ai_addr, res->ai_addrlen) < 0)
		{
			std::cerr << RED << "Error binding port " << config[i].port << ": " << strerror(errno) << RESET << std::endl;
			freeaddrinfo(res);
			close(fd);
			continue;
		}
		freeaddrinfo(res);

		if (listen(fd, 128) < 0)
		{
			std::cerr << RED << "Error listen fd: " << strerror(errno) << RESET << std::endl;
			close(fd);
			continue;
		}

		struct epoll_event ev;
		std::memset(&ev, 0, sizeof(ev));
		ev.events = EPOLLIN;
		ev.data.fd = fd;

		if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev) < 0)
		{
			std::cerr << "Error epoll_ctl" << std::endl;
			close(fd);
			continue;
		}

		this->fdToConfig[fd] = this->config[i];
		this->fds.push_back(fd);
		std::cout << PURPLE << "Server [" << config[i].server_name << "] listening on port " << config[i].port << RESET << std::endl;
	}
}

bool Webserv::isListeningFd(int fd)
{
    for (size_t i = 0; i < this->fds.size(); ++i)
	{
        if (this->fds[i] == fd)
            return true;
    }
    return false;
}

void Webserv::acceptNewConnection(int listeningFd)
{
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    int clientFd = accept(listeningFd, (struct sockaddr *)&clientAddr, &clientLen);
    
    if (clientFd < 0) {
        std::cerr << "Error en accept: " << strerror(errno) << std::endl;
        return;
    }

    
    fcntl(clientFd, F_SETFL, O_NONBLOCK);
    struct epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = clientFd;

    if (epoll_ctl(this->epollFd, EPOLL_CTL_ADD, clientFd, &ev) < 0) {
        std::cerr << "Error añadiendo cliente a epoll" << std::endl;
        close(clientFd);
        return;
    }

    // 4. (Opcional pero recomendado) Guardar que este cliente pertenece a X configuración
    // Esto es útil para saber qué límites de 'client_max_body_size' aplicarle luego.
    //this->clientToConfig[clientFd] = this->fdToConfig[listeningFd];

    std::cout << YELLOW << "New connection accepted on FD " << clientFd << RESET << std::endl;
}

void Webserv::handleClient(int fd)
{
}

/*
* Main event loop using poll():
* - Accept clients from listening sockets
* - Read from client sockets, accumulate data until a full HTTP request is available
* - Once request is parsed, build response via routing pipeline (below)
* - ...
*
* - 📌TO DO:📌 [REVISAR] 
* 	- poll(fds, ...)
*	- handle events:
*		   1) listening fd -> accept() new client -> add client fd to poll
*		   2) client fd POLLIN -> read request data -> parse HttpRequest
*		      -> when request complete: run routing pipeline (calls matchLocation + resolvePath)
*		   3) client fd POLLOUT -> send pending response bytes
*		   4) CGI pipe fds -> progress CGI sessions (read/write) + finalize response
*/
void Webserv::run()
{
	// 🤖 AI BASE STRUCTURE FOR RUN FUNCTION 🤖 [REVISAR] ❗❗
	// - 1. Select server config:
	// 		* by listening socket (port) and optionally Host header
	// - 2. Location match:
	//      * const Location* loc = matchLocation(serverCfg, req.path);
	// - 3. Apply location directives before filesystem:
	//	    * if loc->redir -> return 3xx response
	//	    * if method not allowed -> 405
	// - 4. File resolution:
	//      * ResolvedPath rp = resolvePath(*loc, req.path);
	//        // rp.fsPath is the target path on disk
	// - 5. Decide static vs autoindex vs CGI:
	//      * stat(rp.fsPath) to detect file/dir
	//      * if directory:
	//	      - if autoindex on -> generate listing
	//        - else -> append index and retry, or 403/404 depending on policy
	//	    * CGI detection:
	//	      CgiTarget cgi = detectCgi(*loc, rp.fsPath);
	//	      if cgi.isCgi -> run CGI subsystem
	// - 6. CGI execution (poll-integrated):
	//     * Create CgiSession (fork/exec + pipes)
	//     * Add CGI pipe fds to poll set
	//     * POLLOUT: write request body to CGI stdin, then close (EOF)
	//     * POLLIN: read CGI stdout until EOF / Content-Length satisfied
	//     * Parse CGI headers/body and build final HttpResponse
	// - 7. Send response:
	//     * write() response to client socket (non-blocking)
	//     * close or keep-alive depending on headers
	setSockets();
	
	std::cout << GREEN << "Webserv running..." << RESET <<std::endl;

	epoll_event epoll_events[100];
	while (true)
	{
		int nfds = epoll_wait(this->epollFd, epoll_events, 100, -1);
		if (nfds < 0) 
		{
			if (errno == EINTR) continue;
			throw std::runtime_error("epoll_wait error");
		}

		for (int i = 0; i < nfds; i++)
		{
			int fd = epoll_events[i].data.fd;
			// Aquí es donde dividimos el trabajo:
			
			// PASO 2: Alguien nuevo quiere entrar
			if (isListeningFd(fd))
				acceptNewConnection(fd);
			// PASO 3: Un cliente que ya aceptamos nos envió algo
			else
				handleClient(fd);
		}
	}

}
