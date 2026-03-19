/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   httpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 13:48:43 by kpineda-          #+#    #+#             */
/*   Updated: 2026/03/19 19:27:03 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

# include <map>
# include <string>
# include <cstddef>
# include <sstream>
# include <cstdlib>

class HttpRequest
{
	public:
		bool parse(const std::string& rawRequest);

		const std::string& getMethod() const;
		const std::string& getPath() const;
		const std::string& getBody() const;
		const std::string& getVersion() const;
		const std::string& getQuery() const;
		const std::map<std::string, std::string>& getHeaders() const;
	private:
		std::string method;
		std::string path;
		std::string body;
		std::string version;
		std::string query; // Query extracted from URL(search?q=cat&page=2)
		std::map<std::string, std::string> headers;

		void parseRequestLine(const std::string& line);
		void parseHeaderLine(const std::string& line);
		void parseStartLineAndHeaders(const std::string& headerPart);
		bool parseChunkedBody(const std::string& raw, size_t bodyStart);
		bool parseBody(const std::string& raw, size_t bodyStart);
};

#endif 
