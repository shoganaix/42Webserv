/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   httpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kpineda- <kpineda-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 13:48:43 by kpineda-          #+#    #+#             */
/*   Updated: 2026/03/14 19:24:18 by kpineda-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

# include "webserv.hpp"

class HttpRequest
{
	public:
		std::string method;
		std::string path;
		std::string body;
		std::map<std::string, std::string> headers;
		std::string version;
		std::string query;

		bool headersFinished; // ¿Ya encontramos el \r\n\r\n?
        bool bodyFinished;    // ¿Ya leímos todo el Content-Length?
        size_t contentLength; // Para saber cuántos bytes de cuerpo faltan

    HttpRequest() : headersFinished(false), bodyFinished(false), contentLength(0) {};
};

#endif 
