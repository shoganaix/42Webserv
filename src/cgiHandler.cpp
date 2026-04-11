/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgiHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: macastro <macastro@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/02 18:05:15 by root              #+#    #+#             */
/*   Updated: 2026/04/11 15:03:35 by macastro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/cgiHandler.hpp"
#include "../includes/utils.hpp"
#include "../includes/logger.hpp"

/*-----------------------------------------------------------------------
 *                          🧩CGI HANDLER🧩
 *
 * This file implements the CGI execution pipeline of the webserver
 *
 *  - Detects whether a requested resource must be executed as CGI
 *  - Identifies the correct CGI handler based on file extension
 *  - Builds the argv list and CGI env variables
 *  - Executes the CGI process using fork(), pipe(), dup2(), and execve()
 *  - Sends request body data to the CGI standard input (inPipe-stdin)
 *  - Reads the CGI raw output from standard output (outPipe-stdout)
 *  - Parses that result (CGI headers and body) into an HttpResponse object
 *
 * The goal of this module is to transform a CGI script execution
 * into a valid HTTP response that can be returned by the server
 *-----------------------------------------------------------------------
 */
//---------------------------------------DETECTING-------------------------------------------

/*
 * Checks whether the requested filesystem path matches configured CGI extension
 * If a match is found:
 *   -> Builds & returns a CgiTarget containing handler path, script path, extension &
 *       working directory for later execution
 *   -> Otherwise, returns a default non-CGI target
 */
CgiTarget CgiHandler::detectCgi(const Location& loc, const std::string& fsPath)
{
    CgiTarget target;
    size_t dot = fsPath.find_last_of('.');

    if (dot == std::string::npos)
        return (target);

    std::string ext = fsPath.substr(dot);
    std::map<std::string, std::string>::const_iterator it = loc.cgi_needs.find(ext);

    if (it == loc.cgi_needs.end())
        return (target);

    target.isCgi = true;
    target.extension = ext;
    target.handlerPath = it->second;
    target.scriptPath = fsPath;
    target.workingDir = dirnameOf(fsPath);
    return (target);
}

//---------------------------------------BUILD + EXECUTE-------------------------------------------

/*
 * Builds the argv array used by execve()
 *
 * - The first argument is ALWAYS the handler executable
 * - Some CGI handlers, such as Python, also require the script path
 *      as an additional argument [BONUS: multiple CGI]
 */
std::vector<std::string> CgiHandler::buildArgv(const CgiTarget& target)
{
    std::vector<std::string> argv;
    argv.push_back(target.handlerPath);
    // Python normally asks script to be argv[1]
    if (target.handlerPath.find("python") != std::string::npos)
        argv.push_back(target.scriptPath);
    // Php-cgi just needs SCRIPT_FILENAME defined in env
    return (argv);
}

/*
 * Builds CGI env map from HTTP REQUEST and the resolved CGI target (...)
 *
 * - Define standard CGI variables (REQUEST_METHOD, ..., SERVER_PORT) &
 *      body-related metadata (CONTENT_TYPE, CONTENT_LENGTH)
 * - Copies info needed by the script to understand request and its body
 * - HTTP headers are converted to CGI format with toUpperHeaderName (HTTP_*)
 * - Returns env map
 */
std::map<std::string, std::string> CgiHandler::buildEnv(const HttpRequest& req,
                                                        const CgiTarget& target,
                                                        const std::string& serverName,
                                                        int serverPort, const std::string& clientIp)
{
    std::map<std::string, std::string> env;
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SERVER_PROTOCOL"] = req.getVersion().empty() ? "HTTP/1.1" : req.getVersion();
    env["REQUEST_METHOD"] = req.getMethod();
    env["QUERY_STRING"] = req.getQuery();
    env["SCRIPT_FILENAME"] = target.scriptPath;
    env["SCRIPT_NAME"] = req.getPath();
    env["SERVER_NAME"] = serverName;
    env["REMOTE_ADDR"] = clientIp;
    env["REDIRECT_STATUS"] = "200";
    env["SERVER_PORT"] = intToString(serverPort);
    std::map<std::string, std::string>::const_iterator it;
    it = req.getHeaders().find("Content-Type");

    if (it != req.getHeaders().end())
        env["CONTENT_TYPE"] = it->second;
    else
    {
        it = req.getHeaders().find("content-type");
        if (it != req.getHeaders().end())
            env["CONTENT_TYPE"] = it->second;
    }
    if (!req.getBody().empty())
        env["CONTENT_LENGTH"] = intToString(req.getBody().size());
    for (it = req.getHeaders().begin(); it != req.getHeaders().end(); ++it)
    {
        if (it->first == "Content-Type" || it->first == "content-type" ||
            it->first == "Content-Length" || it->first == "content-length")
            continue;
        env[toUpperHeaderName(it->first)] = it->second;
    }
    char real_path[4096];
    std::string absoluteFsPath;
    if (realpath(target.scriptPath.c_str(), real_path))
        absoluteFsPath = std::string(real_path);
    else
    {
        absoluteFsPath = target.scriptPath;
    }
    env["SCRIPT_NAME"] = req.getPath();
    env["PATH_INFO"] = req.getPath();
    env["SCRIPT_FILENAME"] = absoluteFsPath;
    env["PATH_TRANSLATED"] = absoluteFsPath;
    env["REQUEST_URI"] = req.getPath();
    return (env);
}

/*
* Executes the CGI process and returns its raw output and exit status
*  1. Creates two pipes
*       inPipe-> [STDIN] sends request body to CGI script
*       outPipe-> [STDOUT] reads CGI script output
*  2. Forks
*    - In CHILD process:
*          Redirect pipes
*          Switch to CGI working directory
*          Prepare argv (interpreter, script path & NULL!)
*          Calls buildArgv & buildEnv
*          In a loop, transforms format to 'KEY=VALUE' (adds '=' between strings)
*          Execute CGI script with execve()

*   - In PARENT process:
*          Write request body to CGI stdin (inPipe), only IF PRESENT (inPipe)
*          Read CGI output from stdout (outPipe) into result.rawOutput
*          Wait for child process to terminate
*          Store child exit status in result.exitCode
*
*  3. Returns CgiResult (child exit code and complete raw CGI output)
*      [ still needs to be parsed ] !!
*/
CgiResult CgiHandler::execute(const HttpRequest& req, const CgiTarget& target,
                              const std::string& serverName, int serverPort,
                              const std::string& clientIp)
{
    CgiResult result;
    int inPipe[2];
    int outPipe[2];

    if (!target.isCgi)
        throw(std::runtime_error("CGIHandler::execute called with a non-CGI target"));

    if (pipe(inPipe) == -1)
        throw(std::runtime_error("pipe() failed for CGIHandler::execute stdin"));
    if (pipe(outPipe) == -1)
    {
        safeClose(inPipe[0]);
        safeClose(inPipe[1]);
        throw(std::runtime_error("pipe() failed for CGIHandler::execute stdout"));
    }

    fcntl(inPipe[1], F_SETFL, O_NONBLOCK);
    fcntl(outPipe[0], F_SETFL, O_NONBLOCK);

    pid_t pid = fork();
    if (pid < 0)
    {
        safeClose(inPipe[0]);
        safeClose(inPipe[1]);
        safeClose(outPipe[0]);
        safeClose(outPipe[1]);
        throw std::runtime_error("fork() in CGIHandler::execute failed");
    }

    if (pid == 0)
    {
        dup2(inPipe[0], STDIN_FILENO);
        dup2(outPipe[1], STDOUT_FILENO);
        safeClose(inPipe[0]);
        safeClose(inPipe[1]);
        safeClose(outPipe[0]);
        safeClose(outPipe[1]);

        if (chdir(target.workingDir.c_str()) != 0)
            _exit(127);

        std::vector<std::string> argvS = buildArgv(target);
        std::vector<char*> argv = vecToCharPtr(argvS);
        std::map<std::string, std::string> envMap =
            buildEnv(req, target, serverName, serverPort, clientIp);
        std::vector<std::string> envS;

        for (std::map<std::string, std::string>::iterator it = envMap.begin(); it != envMap.end();
             ++it)
            envS.push_back(it->first + "=" + it->second);

        std::vector<char*> envp = vecToCharPtr(envS);
        logDebug(GREEN, "[CGI] Executing CGI script... " + std::string(argv[0]));
        if (access(argv[0], X_OK) != 0)
        {
            std::cerr << "[ERROR] CGI: Binario no encontrado o sin permisos: " << argv[0]
                      << std::endl;
            exit(1);
        }
        execve(argv[0], &argv[0], &envp[0]);
        _exit(127);
    }
    safeClose(inPipe[0]);
    safeClose(outPipe[1]);

    result.inFd = inPipe[1];
    result.outFd = outPipe[0];
    result.pid = pid;
    return (result);
}

//---------------------------------------PARSING OUTPUT-------------------------------------------

/*
 * Returns the directory of a filesystem path
 *  - If the path does not contain directory separator "/" -> returns current directory "."
 */
std::string CgiHandler::dirnameOf(const std::string& path)
{
    size_t pos = path.find_last_of('/');
    if (pos == std::string::npos)
        return (".");
    if (pos == 0)
        return ("/");
    return (path.substr(0, pos));
}

/*
 * Converts HTTP header name into CGI env format
 *
 * Example:
 *   "Content-Length" -> "HTTP_CONTENT_LENGTH"
 *
 *  - Hyphens ("-") are replaced with underscores ("_")
 *  - Letters are converted to uppercase
 */
std::string CgiHandler::toUpperHeaderName(const std::string& key)
{
    std::string out = "HTTP_";
    for (size_t i = 0; i < key.size(); ++i)
    {
        char c = key[i];
        if (c == '-')
            out += '_';
        else
            out += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return (out);
}

/*
 * Parses the raw CGI output and converts it into an HttpResponse
 *
 * - Separates CGI headers from body using the standard "\r\n\r\n" delimiter
 *     - If no header block is present -> the whole output is treated as a text/plain body
 *     - If a "Status" header is present -> its value is used as the HTTP status code
 * - Splits raw output into a header section and a body section
 * - Reads each header:
 *    - Removes trailing '\r' left by std::getline()
 *    - Ignores malformed lines that dont contain ':'
 *    - Extracts headers name and value
 *     - If header is "Status"-> parse its numeric HTTP status code
 *     - Otherwise -> copy the header into the HttpResponse object
 * - Stores CGI body as the HTTP response body
 * - If no Content-Type header was provided by CGI -> add "Content-Type: text/html"
 * - Returns parsed obj response
 */
HttpResponse CgiHandler::parseCgiOutput(const std::string& rawOutput)
{
    HttpResponse response;
    size_t sep = rawOutput.find("\r\n\r\n");
    if (sep == std::string::npos)
        sep = rawOutput.find("\n\n");
    if (sep == std::string::npos)
    {
        response.setStatusCode(200);
        response.addHeader("Content-Type", "text/plain");
        response.setBody(rawOutput);
        response.addHeader("Content-Length", to_string(rawOutput.size()));
        return (response);
    }

    std::string headersPart = rawOutput.substr(0, sep);
    std::string bodyPart = rawOutput.substr(sep + 4);
    std::istringstream iss(headersPart);
    std::string line;
    int statusCode = 200;
    bool hasContentType = false;

    while (std::getline(iss, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty())
            continue;

        size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;

        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        // Trim de espacios
        size_t first = value.find_first_not_of(" \t");
        if (std::string::npos != first)
        {
            size_t last = value.find_last_not_of(" \t");
            value = value.substr(first, (last - first + 1));
        }

        if (key == "Status")
        {
            std::istringstream codeStream(value);
            codeStream >> statusCode;
        }
        else
        {
            if (key == "Content-Type")
                hasContentType = true;
            response.addHeader(key, value);
        }
    }
    if (statusCode < 100 || statusCode > 599)
        statusCode = 200;
    response.setStatusCode(statusCode);
    response.setBody(bodyPart);

    // 2. FORZAR CONTENT-LENGTH (Esto es lo que el tester mira con lupa)
    // El tamaño debe ser exactamente el de bodyPart.size()
    response.addHeader("Content-Length", to_string(bodyPart.size()));

    if (!hasContentType)
        response.addHeader("Content-Type", "text/html");

    return (response);
}
