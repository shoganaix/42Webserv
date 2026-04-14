/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: macastro <macastro@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/08 18:51:13 by angnavar          #+#    #+#             */
/*   Updated: 2026/04/14 09:29:32 by macastro         ###   ########.fr       */
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
 * - Eso cubre la parte “default” de forma básica pero no demuestra que esté leyendo y sirviendo
 páginas de error desde la config.
 *
 * Upload	       ❌
 * - El subject pide que los clientes puedan subir archivos y que en config puedas autorizar subida
 y definir storage location
 * - En handlePost() se guarda el body tal cual en upload_<timestamp>.txt dentro de upload_path o
 "."
 * - PERO
 *    -> no se valida bien si la ruta de upload existe
 *    -> no se gestiona multipart/form-data
 *    -> no se recupera el nombre real del fichero
 *    -> no se distingue entre POST “normal” y una subida real
 *    -> no se garantiza luego “upload some file to the server and get it back” como pide la
 evaluation sheet
 *
 * Non blocking IO ❌
 * - Usar un solo poll() o equivalente para lectura y escritura
 * - Está prohibido ajustar el comportamiento mirando errno después de read o write.
 * - Si revisan errno tras read/recv/write/send, la evaluación es un 0 (revisar handleClientData() y
 handleClientWrite())
 *
 * Default file for directories ✔
 * - El subject dice que en config debes poder definir el fichero por defecto cuando el recurso
 pedido es un directorio.
 * - En handleGet() si el target es directorio:
 *   autoindex está on, generas listing, si no, se devuelves 404
 *   PERO NO si ya hay autoindex, mostrar index por directorio (DONE)
 *
 * 2026 abril
 * Qué priorizar ahora:
 * - Primero pasar el tester: comportamiento correcto según config, sin regressions.
 * - Luego bajar ruido de logs grandes en webserv.cpp.
 * - Después optimizar rendimiento de body chunked para no reparsear todo el buffer en cada paquete,
 moviendo a parse incremental en httpRequest.cpp.
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
#include "logger.hpp"
#include "httpResponse.hpp"
#include "cgiHandler.hpp"
#include "pathResolver.hpp"

static size_t getCgiPendingBytes(const CgiContext* ctx)
{
    if (!ctx)
        return 0;
    if (ctx->writeBuffer.size() <= ctx->writeOffset)
        return 0;
    return ctx->writeBuffer.size() - ctx->writeOffset;
}

bool Webserv::parseChunkedIncremental(ClientState& client)
{
    if (!client.requestIsChunked)
        return false;

    if (!client.chunkParseInitialized)
    {
        client.chunkParseInitialized = true;
        client.chunkParsePos = client.requestHeadersEnd + 4;
        client.chunkCurrentSize = 0;
        client.chunkParseState = 0;
        client.chunkParseComplete = false;
        client.chunkDecodedBody.clear();
    }

    while (true)
    {
        if (client.chunkParseState == 0)
        {
            size_t lineEnd = client.readBuffer.find("\r\n", client.chunkParsePos);
            if (lineEnd == std::string::npos)
                return false;

            std::string sizeLine =
                client.readBuffer.substr(client.chunkParsePos, lineEnd - client.chunkParsePos);
            size_t semicolonPos = sizeLine.find(';');
            if (semicolonPos != std::string::npos)
                sizeLine.erase(semicolonPos);
            if (sizeLine.empty())
                throw std::runtime_error("Invalid chunk size");

            std::stringstream ss;
            size_t chunkSize = 0;
            ss << std::hex << sizeLine;
            ss >> chunkSize;
            if (ss.fail())
                throw std::runtime_error("Invalid chunk size");

            client.chunkCurrentSize = chunkSize;
            client.chunkParsePos = lineEnd + 2;
            if (chunkSize == 0)
                client.chunkParseState = 3;
            else
                client.chunkParseState = 1;
        }
        else if (client.chunkParseState == 1)
        {
            if (client.readBuffer.size() < client.chunkParsePos + client.chunkCurrentSize)
                return false;

            client.chunkDecodedBody.append(client.readBuffer, client.chunkParsePos,
                                           client.chunkCurrentSize);
            client.chunkParsePos += client.chunkCurrentSize;
            client.chunkParseState = 2;
        }
        else if (client.chunkParseState == 2)
        {
            if (client.readBuffer.size() < client.chunkParsePos + 2)
                return false;
            if (client.readBuffer.compare(client.chunkParsePos, 2, "\r\n") != 0)
                throw std::runtime_error("Missing CRLF after chunk data");

            client.chunkParsePos += 2;
            client.chunkParseState = 0;
        }
        else
        {
            if (client.readBuffer.size() < client.chunkParsePos + 2)
                return false;
            if (client.readBuffer.compare(client.chunkParsePos, 2, "\r\n") != 0)
                throw std::runtime_error("Invalid final chunk ending");

            client.chunkParsePos += 2;
            client.chunkParseComplete = true;
            return true;
        }
    }
}

void Webserv::destroyCgiContext(CgiContext* ctx, bool killProcess)
{
    if (!ctx)
        return;

    if (this->clients.count(ctx->clientFd))
    {
        ClientState& client = this->clients[ctx->clientFd];
        if (client.cgiCtx == ctx)
            detachCgiContext(client);
    }

    closeCgiPipe(ctx, ctx->inFd);
    closeCgiPipe(ctx, ctx->outFd);

    if (killProcess && ctx->pid > 0)
        kill(ctx->pid, SIGKILL);

    if (ctx->pid > 0)
        waitpid(ctx->pid, NULL, WNOHANG);

    delete ctx;
}

void Webserv::detachCgiContext(ClientState& client)
{
    if (client.cgiCtx)
        client.resetCgiStreamState();
}

void Webserv::closeCgiPipe(CgiContext* ctx, int& pipeFd)
{
    if (!ctx || pipeFd == -1)
        return;

    int oldFd = pipeFd;
    epoll_ctl(this->epollFd, EPOLL_CTL_DEL, oldFd, NULL);
    _cgiFds.erase(oldFd);
    close(oldFd);

    if (ctx->inFd == oldFd)
        ctx->inputRegistered = false;

    pipeFd = -1;
}

/*
 * - Receives path to conf at startup
 * - Loads and parses using ConfigParser
 * - Normalizes + validates
 * - Stores the resulting configurations internally for later use
 */
Webserv::Webserv(const std::string& configFile)
{
    std::cout << BLUE << "Webserv initialized with config: " << RESET << configFile << std::endl;

    // 1) Parse configuration
    ConfigParser parser;
    this->config = parser.parse(configFile);

    // -------------- DEBUG: ------------------
    // Print parsed configuration to verify correct parsing and normalization
    for (size_t i = 0; i < this->config.size(); ++i)
    {
        std::cerr << "[CONFIG] server " << i << " port=" << this->config[i].port
                  << " root=" << this->config[i].root << std::endl;

        for (size_t j = 0; j < this->config[i].locations.size(); ++j)
        {
            const Location& loc = this->config[i].locations[j];
            std::cerr << "[CONFIG]   location=" << loc.path
                      << " max_body=" << loc.client_max_body_size << std::endl;
        }
    }
    // ----------------------------------------

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
            std::cerr << RED << "Error binding port " << config[i].port << " " << strerror(errno)
                      << RESET << std::endl;
            continue;
        }

        sockaddr_in* addr = (struct sockaddr_in*)res->ai_addr;
        addr->sin_port = htons(config[i].port);

        if (bind(fd, res->ai_addr, res->ai_addrlen) < 0)
        {
            std::cerr << RED << "Error binding port " << config[i].port << ": " << strerror(errno)
                      << RESET << std::endl;
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
        std::cout << PURPLE << "Server [" << config[i].server_name << "] listening on port "
                  << config[i].port << RESET << std::endl;
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

    int clientFd = accept(listeningFd, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientFd < 0)
    {
        std::cerr << "Error in accept: " << strerror(errno) << std::endl;
        return;
    }
    fcntl(clientFd, F_SETFL, O_NONBLOCK); // Set non-blocking mode

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

    logDebug(GREEN, "New connection accepted on FD: " + to_string(clientFd) +
                        " for server: " + newClient.config.server_name);
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
HttpResponse Webserv::routeRequest(const HttpRequest& req, const Config& server)
{
    HttpResponse res;
    // -------------- DEBUG: ------------------
    if (DEBUG)
    {
        std::string routeMsg = "[ROUTE] method=" + req.getMethod() + " path=" + req.getPath() +
                               " body_size=" + to_string(req.getBody().size());
        logDebug(CYAN, routeMsg);
    }
    // ----------------------------------------
    // 1. Match location algorythm
    const Location* loc = matchLocation(server, req.getPath());
    // STEPS 2, 3 & 4: MOVED FROM HTTPRESPONSE
    //   - Why? HttpResponse shouldnt be the one deciding if reqeust can or cannotr execute but only
    //   handle reponse after core decides

    // 2. If NO location        → 404
    //	  	 redirection        → 302
    if (!loc)
    {
        // -------------- DEBUG: ------------------
        logDebug(RED, "[ROUTE] no matching location");
        // ----------------------------------------
        res.setStatusCode(404);
        res.setBody("<html><body><h1>404 Not Found</h1></body></html>");
        res.addHeader("Content-Type", "text/html");
        return (res);
    }
    // -------------- DEBUG: ------------------
    else
    {
        if (DEBUG)
        {
            std::string matchedMsg =
                "[ROUTE] matched location path=" + loc->path +
                " client_max_body_size=" + to_string(loc->client_max_body_size);
            logDebug(GREEN, matchedMsg);
        }
    }
    // ----------------------------------------
    if (!loc->redir.empty())
    {
        res.setRedirect(loc->redir, 302);
        return (res);
    }
    // 4. Check allowed client_max_body_size
    if (loc->client_max_body_size > 0 && req.getBody().length() > loc->client_max_body_size)
    {
        if (DEBUG)
        {
            std::string bodyLimitMsg = "[ROUTE] body size exceeds client_max_body_size limit=" +
                                       to_string(loc->client_max_body_size) +
                                       " actual=" + to_string(req.getBody().length());
            logDebug(RED, bodyLimitMsg);
        }
        res.setStatusCode(413);
        res.setBody("<html><body><h1>413 Payload Too Large</h1></body></html>");
        res.addHeader("Content-Type", "text/html");
        return (res);
    }

    // 5. Resolve real path (FILESYSTEM)
    ResolvedPath resolved = resolvePath(*loc, req.getPath());
    if (DEBUG)
    {
        std::string resolvedMsg = "[ROUTE] resolved path=" + resolved.fsPath;
        logDebug(BLUE, resolvedMsg);
    }

    // 6. Check allowed methods.
    // CGI gets priority so POST to .bla works even if the location is GET-only.
    CgiTarget target;
    if (isCgiRequest(*loc, resolved.fsPath))
        target = _cgiHandler->detectCgi(*loc, resolved.fsPath);

    if (!target.isCgi)
    {
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
            // -------------- DEBUG: ------------------
            logDebug(RED, "[ROUTE] returning 405");
            // ----------------------------------------
            res.setStatusCode(405);
            res.setBody("<html><body><h1>405 Method Not Allowed</h1></body></html>");
            res.addHeader("Content-Type", "text/html");
            return (res);
        }
    }
    else if (req.getMethod() != "GET" && req.getMethod() != "POST")
    {
        // -------------- DEBUG: ------------------
        logDebug(RED, "[ROUTE] returning 405");
        // ----------------------------------------
        res.setStatusCode(405);
        res.setBody("<html><body><h1>405 Method Not Allowed</h1></body></html>");
        res.addHeader("Content-Type", "text/html");
        return (res);
    }

    // 7. Calling CGIHandler if necessary
    if (target.isCgi)
    {
        try
        {
            CgiResult result =
                _cgiHandler->execute(req, target, server.server_name, server.port, "127.0.0.1");

            // Creamos el contexto para que handleCgiEvent sepa qué hacer
            CgiContext* ctx = new CgiContext();
            ctx->clientFd = req.getClientFd(); // Asegúrate de haberlo seteado antes
            ctx->writeBuffer = req.getBody();
            ctx->bytesWritten = 0;
            ctx->inputFinished = true;
            ctx->pid = result.pid;
            ctx->inFd = result.inFd;
            ctx->outFd = result.outFd;
            logDebug(GREEN,
                     "[ROUTE] 1 CGI executed, setting up context and epoll events. Client FD: " +
                         to_string(ctx->clientFd) + " CGI PID: " + to_string(ctx->pid));
            logDebug(GREEN, "[ROUTE] 2 CGI inFd: " + to_string(ctx->inFd) +
                                " outFd: " + to_string(ctx->outFd));
            // Mapeamos ambos FDs al mismo contexto
            this->_cgiFds[result.inFd] = ctx;
            this->_cgiFds[result.outFd] = ctx;

            if (ctx->writeBuffer.empty())
            {
                // CLAVE: no body -> epoll delete inFd and Close pipe
                // -> Esto es porque el CGI espera un EOF para empezar a ejecutar
                // y si no hay body, el pipe se queda esperando a que le escriban algo que nunca
                // llega. Al cerrar el pipe, el CGI recibe un EOF y empieza a ejecutarse
                // directamente. Esto envía un EOF al binario CGI y lo desbloquea.
                closeCgiPipe(ctx, ctx->inFd);
            }
            else
            {
                // Si hay body (POST 100MB), registramos pipe de entrada para escribir
                struct epoll_event evIn;
                std::memset(&evIn, 0, sizeof(evIn));
                evIn.events = EPOLLOUT;
                evIn.data.fd = ctx->inFd;
                epoll_ctl(this->epollFd, EPOLL_CTL_ADD, ctx->inFd, &evIn);
                ctx->inputRegistered = true;
            }

            // Registramos pipe de salida para leer la respuesta del CGI
            struct epoll_event ev;
            std::memset(&ev, 0, sizeof(ev));

            ev.events = EPOLLIN; // Para leer la respuesta del CGI
            ev.data.fd = result.outFd;
            epoll_ctl(this->epollFd, EPOLL_CTL_ADD, result.outFd, &ev);

            // MARCAMOS LA RESPUESTA COMO CGI
            res.setIsCgi(true);
            return (res);
        }
        catch (std::exception& e)
        {
            res.setStatusCode(500);
            res.setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
            res.addHeader("Content-Type", "text/html");
            return (res);
        }
    }
    // 7. Relative path for normal handlers
    //   - Why? We want to keep the logic of “how to handle a GET/POST/DELETE request” inside
    //   HttpResponse but the logic of “what is the real path of the resource we are trying to
    //   access” should be outside of it and be decided by the core router

    // COMPROBAR POR QUE HACE SUBSTR !! *
    std::string relativePath = resolved.fsPath;
    if (relativePath.empty())
        relativePath = "/";

    // 8. Dispatch method
    if (req.getMethod() == "GET" || req.getMethod() == "HEAD")
        res.handleGet(relativePath, *loc);
    else if (req.getMethod() == "POST")
        res.handlePost(resolved.fsPath, req.getBody(), *loc);
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

void Webserv::finalizeCgiResponse(CgiContext* ctx, int fd)
{
    (void)fd;
    HttpResponse res = _cgiHandler->parseCgiOutput(ctx->rawResponse);
    if (res.getStatusCode() < 100)
        res.setStatusCode(200);

    if (this->clients.count(ctx->clientFd))
    {
        this->clients[ctx->clientFd].writeBuffer = res.toString(false);
        this->clients[ctx->clientFd].bytesSent = 0;

        struct epoll_event ev;
        std::memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLOUT;
        ev.data.fd = ctx->clientFd;
        epoll_ctl(this->epollFd, EPOLL_CTL_MOD, ctx->clientFd, &ev);
    }

    destroyCgiContext(ctx, false);
}

void Webserv::registerCgiInputFd(CgiContext* ctx)
{
    if (!ctx || ctx->inFd == -1 || ctx->inputRegistered)
        return;

    struct epoll_event evIn;
    std::memset(&evIn, 0, sizeof(evIn));
    evIn.events = EPOLLOUT;
    evIn.data.fd = ctx->inFd;
    if (epoll_ctl(this->epollFd, EPOLL_CTL_ADD, ctx->inFd, &evIn) == 0)
    {
        this->_cgiFds[ctx->inFd] = ctx;
        ctx->inputRegistered = true;
    }
}

void Webserv::syncCgiInputFdState(CgiContext* ctx)
{
    if (!ctx || ctx->inFd == -1)
        return;

    if (getCgiPendingBytes(ctx) == 0 && ctx->inputFinished)
    {
        closeCgiPipe(ctx, ctx->inFd);
    }
    else if (getCgiPendingBytes(ctx) == 0 && !ctx->inputFinished && ctx->inputRegistered)
    {
        epoll_ctl(this->epollFd, EPOLL_CTL_DEL, ctx->inFd, NULL);
        _cgiFds.erase(ctx->inFd);
        ctx->inputRegistered = false;
    }
}

void Webserv::updateCgiBackpressure(ClientState& client, CgiContext* ctx)
{
    const size_t kCgiBufferHighWatermark = 512 * 1024;
    const size_t kCgiBufferLowWatermark = 128 * 1024;

    if (!ctx || client.cgiCtx != ctx)
        return;

    const size_t pending = getCgiPendingBytes(ctx);

    if (!client.cgiReadPaused && pending >= kCgiBufferHighWatermark)
        client.cgiReadPaused = true;
    else if (client.cgiReadPaused && pending <= kCgiBufferLowWatermark)
        client.cgiReadPaused = false;
}

void Webserv::handleCgiEvent(int fd, uint32_t events)
{
    const size_t kCgiProgressStepBytes = 10 * 1024 * 1024;

    if (_cgiFds.find(fd) == _cgiFds.end())
        return;

    CgiContext* ctx = _cgiFds[fd];

    // --- 1. ESCRITURA AL CGI (Hacia el inFd / Pipe 8) ---
    if (fd == ctx->inFd && (events & EPOLLOUT))
    {
        const size_t pending = getCgiPendingBytes(ctx);
        if (pending > 0)
        {
            ssize_t n = write(fd, ctx->writeBuffer.data() + ctx->writeOffset, pending);
            if (n > 0)
            {
                ctx->bytesWritten += n;
                ctx->writeOffset += static_cast<size_t>(n);

                if (ctx->bytesWritten >= ctx->progressLogCheckpoint + kCgiProgressStepBytes)
                {
                    ctx->progressLogCheckpoint =
                        (ctx->bytesWritten / kCgiProgressStepBytes) * kCgiProgressStepBytes;
                    std::cout << "[CGI-STREAM] client_fd=" << ctx->clientFd
                              << " written=" << ctx->bytesWritten
                              << " pending=" << getCgiPendingBytes(ctx) << std::endl;
                }

                if (ctx->writeOffset == ctx->writeBuffer.size())
                {
                    ctx->writeBuffer.clear();
                    ctx->writeOffset = 0;
                }
                else if (ctx->writeOffset > 65536 &&
                         ctx->writeOffset >= ctx->writeBuffer.size() / 2)
                {
                    ctx->writeBuffer.erase(0, ctx->writeOffset);
                    ctx->writeOffset = 0;
                }

                if (this->clients.count(ctx->clientFd))
                {
                    ClientState& client = this->clients[ctx->clientFd];
                    updateCgiBackpressure(client, ctx);
                }
            }
            else if (n < 0)
            {
                // Keep waiting for readiness changes; no error-specific branching here.
                return;
            }
        }

        syncCgiInputFdState(ctx);
    }

    // --- 2. LECTURA DEL CGI (Desde el outFd / Pipe 9) ---
    // IMPORTANTE: Un 'if' nuevo, no un 'else if'
    if (_cgiFds.count(fd) && fd == ctx->outFd && (events & (EPOLLIN | EPOLLHUP | EPOLLERR)))
    {
        char buffer[32768];
        ssize_t n = read(fd, buffer, sizeof(buffer));

        if (n > 0)
        {
            ctx->rawResponse.append(buffer, n);
        }
        else if (n == 0)
        {
            std::cout << "[CGI-DONE] client_fd=" << ctx->clientFd
                      << " written=" << ctx->bytesWritten
                      << " response_bytes=" << ctx->rawResponse.size() << std::endl;
            logDebug(GREEN, std::string("[CGI] Output completo bytes=") +
                                to_string(ctx->rawResponse.size()));
            finalizeCgiResponse(ctx, fd);
        }
        else if (n < 0)
        {
            // Keep waiting for readiness changes; no error-specific branching here.
            return;
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
    const size_t kProgressLogStepBytes = 5 * 1024 * 1024;

    if (this->clients.find(fd) == this->clients.end())
        return;

    ClientState& client = this->clients[fd];
    if (client.cgiStreaming && client.cgiCtx && client.cgiReadPaused)
        return;

    char buffer[65536];
    ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);

    if (bytes < 0)
        return;
    if (bytes == 0)
    {
        this->closeConnection(fd);
        return;
    }

    client.request.setClientFd(fd);
    client.readBuffer.append(buffer, bytes);

    if (client.cgiStreaming && client.cgiCtx)
    {
        CgiContext* ctx = client.cgiCtx;
        if (client.requestIsChunked)
        {
            bool chunkComplete = false;
            try
            {
                chunkComplete = parseChunkedIncremental(client);
            }
            catch (std::exception&)
            {
                this->closeConnection(fd);
                return;
            }

            if (client.chunkDecodedBody.size() > client.cgiChunkForwarded)
            {
                const size_t delta = client.chunkDecodedBody.size() - client.cgiChunkForwarded;
                ctx->writeBuffer.append(client.chunkDecodedBody, client.cgiChunkForwarded, delta);
                client.cgiChunkForwarded = client.chunkDecodedBody.size();
                client.cgiReceivedBody += delta;
                registerCgiInputFd(ctx);
            }

            if (chunkComplete)
                ctx->inputFinished = true;

            if (client.chunkParsePos > 65536 &&
                client.chunkParsePos >= client.readBuffer.size() / 2)
            {
                client.readBuffer.erase(0, client.chunkParsePos);
                client.chunkParsePos = 0;
            }
            else if (chunkComplete)
            {
                client.readBuffer.clear();
                client.chunkParsePos = 0;
            }
        }
        else if (!client.readBuffer.empty())
        {
            ctx->writeBuffer.append(client.readBuffer);
            client.cgiReceivedBody += client.readBuffer.size();
            client.readBuffer.clear();
            registerCgiInputFd(ctx);
        }

        if (!client.requestIsChunked && client.cgiReceivedBody >= client.requestBodyLength)
            ctx->inputFinished = true;

        updateCgiBackpressure(client, ctx);

        syncCgiInputFdState(ctx);

        return;
    }

    try
    {
        // ------------------------------------------------------------
        // EARLY REJECTION: TEST 200
        // If headers are already received, check Content-Length
        // and respond with 413 without waiting for the full body
        // ------------------------------------------------------------
        size_t headersEnd = std::string::npos;
        if (client.requestMetaParsed)
            headersEnd = client.requestHeadersEnd;
        else
            headersEnd = client.readBuffer.find("\r\n\r\n");
        if (headersEnd != std::string::npos)
        {
            size_t bodyStart = headersEnd + 4;
            size_t bodyAvailable = 0;
            if (client.readBuffer.size() > bodyStart)
                bodyAvailable = client.readBuffer.size() - bodyStart;

            if (!client.requestMetaParsed)
            {
                // Extract and parse headers only once for this request.
                std::string headerPart = client.readBuffer.substr(0, headersEnd);
                std::istringstream stream(headerPart);
                std::string line;

                // Parse the request line (e.g., "POST /path HTTP/1.1")
                if (std::getline(stream, line))
                {
                    if (!line.empty() && line[line.size() - 1] == '\r')
                        line.erase(line.size() - 1);

                    std::istringstream iss(line);
                    iss >> client.requestMethod >> client.requestPath >> client.requestVersion;

                    // Remove query string from the path (everything after '?')
                    size_t qpos = client.requestPath.find('?');
                    if (qpos != std::string::npos)
                    {
                        client.requestQuery = client.requestPath.substr(qpos + 1);
                        client.requestPath = client.requestPath.substr(0, qpos);
                    }
                    else
                        client.requestQuery.clear();
                }

                // Parse all HTTP headers into a map
                while (std::getline(stream, line))
                {
                    if (!line.empty() && line[line.size() - 1] == '\r')
                        line.erase(line.size() - 1);

                    if (line.empty())
                        break;

                    size_t colon = line.find(':');
                    if (colon == std::string::npos)
                        continue;

                    std::string key = line.substr(0, colon);
                    std::string value = line.substr(colon + 1);

                    // Trim leading spaces/tabs from header value
                    while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
                        value.erase(0, 1);

                    // Normalize header key to lowercase for case-insensitive lookup
                    for (size_t i = 0; i < key.size(); ++i)
                        key[i] = std::tolower(static_cast<unsigned char>(key[i]));

                    client.requestHeaders[key] = value;
                }

                client.requestHasContentLength = false;
                client.requestBodyLength = 0;
                client.requestIsChunked = false;

                std::map<std::string, std::string>::const_iterator teIt =
                    client.requestHeaders.find("transfer-encoding");
                if (teIt != client.requestHeaders.end())
                {
                    std::string te = teIt->second;
                    for (size_t i = 0; i < te.size(); ++i)
                        te[i] = std::tolower(static_cast<unsigned char>(te[i]));
                    client.requestIsChunked = (te.find("chunked") != std::string::npos);
                }

                std::map<std::string, std::string>::const_iterator clIt =
                    client.requestHeaders.find("content-length");
                if (clIt != client.requestHeaders.end())
                {
                    char* endptr = NULL;
                    unsigned long declaredLen = std::strtoul(clIt->second.c_str(), &endptr, 10);
                    if (clIt->second.empty() || (endptr && *endptr != '\0'))
                        throw std::runtime_error("Invalid Content-Length value");
                    client.requestHasContentLength = true;
                    client.requestBodyLength = static_cast<size_t>(declaredLen);
                }

                client.requestMetaParsed = true;
                client.requestHeadersEnd = headersEnd;
            }

            const std::string& method = client.requestMethod;
            const std::string& path = client.requestPath;
            const std::map<std::string, std::string>& headers = client.requestHeaders;

            if (!client.headersLogged)
            {
                std::map<std::string, std::string>::const_iterator clIt =
                    headers.find("content-length");
                std::map<std::string, std::string>::const_iterator teIt =
                    headers.find("transfer-encoding");
                std::string cl = (clIt != headers.end()) ? clIt->second : "-";
                std::string te = (teIt != headers.end()) ? teIt->second : "-";
                if (DEBUG)
                {
                    std::string headersMsg = "[HEADERS] FD=" + to_string(fd) + " method=" + method +
                                             " path=" + path + " content-length=" + cl +
                                             " transfer-encoding=" + te;
                    logDebug(BLUE, headersMsg);
                }
                client.headersLogged = true;
            }

            if (bodyAvailable >= client.lastBodyLogCheckpoint + kProgressLogStepBytes)
            {
                client.lastBodyLogCheckpoint =
                    (bodyAvailable / kProgressLogStepBytes) * kProgressLogStepBytes;
                if (DEBUG)
                {
                    std::string bodyProgressMsg = "[BODY-PROGRESS] FD=" + to_string(fd) +
                                                  " received_body=" + to_string(bodyAvailable) +
                                                  " bytes";
                    logDebug(CYAN, bodyProgressMsg);
                }
            }

            const Location* loc = NULL;
            if (!path.empty())
                loc = matchLocation(client.config, path);

            size_t maxBody = client.config.client_max_body_size;
            if (loc)
                maxBody = loc->client_max_body_size;

            // Important: bodyAvailable includes chunk metadata, so this early check
            // is reliable only for non-chunked bodies.
            if (maxBody > 0 && !client.requestIsChunked && bodyAvailable > maxBody)
            {
                if (DEBUG)
                {
                    std::string earlyBodyMsg =
                        "[EARLY] returning 413 body_received=" + to_string(bodyAvailable) +
                        " limit=" + to_string(maxBody);
                    logDebug(RED, earlyBodyMsg);
                }
                HttpResponse res;
                res.setStatusCode(413);
                res.setBody("<html><body><h1>413 Payload Too Large</h1></body></html>");
                res.addHeader("Content-Type", "text/html");

                client.writeBuffer = res.toString(false);
                client.bytesSent = 0;
                client.readBuffer.clear();
                client.headersLogged = false;
                client.lastBodyLogCheckpoint = 0;
                client.resetRequestCache();

                epoll_event ev;
                std::memset(&ev, 0, sizeof(ev));
                ev.events = EPOLLOUT;
                ev.data.fd = fd;
                epoll_ctl(this->epollFd, EPOLL_CTL_MOD, fd, &ev);
                return;
            }

            // If we have a valid path and a Content-Length header, validate it
            if (!path.empty() && client.requestHasContentLength)
            {
                size_t waitingFor = 0;
                if (client.requestBodyLength > bodyAvailable)
                    waitingFor = client.requestBodyLength - bodyAvailable;
                if (DEBUG)
                {
                    std::string bodyCheckMsg =
                        "[BODY-CHECK] path=" + path +
                        " declared_content-length=" + to_string(client.requestBodyLength) +
                        " body_available=" + to_string(bodyAvailable) +
                        " waiting_for=" + to_string(waitingFor) + " bytes";
                    logDebug(GREEN, bodyCheckMsg);
                }

                if (loc)
                {
                    // Check against configured maximum body size
                    if (loc->client_max_body_size > 0 &&
                        client.requestBodyLength > loc->client_max_body_size)
                    {
                        if (DEBUG)
                        {
                            std::string earlyLenMsg =
                                std::string("[EARLY] returning 413") +
                                " declaredLen=" + to_string(client.requestBodyLength) +
                                " limit=" + to_string(loc->client_max_body_size);
                            logDebug(RED, earlyLenMsg);
                        }
                        // Build 413 Payload Too Large response
                        HttpResponse res;
                        res.setStatusCode(413);
                        res.setBody("<html><body><h1>413 Payload Too Large</h1></body></html>");
                        res.addHeader("Content-Type", "text/html");

                        // Prepare response for sending
                        client.writeBuffer = res.toString(false);
                        client.bytesSent = 0;

                        // Clear read buffer to stop processing request body
                        client.readBuffer.clear();
                        client.headersLogged = false;
                        client.lastBodyLogCheckpoint = 0;
                        client.resetRequestCache();

                        // Switch epoll to write mode to send the response immediately
                        epoll_event ev;
                        std::memset(&ev, 0, sizeof(ev));
                        ev.events = EPOLLOUT;
                        ev.data.fd = fd;
                        epoll_ctl(this->epollFd, EPOLL_CTL_MOD, fd, &ev);

                        return;
                    }
                }
            }

            if (!client.cgiStreaming && loc && method == "POST" &&
                (client.requestHasContentLength || client.requestIsChunked))
            {
                ResolvedPath resolved = resolvePath(*loc, path);
                if (isCgiRequest(*loc, resolved.fsPath))
                {
                    CgiTarget target = _cgiHandler->detectCgi(*loc, resolved.fsPath);
                    if (target.isCgi)
                    {
                        try
                        {
                            client.request.setClientFd(fd);
                            client.request.setMethod(client.requestMethod);
                            client.request.setPath(client.requestPath);
                            client.request.setQuery(client.requestQuery);
                            client.request.setVersion(client.requestVersion);
                            client.request.setHeaders(client.requestHeaders);
                            client.request.setBody("");

                            CgiResult result = _cgiHandler->execute(
                                client.request, target, client.config.server_name,
                                client.config.port, "127.0.0.1");

                            CgiContext* ctx = new CgiContext();
                            ctx->clientFd = fd;
                            ctx->pid = result.pid;
                            ctx->inFd = result.inFd;
                            ctx->outFd = result.outFd;
                            this->_cgiFds[result.inFd] = ctx;
                            this->_cgiFds[result.outFd] = ctx;

                            struct epoll_event evOut;
                            std::memset(&evOut, 0, sizeof(evOut));
                            evOut.events = EPOLLIN;
                            evOut.data.fd = result.outFd;
                            epoll_ctl(this->epollFd, EPOLL_CTL_ADD, result.outFd, &evOut);

                            client.cgiStreaming = true;
                            client.cgiCtx = ctx;
                            client.cgiReceivedBody = 0;
                            client.cgiChunkForwarded = 0;
                            client.cgiReadPaused = false;

                            if (client.requestIsChunked)
                            {
                                bool chunkComplete = parseChunkedIncremental(client);
                                if (!client.chunkDecodedBody.empty())
                                {
                                    ctx->writeBuffer.append(client.chunkDecodedBody);
                                    client.cgiChunkForwarded = client.chunkDecodedBody.size();
                                    client.cgiReceivedBody = client.chunkDecodedBody.size();
                                    registerCgiInputFd(ctx);
                                }

                                if (chunkComplete)
                                    ctx->inputFinished = true;

                                if (client.chunkParsePos > 65536 &&
                                    client.chunkParsePos >= client.readBuffer.size() / 2)
                                {
                                    client.readBuffer.erase(0, client.chunkParsePos);
                                    client.chunkParsePos = 0;
                                }
                                else if (chunkComplete)
                                {
                                    client.readBuffer.clear();
                                    client.chunkParsePos = 0;
                                }
                            }
                            else
                            {
                                if (bodyAvailable > 0)
                                {
                                    ctx->writeBuffer.append(client.readBuffer, bodyStart,
                                                            client.readBuffer.size() - bodyStart);
                                    client.cgiReceivedBody += client.readBuffer.size() - bodyStart;
                                    registerCgiInputFd(ctx);
                                }

                                if (client.cgiReceivedBody >= client.requestBodyLength)
                                    ctx->inputFinished = true;
                            }

                            updateCgiBackpressure(client, ctx);

                            syncCgiInputFdState(ctx);

                            client.readBuffer.clear();
                            return;
                        }
                        catch (std::exception&)
                        {
                            HttpResponse res;
                            res.setStatusCode(500);
                            res.setBody(
                                "<html><body><h1>500 Internal Server Error</h1></body></html>");
                            res.addHeader("Content-Type", "text/html");
                            client.writeBuffer = res.toString(false);
                            client.bytesSent = 0;
                            client.readBuffer.clear();
                            client.headersLogged = false;
                            client.lastBodyLogCheckpoint = 0;
                            client.resetRequestCache();

                            epoll_event ev;
                            std::memset(&ev, 0, sizeof(ev));
                            ev.events = EPOLLOUT;
                            ev.data.fd = fd;
                            epoll_ctl(this->epollFd, EPOLL_CTL_MOD, fd, &ev);
                            return;
                        }
                    }
                }
            }

            bool requestComplete = true;
            if (client.requestIsChunked)
            {
                requestComplete = parseChunkedIncremental(client);

                if (maxBody > 0 && client.chunkDecodedBody.size() > maxBody)
                {
                    HttpResponse res;
                    res.setStatusCode(413);
                    res.setBody("<html><body><h1>413 Payload Too Large</h1></body></html>");
                    res.addHeader("Content-Type", "text/html");

                    client.writeBuffer = res.toString(false);
                    client.bytesSent = 0;
                    client.readBuffer.clear();
                    client.headersLogged = false;
                    client.lastBodyLogCheckpoint = 0;
                    client.resetRequestCache();

                    epoll_event ev;
                    std::memset(&ev, 0, sizeof(ev));
                    ev.events = EPOLLOUT;
                    ev.data.fd = fd;
                    epoll_ctl(this->epollFd, EPOLL_CTL_MOD, fd, &ev);
                    return;
                }
            }

            if (client.requestHasContentLength)
            {
                if (bodyAvailable < client.requestBodyLength)
                    requestComplete = false;
            }

            if (!requestComplete)
                return;
        }
        // ------------------------------------------------------------
        bool requestReady = false;
        if (client.requestHasContentLength && !client.requestIsChunked)
        {
            size_t bodyStart = client.requestHeadersEnd + 4;
            if (client.readBuffer.size() >= bodyStart + client.requestBodyLength)
            {
                client.request.setClientFd(fd);
                client.request.setMethod(client.requestMethod);
                client.request.setPath(client.requestPath);
                client.request.setQuery(client.requestQuery);
                client.request.setVersion(client.requestVersion);
                client.request.setHeaders(client.requestHeaders);

                if (bodyStart > 0)
                    client.readBuffer.erase(0, bodyStart);
                if (client.readBuffer.size() > client.requestBodyLength)
                    client.readBuffer.resize(client.requestBodyLength);
                client.request.swapBody(client.readBuffer);
                requestReady = true;
            }
        }
        else if (client.requestIsChunked && client.chunkParseComplete)
        {
            client.request.setClientFd(fd);
            client.request.setMethod(client.requestMethod);
            client.request.setPath(client.requestPath);
            client.request.setQuery(client.requestQuery);
            client.request.setVersion(client.requestVersion);
            client.request.setHeaders(client.requestHeaders);
            client.request.swapBody(client.chunkDecodedBody);
            requestReady = true;
        }
        else if (client.request.parse(client.readBuffer))
        {
            requestReady = true;
        }

        if (requestReady)
        {
            if (DEBUG && (client.request.getMethod() != "GET" || !client.request.getBody().empty()))
            {
                logDebug(GREEN, "[REQUEST-START] FD: " + to_string(fd) +
                                    " Method: " + client.request.getMethod() +
                                    " Path: " + client.request.getPath() + " Body size: " +
                                    to_string(client.request.getBody().size()) + " bytes");
            }

            if (DEBUG)
            {
                std::string parseCompleteMsg =
                    "[DEBUG] REQUEST COMPLETA. Método: " + client.request.getMethod() +
                    " | Body size: " + to_string(client.request.getBody().size()) + " bytes";
                logDebug(BLUE, parseCompleteMsg);
            }

            client.readBuffer.clear();
            client.headersLogged = false;
            client.lastBodyLogCheckpoint = 0;

            Config* server = &this->clients[fd].config;

            // Aquí es donde el router decide si es CGI
            HttpResponse res = routeRequest(client.request, *server);

            if (res.getIsCgi())
            {
                if (DEBUG)
                    logDebug(PURPLE, "[ROUTE] request routed to CGI handler, waiting for CGI "
                                     "output...handleCgiEvent will take care of the rest.");
                // For CGI we keep client.request body for async pipe writing,
                // but raw readBuffer is no longer needed.
                client.readBuffer.clear();
                client.headersLogged = false;
                client.lastBodyLogCheckpoint = 0;
                return;
            }
            // -------------- DEBUG: ------------------
            if (DEBUG)
            {
                std::string handleMsg =
                    "[HANDLE] response status=" + to_string(res.getStatusCode());
                logDebug(BLUE, handleMsg);
            }
            // -----------------------------------------
            client.writeBuffer = res.toString(client.request.getMethod() == "HEAD");
            client.bytesSent = 0;
            epoll_event ev;
            std::memset(&ev, 0, sizeof(ev));
            ev.events = EPOLLOUT;
            ev.data.fd = fd;
            epoll_ctl(this->epollFd, EPOLL_CTL_MOD, fd, &ev);

            client.readBuffer.clear();
            client.headersLogged = false;
            client.lastBodyLogCheckpoint = 0;
            client.resetRequestCache();
        }
    }
    catch (std::exception& e)
    {
        std::cout << RED << "Error processing request on FD " << fd << ": " << e.what() << RESET
                  << std::endl;
        HttpResponse res;
        res.setStatusCode(400);
        res.setBody("<html><body><h1>400 Bad Request</h1></body></html>");
        res.addHeader("Content-Type", "text/html");
        client.writeBuffer = res.toString(client.request.getMethod() == "HEAD");

        client.bytesSent = 0;
        client.readBuffer.clear();
        client.headersLogged = false;
        client.lastBodyLogCheckpoint = 0;
        client.resetRequestCache();
        // CAMBIO A MODO ESCRITURA (EPOLLOUT)
        epoll_event ev;
        std::memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLOUT;
        ev.data.fd = fd;
        epoll_ctl(this->epollFd, EPOLL_CTL_MOD, fd, &ev);
    }
}

/**
 * Handles write events for a client socket:
 *  - Checks if there is data to send in the client's write buffer
 *  - Sends as much data as possible to the client
 *  - If all data is sent, switches back to read mode (EPOLLIN)
 *  - If an error occurs during send, closes the connection
 *  - If the response was fully sent, closes the connection (no keep-alive)
 */
void Webserv::handleClientWrite(int fd)
{
    if (this->clients.find(fd) == this->clients.end())
        return;

    ClientState& client = this->clients[fd];

    if (client.writeBuffer.empty() || client.bytesSent >= client.writeBuffer.size())
    {
        if (DEBUG)
        {
            logDebug(GREEN, std::string("[WRITE-COMPLETE] FD ") + to_string(fd) +
                                ", total bytes sent: " + to_string(client.bytesSent));
            logDebug(YELLOW, std::string("[DEBUG] No hay más datos que enviar o índice de bytes "
                                         "enviados supera el tamaño ") +
                                 "del buffer. FD: " + to_string(fd));
        }
        client.bytesSent = 0;
        client.writeBuffer.clear();
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

    if (sent > 0)
    {
        client.bytesSent += sent;
        if (DEBUG)
        {
            logDebug(GREEN, std::string("[WRITE] Sent ") + to_string(sent) + " bytes to FD " +
                                to_string(fd) + ", total sent: " + to_string(client.bytesSent) +
                                "/" + to_string(client.writeBuffer.size()));
        }
    }
    else if (sent < 0)
        return;

    // FINALIZACIÓN:
    if (client.bytesSent >= client.writeBuffer.size())
    {
        if (DEBUG)
            logDebug(GREEN,
                     std::string("[SUCCESS] Respuesta enviada completa al FD ") + to_string(fd));
        client.writeBuffer.clear();
        client.bytesSent = 0;

        this->closeConnection(fd);
    }
}

void Webserv::closeConnection(int fd)
{
    std::vector<CgiContext*> contextsToDestroy;

    std::map<int, CgiContext*>::iterator it = _cgiFds.begin();
    while (it != _cgiFds.end())
    {
        if (it->second->clientFd == fd)
        {
            CgiContext* ctx = it->second;
            bool alreadyQueued = false;
            for (size_t i = 0; i < contextsToDestroy.size(); ++i)
            {
                if (contextsToDestroy[i] == ctx)
                {
                    alreadyQueued = true;
                    break;
                }
            }
            if (!alreadyQueued)
                contextsToDestroy.push_back(ctx);
        }
        ++it;
    }

    for (size_t i = 0; i < contextsToDestroy.size(); ++i)
        destroyCgiContext(contextsToDestroy[i], true);

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

    // Ignore SIGPIPE -> avoid crashing when writing to a closed socket
    signal(SIGPIPE, SIG_IGN);

    std::cout << GREEN << "Webserv running..." << RESET << std::endl;

    epoll_event epoll_events[100];
    while (true)
    {
        while (waitpid(-1, NULL, WNOHANG) > 0)
        {
        }

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
            const bool isCgiFd = (_cgiFds.count(fd) != 0);
            if (isListeningFd(fd))
                acceptNewConnection(fd);
            else if (isCgiFd)
                handleCgiEvent(fd, epoll_events[i].events);
            else
            {
                if (epoll_events[i].events & (EPOLLHUP | EPOLLERR))
                {
                    closeConnection(fd);
                    continue;
                }
                if (epoll_events[i].events & EPOLLIN)
                    handleClientData(fd);
                if (epoll_events[i].events & EPOLLOUT)
                    handleClientWrite(fd);
            }
        }
    }
}
