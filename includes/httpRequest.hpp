/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   httpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kpineda- <kpineda-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 13:48:43 by kpineda-          #+#    #+#             */
/*   Updated: 2026/03/14 20:19:29 by kpineda-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

# include <map>
# include <string>
# include <cstddef>
# include <sstream>

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
		void parseRequestLine(const std::string& line);
		void parseHeaderLine(const std::string& line);
};

#endif 
