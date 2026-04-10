/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   httpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: macastro <macastro@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 13:48:43 by kpineda-          #+#    #+#             */
/*   Updated: 2026/04/10 13:39:37 by macastro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <map>
#include <string>
#include <cstddef>
#include <sstream>
#include <cstdlib>

class HttpRequest
{
  public:
    bool parse(const std::string& rawRequest);
    void setClientFd(int fd) { _clientFd = fd; }
    int getClientFd() const { return _clientFd; }

    const std::string& getMethod() const;
    const std::string& getPath() const;
    const std::string& getBody() const;
    const std::string& getVersion() const;
    const std::string& getQuery() const;
    const std::map<std::string, std::string>& getHeaders() const;
    size_t getContentLength() const
    {
        std::map<std::string, std::string>::const_iterator it = headers.find("Content-Length");
        if (it == headers.end())
            it = headers.find("content-length");
        if (it != headers.end())
            return std::atoll(it->second.c_str());
        return 0;
    }

  private:
    int _clientFd;
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
