/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgiHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: root <root@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/02 18:05:15 by root              #+#    #+#             */
/*   Updated: 2026/03/02 21:47:50 by root             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*-----------------------------------------------------------------------
 *                          🧩CGI HANDLER🧩
 *
 *  - Detects if a request must be handled as CGI ❌
 *      -> Checks file extension against configured cgi_ext
 *
 *  - Prepares CGI execution environment ✅
 *      -> Builds environment variables (REQUEST_METHOD, QUERY_STRING, etc.)
 *      -> Prepares arguments for execve()
 *
 *  - Handles process creation ✅
 *      -> Creates pipes for IPC (stdin/stdout redirection)
 *      -> Calls fork()
 *      -> Executes interpreter with execve()
 *
 *  - Manages input/output flow ✅
 *      -> Sends POST body to CGI via stdin
 *      -> Captures CGI stdout output
 *
 *  - Parses CGI output ❌
 *      -> Extracts headers (Content-Type, etc.)
 *      -> Separates headers from body
 *
 *  - Builds final HTTP response ❌
 *      -> Adds HTTP status line (200 OK, 500, etc.)
 *      -> Appends CGI headers
 *      -> Appends response body
 *
 *  - Handles error cases ❌
 *      -> fork() failure
 *      -> execve() failure
 *      -> Timeout handling (if implemented)
 *      -> Non-zero exit status
 *
 *  This file transforms external scripts into dynamic
 *  HTTP responses compatible with the webserver engine.
 *-----------------------------------------------------------------------
 */

#include "../includes/cgiHandler.hpp"

/*
 * 1. Creates two pipes
 *     inPipe-> [STDIN] sends request body to CGI script
 *     outPipe-> [STDOUT] reads CGI script output
 * 
 * 2. In CHILD process:
 *     Redirect pipes
 *     Prepare argv (interpreter, script path & NULL!) -> /usr/bin/python3 ./cgi-bin/test.py
 *     Set CGI environment variables:
 *        • REQUEST_METHOD
 *        • CONTENT_LENGTH
 *        • CONTENT_TYPE
 *     Execute CGI script
 * 
 * 3. In PARENT process:
 *     If method is POST-> writes request body into inPipe
 *     Close writing end to signal EOF to CGI
 *     Read CGI output (4096 bytes) from outPipe
 *       - Returns X → number of read bytes
 *       - Returns 0 → EOF
 *       - Returns -1 → error
 *     Appends bytes read to output string
 *     Wait for child process
 *
 * 4. Returns this CGI output as a string (NOT PARSED, RAW OUTPUT)
 * 
 *----------------------------------------------REMINDERS------------------------------------------- 
 * ---- ⚠️int execve(const char *pathname, char *const argv[], char *const envp[]) uses ARGV & ENVP TOO!!
 * ---- ⚠️4096 bytes = 4 KB ->  UNIX STANDARD
 * ---- ⚠️CGI TRANSFORMS HTTP RESQUEST INTO ENV VARIABLES -> REQUEST_METHOD, QUERY_STRING, CONTENT_LENGTH, CONTENT_TYPE, SCRIPT_NAME, PATH_INFO, SERVER_PROTOCOL & GATEWAY_INTERFACE
 * ------>We USE ONLY 3 of this variables defined in the 👉 Common Gateway Interface (CGI 1.1)👈
 * --------1️⃣ Request Method (GET, POST, DELETE)
 *              import os
 *              method = os.environ["REQUEST_METHOD"]
 * --------2️⃣ Content Lenght(NEEDED BY POST)
 *              import os, sys
 *              length = int(os.environ["CONTENT_LENGTH"])
 *              body = sys.stdin.read(length)
 * --------3️⃣ Content Type(So it can later treat & parse body) -> application/jso
 *              ctype = os.environ["CONTENT_TYPE"]
 * --------
 */ 
std::string CgiHandler::execute(const std::string& interpreter, const std::string& scriptPath, const std::string& method, const std::string& body)
{
    int inPipe[2];
    int outPipe[2];

    if (pipe(inPipe) < 0 || pipe(outPipe) < 0)
        return(throw (std::runtime_error("Error: Pipe failed\n")));

    pid_t pid = fork();
    if (pid < 0)
        return(throw (std::runtime_error("Error: Fork failed\n")));

    // ---------------- CHILD ----------------
    if (pid == 0)
    {
        dup2(inPipe[0], STDIN_FILENO);
        dup2(outPipe[1], STDOUT_FILENO);

        close(inPipe[1]);
        close(outPipe[0]);

        char* argv[3];
        argv[0] = const_cast<char*>(interpreter.c_str());
        argv[1] = const_cast<char*>(scriptPath.c_str());
        argv[2] = NULL;
        std::stringstream cl;
        cl << body.size();

        char* envp[4];
        envp[0] = const_cast<char*>(("REQUEST_METHOD=" + method).c_str());
        envp[1] = const_cast<char*>(("CONTENT_LENGTH=" + cl.str()).c_str());
        envp[2] = const_cast<char*>(("CONTENT_TYPE=application/x-www-form-urlencoded").c_str());
        envp[3] = NULL;

        execve(argv[0], argv, envp);
        exit(1);
    }
    // ---------------- PARENT ----------------
    close(inPipe[0]);
    close(outPipe[1]);

    if (method == "POST" && !body.empty())
        write(inPipe[1], body.c_str(), body.size());

    close(inPipe[1]);

    char buffer[4096];
    std::string output;
    ssize_t bytes;

    while ((bytes = read(outPipe[0], buffer, sizeof(buffer))) > 0)
        output.append(buffer, bytes);

    close(outPipe[0]);
    waitpid(pid, NULL, 0);
    return (output);
}