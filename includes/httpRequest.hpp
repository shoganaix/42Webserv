/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   httpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 13:48:43 by kpineda-          #+#    #+#             */
/*   Updated: 2026/03/06 23:08:54 by usuario          ###   ########.fr       */
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
		std::string query; // Query extracted from URL(search?q=cat&page=2)

	HttpRequest() {};
};

#endif 
