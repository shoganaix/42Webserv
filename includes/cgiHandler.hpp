/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgiHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/02 18:57:01 by root              #+#    #+#             */
/*   Updated: 2026/03/09 01:05:40 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP
#include "webserv.hpp"
#include "httpRequest.hpp"
#include "httpResponse.hpp"
#include "configParser.hpp"


/*
 * Stores the result of a CGI execution
 * - exitCode: exit status returned by CGI process after waitpid()
 * - rawOutput: raw output written by the CGI script to stdout
 */
struct CgiResult
{
	int         exitCode;
	std::string rawOutput;

	CgiResult() : exitCode(500), rawOutput() {}
};

/*
 * Describes how a requested resource should be executed as CGI
 *
 * - isCgi: indicates whether the resource must be handled as CGI or not
 * - extension: file extension that matched a CGI rule (.py, .php, ...)
 * - handlerPath: path to the CGI interpreter (python3, php-cgi, ...)
 * - scriptPath: ABSOLUTE path to the CGI script
 * - workingDir: directory in which CGI should run
 */
struct CgiTarget
{
	bool        isCgi;
	std::string extension;
	std::string handlerPath;
	std::string scriptPath;
	std::string workingDir;

	CgiTarget() : isCgi(false) {}
};

class CgiHandler
{
	public:
		static CgiTarget detectCgi(const Location& loc, const std::string& fsPath);
		static CgiResult execute(const HttpRequest& req, const CgiTarget& target, const std::string& serverName, int serverPort, const std::string& clientIp);
		static HttpResponse parseCgiOutput(const std::string& rawOutput);
		static std::map<std::string, std::string> buildEnv(const HttpRequest& req, const CgiTarget& target, const std::string& serverName, int serverPort, const std::string& clientIp);
		//buildEnv temporarily public to debug
	private:
		//static std::map<std::string, std::string> buildEnv(const HttpRequest& req, const CgiTarget& target, const std::string& serverName, int serverPort, const std::string& clientIp);
		static std::vector<std::string> buildArgv(const CgiTarget& target);
		static std::string dirnameOf(const std::string& path);
		static std::string toUpperHeaderName(const std::string& key);
};

#endif
