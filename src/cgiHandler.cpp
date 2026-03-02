/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgiHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: root <root@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/02 18:05:15 by root              #+#    #+#             */
/*   Updated: 2026/03/02 18:56:14 by root             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*-----------------------------------------------------------------------
 *                          🧩CGI HANDLER🧩
 *
 *  - Detects if a request must be handled as CGI
 *      -> Checks file extension against configured cgi_ext
 *
 *  - Prepares CGI execution environment
 *      -> Builds environment variables (REQUEST_METHOD, QUERY_STRING, etc.)
 *      -> Prepares arguments for execve()
 *
 *  - Handles process creation
 *      -> Creates pipes for IPC (stdin/stdout redirection)
 *      -> Calls fork()
 *      -> Executes interpreter with execve()
 *
 *  - Manages input/output flow
 *      -> Sends POST body to CGI via stdin
 *      -> Captures CGI stdout output
 *
 *  - Parses CGI output
 *      -> Extracts headers (Content-Type, etc.)
 *      -> Separates headers from body
 *
 *  - Builds final HTTP response
 *      -> Adds HTTP status line (200 OK, 500, etc.)
 *      -> Appends CGI headers
 *      -> Appends response body
 *
 *  - Handles error cases
 *      -> fork() failure
 *      -> execve() failure
 *      -> Timeout handling (if implemented)
 *      -> Non-zero exit status
 *
 *  This file transforms external scripts into dynamic
 *  HTTP responses compatible with the webserver engine.
 *-----------------------------------------------------------------------
 */