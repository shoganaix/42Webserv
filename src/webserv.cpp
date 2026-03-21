/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kpineda- <kpineda-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/08 18:51:13 by angnavar          #+#    #+#             */
/*   Updated: 2026/03/21 21:35:45 by kpineda-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* DELETE LATER:
 * ----------------------------TO DO:---------------------------------
 * HTTP parsing	    ✔
 * BASIC routing	✔
 * Static files	    ✔
 * Autoindex	    ✔
 * 	-> Si la URI apunta a un directorio y no hay index,
 *      entonces el servidor puede generar una página HTML
 *      con el listado de archivos del directorio
 * Chunked	       ✔
 *  ->  Codifies body request HTTP
 * CGI in route    ✔
 *
 * Error pages	   ❌
 * - HTML hardcodeado para 404, 403, 405, 413, 500 ...
 * - Eso cubre la parte “default” de forma básica pero no demuestra que esté leyendo y sirviendo páginas de error desde la config.
 *
 * Upload	       ❌
 * - El subject pide que los clientes puedan subir archivos y que en config puedas autorizar subida y definir storage location
 * - En handlePost() se guarda el body tal cual en upload_<timestamp>.txt dentro de upload_path o "."
 * - PERO
 *    -> no se valida bien si la ruta de upload existe
 *    -> no se gestiona multipart/form-data
 *    -> no se recupera el nombre real del fichero
 *    -> no se distingue entre POST “normal” y una subida real
 *    -> no se garantiza luego “upload some file to the server and get it back” como pide la evaluation sheet
 *
 * Non blocking IO ❌
 * - Usar un solo poll() o equivalente para lectura y escritura
 * - Está prohibido ajustar el comportamiento mirando errno después de read o write.
 * - Si revisan errno tras read/recv/write/send, la evaluación es un 0 (revisar handleClientData() y handleClientWrite())
 *
 * Default file for directories ❌
 * - El subject dice que en config debes poder definir el fichero por defecto cuando el recurso pedido es un directorio.
 * - En handleGet() si el target es directorio:
 *   autoindex está on, generas listing, si no, se devuelves 403
 *   PERO NO si ya hay autoindex, mostrar index por directorio

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
#include "matchLocation.hpp"
#include "utils.hpp"
#include "httpResponse.hpp"
#include "cgiHandler.hpp"
#include "pathResolver.hpp"

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
		if (fd < 0)
			continue;
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
 * Routes a parsed HTTP request accroding to server and location
 * (router decides how sever must respond to a request)
 * 	1. Matches request path  configured locations
 *       If no matching location found -> returns 404
 *  2. Handles redirections before any further processing
 *  3. Checks wether HTTP method allowed
 *  4. Rejects request if body exceeds client_max_body_size
 *  5. Resolves request URI into a real path (FILESYSTEM)
 *  6. Detects whether target must be executed as CGI
 *    If the request IS  CGI:
 *          a) builds the CGI target description
 *          b) executes program
 *          c) parses output into an HttpResponse
 *    If the request IS NOT CGI:
 *      	a) computes relative path inside matched location
 *      	b) dispatches request to method handler
 *  7. Returns a fully built HttpResponse object
 *   📌TO DO:📌
 *      - ...
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
HttpResponse Webserv::routeRequest(const HttpRequest &req, const Config &server)
{
	HttpResponse res;
	// 1. Match location algorythm
	const Location *loc = matchLocation(server, req.getPath());
	std::cout << "location matched: " << (loc ? loc->path : "NULL") << std::endl;
	// STEPS 2, 3 & 4: MOVED FROM HTTPRESPONSE
	//   - Why? HttpResponse shouldnt be the one deciding if reqeust can or cannotr execute but only handle reponse after core decides

	// 2. If NO location        → 404
	//	  	 redirection        → 302
	if (!loc)
	{
		res.setStatusCode(404);
		res.setBody("<html><body><h1>404 Not Found</h1></body></html>");
		res.addHeader("Content-Type", "text/html");
		return (res);
	}
	if (!loc->redir.empty())
	{
		res.setRedirect(loc->redir, 302);
		return (res);
	}
	// 3. Check allowed methods
	bool allowed = false;
	for (size_t i = 0; i < loc->allow_methods.size(); ++i)
	{
		if (loc->allow_methods[i] == req.getMethod())
		{
			allowed = true;
			break;
		}
	}
	if (!allowed)
	{
		res.setStatusCode(405);
		res.setBody("<html><body><h1>405 Method Not Allowed</h1></body></html>");
		res.addHeader("Content-Type", "text/html");
		return (res);
	}
	// 4. Check allowed client_max_body_size
	if (req.getBody().length() > server.client_max_body_size)
	{
		res.setStatusCode(413);
		res.setBody("<html><body><h1>413 Payload Too Large</h1></body></html>");
		res.addHeader("Content-Type", "text/html");
		return (res);
	}
	// 5. Resolve real path (FILESYSTEM)
	ResolvedPath resolved = resolvePath(*loc, req.getPath());

	// 5. Calling CGIHandler if necessary
	if (isCgiRequest(*loc, resolved.fsPath))
	{
		try
		{
			CgiHandler cgi;
			CgiTarget target = cgi.detectCgi(*loc, resolved.fsPath);
			CgiResult result = cgi.execute(req, target, server.server_name, server.port, "127.0.0.1");
			return (cgi.parseCgiOutput(result.rawOutput));
		}
		catch (std::exception &e)
		{
			res.setStatusCode(500);
			res.setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
			res.addHeader("Content-Type", "text/html");
			return (res);
		}
	}

	// 7. Relative path for normal handlers
	//   - Why? We want to keep the logic of “how to handle a GET/POST/DELETE request” inside HttpResponse but the logic of “what is the real path of the resource we are trying to access” should be outside of it and be decided by the core router

	// COMPROBAR POR QUE HACE SUBSTR !! *
	std::string relativePath = resolved.fsPath;
	if (relativePath.empty())
		relativePath = "/";

	// 8. Dispatch method
	if (req.getMethod() == "GET")
		res.handleGet(relativePath, *loc);
	else if (req.getMethod() == "POST")
		res.handlePost(req.getBody(), *loc);
	else if (req.getMethod() == "DELETE")
		res.handleDelete(relativePath, *loc);
	else
	{
		res.setStatusCode(405);
		res.setBody("<html><body><h1>405 Method Not Allowed</h1></body></html>");
		res.addHeader("Content-Type", "text/html");
	}
	return (res);
}

/*
 * Handles a client socket event:
 *  - Reads the HTTP request
 *  - ...
 */
void Webserv::handleClientData(int fd)
{
	char buffer[8192]; // 8192 -> 8 KB
	ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);

	// NON BLOCKING SI es tomado en cuenta !
	if (bytes < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
		this->closeConnection(fd);
		return;
	}
	if (bytes == 0)
	{
		this->closeConnection(fd);
		return;
	}

	// NON BLOCKING NO es tomado en cuenta !
	// if (bytes <= 0)
	//{
	//	epoll_ctl(this->epollFd, EPOLL_CTL_DEL, fd, NULL);
	//    this->clients.erase(fd);
	//    close(fd);
	//    return;
	//}

	// Client detected or error
	ClientState &client = this->clients[fd];
	client.readBuffer.append(buffer, bytes);
	try
	{
		if (client.request.parse(client.readBuffer))
		{
			std::cout << "Method: " << client.request.getMethod() << " Path: " << client.request.getPath() << RESET << std::endl;

			// SI ENTRA AQUÍ, es que parseRequest devolvió 'true' (Petición completa)
			Config *server = &this->clients[fd].config;
			HttpResponse res = routeRequest(client.request, *server);
			std::cout << "Response generated with status code: " << res.getStatusCode() << RESET << std::endl;
			client.writeBuffer = res.toString();
			std::cout << "Respuesta: " << client.writeBuffer << " para FD " << fd << RESET << std::endl;

			epoll_event ev;
			std::memset(&ev, 0, sizeof(ev));
			ev.events = EPOLLOUT;
			ev.data.fd = fd;
			epoll_ctl(this->epollFd, EPOLL_CTL_MOD, fd, &ev);

			client.readBuffer.clear(); // listo para próxima request
		}
	}
	catch (std::exception &e)
	{
		std::cout << RED << "Error processing request on FD " << fd << ": " << e.what() << RESET << std::endl;
		HttpResponse res;
		res.setStatusCode(400);
		res.setBody("<html><body><h1>400 Bad Request</h1></body></html>");
		res.addHeader("Content-Type", "text/html");
		client.writeBuffer = res.toString();

		// CAMBIO A MODO ESCRITURA (EPOLLOUT)
		epoll_event ev;
		std::memset(&ev, 0, sizeof(ev));
		ev.events = EPOLLOUT;
		ev.data.fd = fd;
		epoll_ctl(this->epollFd, EPOLL_CTL_MOD, fd, &ev);

		// Limpiamos el buffer de lectura para que esté listo para la siguiente petición
		client.readBuffer.clear();
	}
}
/*
 * ...
 * */
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
		// En non-blocking, si send() devuelve -1 y errno == EAGAIN o EWOULDBLOCK, NO hay que cerrar
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
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
 */
void Webserv::run()
{
	setSockets();

	std::cout << GREEN << "Webserv running..." << RESET << std::endl;

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
