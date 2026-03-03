/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   httpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: root <root@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 13:48:43 by kpineda-          #+#    #+#             */
<<<<<<< HEAD
/*   Updated: 2026/03/03 21:57:18 by root             ###   ########.fr       */
=======
/*   Updated: 2026/03/14 19:49:40 by kpineda-         ###   ########.fr       */
>>>>>>> origin/main
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

# include "webserv.hpp"
# include <map>

class HttpRequest
{
	public:
		std::string method;
		std::string path;
		std::string body;
		std::map<std::string, std::string> headers;
		std::string version;
		std::string query; // Query extracted from URL(search?q=cat&page=2)
		
		bool headersFinished; // ¿Ya encontramos el \r\n\r\n?
        bool bodyFinished;    // ¿Ya leímos todo el Content-Length?
        size_t contentLength; // Para saber cuántos bytes de cuerpo faltan

    	HttpRequest() : headersFinished(false), bodyFinished(false), contentLength(0) {};
		~HttpRequest() {};
		
		bool parse(const std::string& rawRequest);

	private:
		void HttpRequest::parseRequestLine(const std::string& line);
		void HttpRequest::parseHeaderLine(const std::string& line);
};

#endif 
