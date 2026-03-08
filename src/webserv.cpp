/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kpineda- <kpineda-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/08 18:51:13 by angnavar          #+#    #+#             */
/*   Updated: 2026/03/08 12:51:49 by kpineda-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


/* BORRAR LUEGO:
 * ----------------------------TO DO:---------------------------------
 * HTTP parsing	❌
 * Routing	❌
 * Static files	❌
 * Autoindex	❌ -> Si la URI apunta a un directorio y no hay index,
 *                    entonces el servidor puede generar una página HTML 
 *                     con el listado de archivos del directorio
 * Error pages	❌
 * Upload	❌
 * Chunked	❌     ->  Codifies body request HTTP
 * Non blocking IO correcto	❌
*/

/*-----------------------------------------------------------------------
 *                          🧠WEBSERV BRAIN🧠
 *
 * This class represents the main Webserver engine
 *
 * Responsibilities:
 *  - Load and store parsed configuration
 *  - Initialize listening sockets for each server
 *  - Manage the main event loop (epoll)
 *  - Handle client connections 
 * 	- Handle client requests
 *  - Generate a response
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
 * Creates and initializes the listening sockets defined in config
 * -📌DONE:📌 
 * - Creates listening sockets based on server blocks
 * - Configures socket OPTIONS (SO_REUSEADDR, non-blocking mode, ...) and PORT
 * - Adds listening sockets to poll()
 * - Stores mapping between fd and server configuration
 * 
 * 📌TO DO:📌
 * - Support multiple servers with same port [OPTIONAL]
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

/*
 * Checks whether a fd corresponds to one of the server listening sockets
 */
bool Webserv::isListeningFd(int fd)
{
    for (size_t i = 0; i < this->fds.size(); ++i)
	{
        if (this->fds[i] == fd)
            return true;
    }
    return false;
}

/* 
 * - Accepts a new client connection from a listening socket
 * - Sets the client socket to non-blocking mode & registers
 *    it in epoll instance for further monitoring
 */
void Webserv::acceptNewConnection(int listeningFd)
{
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    int clientFd = accept(listeningFd, (struct sockaddr *)&clientAddr, &clientLen);
    if (clientFd < 0)
    {
        std::cerr << "Error in accept: " << strerror(errno) << std::endl;
        return;
    }
    fcntl(clientFd, F_SETFL, O_NONBLOCK);

    ClientState newClient;
    newClient.fd = clientFd;
    newClient.config = this->fdToConfig[listeningFd];
    newClient.writeBuffer = "";
    this->clients[clientFd] = newClient;

    epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = clientFd;

    if (epoll_ctl(this->epollFd, EPOLL_CTL_ADD, clientFd, &ev) < 0)
    {
        std::cerr << "Error adding client to epoll" << std::endl;
        this->clients.erase(clientFd);
        close(clientFd);
        return;
    }

    std::cout << YELLOW << "New connection accepted on FD: " << clientFd 
              << " for server: " << newClient.config.server_name << RESET << std::endl;
}
/*
 * Handles a client socket event:
 *  - Reads the HTTP request
 * 
 *  📌TO DO:📌
 *  - Parses the request
 *  - Routes it through router.cpp (router decides how sever must respond to a request)
 *    [ route.cpp ]
 *    { 
 *		- selects the server
 *		- calls parsing location and path resolver
 *		- checks allowed methods in loc.allow_methods
 *		- checks client_max_body_size limits in loc.client_max_body_size
 *		- handles configured redirections (return)
 *      - calls detect cgi & CgiHandler if so
 *      - detects if the request targets a directory
 *            -> performs Autoindex or Index
 *      - detects if request targets a regular file
 *            -> calls staticFileHandler
 *      - handles uploads
 *            -> calls upload handler if needed
 *      - handles HTTP errors
 *            -> builds ErrorResponse (404, 403, 405, 413, 500, ...)
 *      - returns a complete HttpResponse object
 *    }
 *  - Serializes the generated HTTP response
 *  - Stores it in the client write buffer
 *  - Sends response through non-blocking socket I/O
 *  - Closes connection when the full response has been sent
 * 
 * ------------------------------ STRUCTURE ---------------------------
 * 
 * 	socket accepted
 *      ↓
 * 	read raw HTTP data
 *      ↓
 * 	HTTP parser
 *      ↓
 * 	HTTP Request
 *		↓
 * 	Router / file resolution
 *		↓
 * 	detectCgi()
 *		↓
 * 	CgiTarget (describes how a resource should be executed as CGI)
 *		↓
 * 	execute CGI
 *		↓
 * 	CgiResult (saves CGI execution result & exit status)
 *		↓
 * 	parseCgiOutput()
 *		↓
 *	HttpResponse
 *		↓	
 *	serialize response
 *	    ↓
 *	send to client
 *
 */
void Webserv::handleClientData(int fd)
{
	char buffer[8192]; // 8192 -> 8 KB
    ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);

    if (bytes <= 0)
    {
		epoll_ctl(this->epollFd, EPOLL_CTL_DEL, fd, NULL);
        this->clients.erase(fd);
        close(fd);
        return;
    }

    std::string rawRequest(buffer, bytes);
    // 1. Parses request [⚠️HTTPREQUEST.CPP]
		//HttpRequest req = parseRequest(rawRequest);
		   // parses request line, headers and body
		   // if Transfer-Encoding: chunked → rebuild body
		   // separates query string from path

    // 2. Routes it [⚠️ROUTER.CPP]
		//Router router(this->servers);
		//HttpResponse res = router.route(req);

	// 3. Serializes the generated response
		//std::string rawResponse = res.serialize();
	
 	// 4. Stores it in the client write buffer
	ClientState &client = this->clients[fd];
	std::string dummyResponse = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello Webserv";
	client.writeBuffer = dummyResponse;

	// --- CAMBIO A MODO ESCRITURA ---
    // Le decimos a epoll que ahora queremos saber cuando el socket esté listo para enviar (OUT)
    epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN | EPOLLOUT; // Escuchamos ambos por seguridad
    ev.data.fd = fd;
    epoll_ctl(this->epollFd, EPOLL_CTL_MOD, fd, &ev);
}

void Webserv::handleClientWrite(int fd)
{
    ClientState &client = this->clients[fd];

    if (client.writeBuffer.empty())
        return;

	// 5. Sends HTTP response through non-blocking socket I/O
    ssize_t bytesSent = send(fd, client.writeBuffer.c_str(), client.writeBuffer.size(), 0);

    if (bytesSent > 0)
        client.writeBuffer.erase(0, bytesSent);
	else if (bytesSent < 0)
    {
        std::cerr << RED << " [ERROR] Send failed on FD " << fd 
                  << ": " << strerror(errno) << RESET << std::endl;
        this->closeConnection(fd);
    }

	// 6. Closes connection
    if (client.writeBuffer.empty())
    {
        std::cout << GREEN << " [SUCCESS] Response sent successfully to FD " << fd << RESET << std::endl;
        this->closeConnection(fd);
    }
}


/*
 * -----------------------------MAIN EVENT LOOP USING EPOLL-----------------------------
 *  Starts the main server loop using epoll. 
 * 
 *  - Initializes all listening sockets
 *  - Waits for I/O events on the epoll instance
 *  - For each triggered fd:
 *     -> If that fd is a listening socket, accepts new client connections on sockets
 *     -> Otherwise, client request is processed
 *  - Handles client activity on connected sockets
 * 
 *- 📌DONE:📌
 * 	- poll(fds, ...)
 *	- handle events:
 *		   1) listening fd -> accept() new client -> add client fd to poll
 *		   2) client fd POLLIN -> read request data -> parse HttpRequest
 *		      -> when request complete: run routing pipeline (calls matchLocation + resolvePath)
 *		   3) client fd POLLOUT -> send pending response bytes
 *  - Send responses back to clients
 */
void Webserv::run()
{
	setSockets();
	
	std::cout << GREEN << "Webserv running..." << RESET <<std::endl;

	epoll_event epoll_events[100];
	while (true)
	{
		int nfds = epoll_wait(this->epollFd, epoll_events, 100, -1);
		if (nfds < 0) 
		{
			if (errno == EINTR)
				continue;
			throw std::runtime_error("epoll_wait error");
		}

		for (int i = 0; i < nfds; i++)
		{
			int fd = epoll_events[i].data.fd;
			if (isListeningFd(fd))
				acceptNewConnection(fd);
			else
			{
				// Si el socket está listo para leer
				if (epoll_events[i].events & EPOLLIN)
					handleClientData(fd);
				// Si el socket está listo para escribir
				if (epoll_events[i].events & EPOLLOUT)
					handleClientWrite(fd);
			}

            // Limpiar y cerrar si el socket se rompe bruscamente
			if (epoll_events[i].events & (EPOLLHUP | EPOLLERR))
            	closeConnection(fd);
		}
	}
}
