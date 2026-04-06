/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kpineda- <kpineda-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/08 18:51:13 by angnavar          #+#    #+#             */
/*   Updated: 2026/04/05 23:47:01 by kpineda-         ###   ########.fr       */
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
 * Default file for directories ✔
 * - El subject dice que en config debes poder definir el fichero por defecto cuando el recurso pedido es un directorio.
 * - En handleGet() si el target es directorio:
 *   autoindex está on, generas listing, si no, se devuelves 404
 *   PERO NO si ya hay autoindex, mostrar index por directorio (DONE)

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
	if (loc->client_max_body_size > 0 && req.getBody().length() > loc->client_max_body_size)
	{
		res.setStatusCode(413);
		res.setBody("<html><body><h1>413 Payload Too Large</h1></body></html>");
		res.addHeader("Content-Type", "text/html");
		std::cerr << "client_max_body_size: " << loc->client_max_body_size << std::endl;
		std::cerr << "req.getBody().length() : " << req.getBody().length() << std::endl;
		return (res);
	}
	// 5. Resolve real path (FILESYSTEM)
	ResolvedPath resolved = resolvePath(*loc, req.getPath());

	// 5. Calling CGIHandler if necessary
	if (isCgiRequest(*loc, resolved.fsPath))
	{
		try
		{
			CgiTarget target = _cgiHandler->detectCgi(*loc, resolved.fsPath);
			CgiResult result = _cgiHandler->execute(req, target, server.server_name, server.port, "127.0.0.1");

			// Creamos el contexto para que handleCgiEvent sepa qué hacer
			CgiContext* ctx = new CgiContext();
			ctx->clientFd = req.getClientFd(); // Asegúrate de haberlo seteado antes
			ctx->bodyToWrite = req.getBody();
			ctx->bytesWritten = 0;
			ctx->pid = result.pid;
			ctx->inFd = result.inFd;
    		ctx->outFd = result.outFd;

			std::cout << "DEBUG: Cliente FD: " << req.getClientFd() 
			<< " | Pipe In: " << result.inFd 
			<< " | Pipe Out: " << result.outFd << std::endl;

			// Mapeamos ambos FDs al mismo contexto
			this->_cgiFds[result.inFd] = ctx;
			this->_cgiFds[result.outFd] = ctx;

			if (req.getBody().empty()) {
				// CLAVE: Si no hay body, cerramos el pipe de entrada YA.
				// Esto envía un EOF al binario CGI y lo desbloquea.
				epoll_ctl(this->epollFd, EPOLL_CTL_DEL, ctx->inFd, NULL);
				close(ctx->inFd);
				_cgiFds.erase(ctx->inFd);
				ctx->inFd = -1; 
			} else {
				// Si hay body (POST 100MB), registramos para escribir
				struct epoll_event evIn;
				std::memset(&evIn, 0, sizeof(evIn));
				evIn.events = EPOLLOUT;
				evIn.data.fd = ctx->inFd;
				epoll_ctl(this->epollFd, EPOLL_CTL_ADD, ctx->inFd, &evIn);
			}

			// Registramos en EPOLL
			struct epoll_event ev;
			std::memset(&ev, 0, sizeof(ev));
			
			ev.events = EPOLLOUT; // Para escribir el body al CGI
			ev.data.fd = result.inFd;
			epoll_ctl(this->epollFd, EPOLL_CTL_ADD, result.inFd, &ev);

			ev.events = EPOLLIN;  // Para leer la respuesta del CGI
			ev.data.fd = result.outFd;
			epoll_ctl(this->epollFd, EPOLL_CTL_ADD, result.outFd, &ev);

			// MARCAMOS LA RESPUESTA COMO CGI
			res.setIsCgi(true); 
			return (res);
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
	if (req.getMethod() == "GET" || req.getMethod() == "HEAD")
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

void Webserv::finalizeCgiResponse( CgiContext* ctx, int fd)
{
    HttpResponse res = _cgiHandler->parseCgiOutput(ctx->rawResponse);
    if (res.getStatusCode() < 100) res.setStatusCode(200);

    if (this->clients.count(ctx->clientFd)) {
        this->clients[ctx->clientFd].writeBuffer = res.toString(false);
        this->clients[ctx->clientFd].bytesSent = 0;
        
        struct epoll_event ev;
        std::memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLOUT;
        ev.data.fd = ctx->clientFd;
        epoll_ctl(this->epollFd, EPOLL_CTL_MOD, ctx->clientFd, &ev);
    }

    epoll_ctl(this->epollFd, EPOLL_CTL_DEL, fd, NULL);
    _cgiFds.erase(fd);
    close(fd);
    waitpid(ctx->pid, NULL, WNOHANG);
    delete ctx;
}

void Webserv::handleCgiEvent(int fd, uint32_t events)
{
    if (_cgiFds.find(fd) == _cgiFds.end())
        return;

    CgiContext* ctx = _cgiFds[fd];

    // --- 1. ESCRITURA AL CGI (Hacia el inFd / Pipe 8) ---
    if (fd == ctx->inFd && (events & EPOLLOUT))
    {
        if (ctx->bytesWritten < ctx->bodyToWrite.size())
        {
            ssize_t n = write(fd, ctx->bodyToWrite.data() + ctx->bytesWritten, 
                              ctx->bodyToWrite.size() - ctx->bytesWritten);
            if (n > 0) {
                ctx->bytesWritten += n;
            } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                // AQUÍ PARAMOS LAS 'E': Si falla, sacamos el FD de epoll
                epoll_ctl(this->epollFd, EPOLL_CTL_DEL, fd, NULL);
                _cgiFds.erase(fd);
                close(fd);
                ctx->inFd = -1;
                return; 
            }
        }
        
        // Cierre normal al terminar los 100MB
        if (ctx->inFd != -1 && ctx->bytesWritten >= ctx->bodyToWrite.size())
        {
            epoll_ctl(this->epollFd, EPOLL_CTL_DEL, fd, NULL);
            _cgiFds.erase(fd);
            close(fd);
            ctx->inFd = -1;
            // No hacemos return para que pueda leer en esta misma vuelta
        }
    }

    // --- 2. LECTURA DEL CGI (Desde el outFd / Pipe 9) ---
    // IMPORTANTE: Un 'if' nuevo, no un 'else if'
    if (_cgiFds.count(fd) && fd == ctx->outFd && (events & (EPOLLIN | EPOLLHUP | EPOLLERR))) 
    {
        char buffer[32768];
        ssize_t n = read(fd, buffer, sizeof(buffer));

        if (n > 0) {
            ctx->rawResponse.append(buffer, n);
        }
        else if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
        {
            // El CGI terminó: enviamos la respuesta acumulada al cliente
            std::cout << GREEN << "[SUCCESS] CGI finalizado. Enviando respuesta..." << RESET << std::endl;
            
            // Aquí pones tu lógica de parsear y enviar (o llamar a tu finalizeCgiResponse)
            HttpResponse res = _cgiHandler->parseCgiOutput(ctx->rawResponse);
            if (res.getStatusCode() < 100) res.setStatusCode(200);

            if (this->clients.count(ctx->clientFd)) {
                this->clients[ctx->clientFd].writeBuffer = res.toString(false);
                this->clients[ctx->clientFd].bytesSent = 0;
                struct epoll_event ev = {};
                ev.events = EPOLLOUT;
                ev.data.fd = ctx->clientFd;
                epoll_ctl(this->epollFd, EPOLL_CTL_MOD, ctx->clientFd, &ev);
            }

            // Limpieza final
            epoll_ctl(this->epollFd, EPOLL_CTL_DEL, fd, NULL);
            _cgiFds.erase(fd);
            close(fd);
            waitpid(ctx->pid, NULL, WNOHANG);
            delete ctx; 
        }
    }
}


/*
 * Handles a client socket event:
 *  - Reads the HTTP request
 *  - ...
 */
void Webserv::handleClientData(int fd)
{
    char buffer[8192];
    ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);

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

    ClientState &client = this->clients[fd];
	client.request.setClientFd(fd);
    client.readBuffer.append(buffer, bytes);

    try
    {
        if (client.readBuffer.size() > 1000000 || client.request.getContentLength() > 1000000) {
             static size_t last_size = 0;
             if (client.readBuffer.size() > last_size + 10485760) { // Log cada 10MB
                 std::cout << BLUE << "[DEBUG] Recibiendo datos... Buffer actual: " 
                           << client.readBuffer.size() << " bytes" << RESET << std::endl;
                 last_size = client.readBuffer.size();
             }
        }

        if (client.request.parse(client.readBuffer))
        {
            std::cout << GREEN << "[DEBUG] REQUEST COMPLETA. Método: " << client.request.getMethod() 
                      << " | Body size: " << client.request.getBody().size() << " bytes" << RESET << std::endl;

            Config *server = &this->clients[fd].config;
            
            // Aquí es donde el router decide si es CGI
            HttpResponse res = routeRequest(client.request, *server);

			if (res.getIsCgi()) 
            {
                std::cout << PURPLE << "[DEBUG] Petición CGI detectada. Delegando a handleCgiEvent..." << RESET << std::endl;
                return; 
            }
            client.writeBuffer = res.toString(client.request.getMethod() == "HEAD");
			client.bytesSent = 0;
            
            if (client.writeBuffer.size() < 500) // Solo imprimimos si no es el body de 100MB
                std::cout << "Respuesta: " << client.writeBuffer << " para FD " << fd << RESET << std::endl;
            else
                std::cout << "Respuesta grande generada (" << client.writeBuffer.size() << " bytes)" << std::endl;

            epoll_event ev;
            std::memset(&ev, 0, sizeof(ev));
            ev.events = EPOLLOUT;
            ev.data.fd = fd;
            epoll_ctl(this->epollFd, EPOLL_CTL_MOD, fd, &ev);

            client.readBuffer.clear();
        }
    }
	catch (std::exception &e)
	{
		std::cout << RED << "Error processing request on FD " << fd << ": " << e.what() << RESET << std::endl;
		HttpResponse res;
		res.setStatusCode(400);
		res.setBody("<html><body><h1>400 Bad Request</h1></body></html>");
		res.addHeader("Content-Type", "text/html");
		client.writeBuffer = res.toString(client.request.getMethod() == "HEAD");

		// CAMBIO A MODO ESCRITURA (EPOLLOUT)
		epoll_event ev;
		std::memset(&ev, 0, sizeof(ev));
		ev.events = EPOLLOUT;
		ev.data.fd = fd;
		epoll_ctl(this->epollFd, EPOLL_CTL_MOD, fd, &ev);

		client.readBuffer.clear();
	}
}
/*
 * ...
 * */
void Webserv::handleClientWrite(int fd)
{
    if (this->clients.find(fd) == this->clients.end())
        return;

    ClientState &client = this->clients[fd];

    if (client.writeBuffer.empty() || client.bytesSent >= client.writeBuffer.size()) {
        std::cout << YELLOW << "[DEBUG] Nada que enviar o índice corrupto en FD " << fd << RESET << std::endl;
        client.bytesSent = 0;
        client.writeBuffer.clear();
        // Volvemos a modo lectura para no entrar en bucle de EPOLLOUT
        struct epoll_event ev;
        std::memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLIN;
        ev.data.fd = fd;
        epoll_ctl(this->epollFd, EPOLL_CTL_MOD, fd, &ev);
        return;
    }

    const char* ptr = client.writeBuffer.data() + client.bytesSent;
    size_t remaining = client.writeBuffer.size() - client.bytesSent;

    ssize_t sent = send(fd, ptr, remaining, 0);

    if (sent > 0) {
        client.bytesSent += sent;
        std::cout << "[DEBUG] FD " << fd << " envió " << sent << " bytes." << std::endl;
    } else if (sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        std::cerr << RED << " [ERROR] Send failed on FD " << fd << ": " << strerror(errno) << RESET << std::endl;
        this->closeConnection(fd);
        return;
    }

    // FINALIZACIÓN:
    if (client.bytesSent >= client.writeBuffer.size()) {
        std::cout << GREEN << "[SUCCESS] Respuesta enviada completa al FD " << fd << RESET << std::endl;
        client.writeBuffer.clear();
        client.bytesSent = 0;
        
        this->closeConnection(fd);
    }
}

void Webserv::closeConnection(int fd)
{
    std::map<int, CgiContext*>::iterator it = _cgiFds.begin();
    while (it != _cgiFds.end())
    {
        if (it->second->clientFd == fd)
        {
            int cgiFd = it->first;
            kill(it->second->pid, SIGKILL);
            epoll_ctl(this->epollFd, EPOLL_CTL_DEL, cgiFd, NULL);
            close(cgiFd);
            delete it->second;
            _cgiFds.erase(it++);
        }
        else
            ++it;
    }
    epoll_ctl(this->epollFd, EPOLL_CTL_DEL, fd, NULL);
    this->clients.erase(fd);
    close(fd);
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
	signal(SIGPIPE, SIG_IGN);

	
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
			else if (_cgiFds.count(fd))
                handleCgiEvent(fd, epoll_events[i].events);
			else
			{
				if (epoll_events[i].events & (EPOLLHUP | EPOLLERR)) {
					closeConnection(fd);
					continue; 
				}
				if (epoll_events[i].events & EPOLLIN)
					handleClientData(fd);
				if (epoll_events[i].events & EPOLLOUT)
					handleClientWrite(fd);
			}

			// Limpiar y cerrar si el socket se rompe bruscamente
			if (epoll_events[i].events & (EPOLLHUP | EPOLLERR))
				closeConnection(fd);
		}
	}
}


