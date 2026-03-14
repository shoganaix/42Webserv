/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   httpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: root <root@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/03 21:07:34 by root              #+#    #+#             */
/*   Updated: 2026/03/03 23:27:44 by root             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/httpRequest.hpp"

 /* Parses HTTP request line and extracts method, path, query and version
 * - Creates a string stream for parsing space-separated tokens (obj fields)
 * - Extracts tokens from line: method, path:(ex: /index.html), version (eX., HTTP/1.1)
 * - CheckS if path contains query (-> indicated by '?'):
 *      - If a '?' is found, split the path into two parts:
 *          a) path: the portion before '?'
 *          b) query: the portion after '?'
 *      - If no '?' is present, the entire path remains as the requested path and query empty
 */
void HttpRequest::parseRequestLine(const std::string& line)
{
    std::istringstream iss(line);
    if (!(iss >> method >> path >> version))
        throw (std::runtime_error("Request line must contain method, path, and HTTP version"));

    if (method.empty())
        throw (std::runtime_error("HTTP method is empty"));

    if (version.size() < 5 || version.substr(0, 5) != "HTTP/")
        throw (std::runtime_error("Invalid HTTP version format"));

    size_t pos = path.find('?');
    if (pos != std::string::npos)
    {
        query = path.substr(pos + 1);
        path = path.substr(0, pos);
    }
    if (path.empty())
        throw (std::runtime_error("Request path cannot be empty"));
}

void HttpRequest::parseHeaderLine(const std::string& line)
{
    (void)line;
}

/*
 * Parses the raw string and fills HTTPREQUEST obj
 * 1. Creates a string stream for easy line-by-line reading and reads first line
 *    (ex: "GET /path HTTP/1.1")
 *    - If there is no line        -> return false (parsing failure)
 *    - If the line ends with '\r' -> remove it & pass new line to parseRequestLine() to extract obj fields
 * 2.. Read lines for headers until empty line & remove '\r' if present
 *    -> Each non-empty line is passed to parseHeaderLine() to pop in headers map
 * 4. Check if "Content-Length" header is present
 *    - If so -> resize  body string to indicated length
 *    - Read exactly that many bytes from the stream into the body.and truncates
 */
bool HttpRequest::parse(const std::string &raw)
{
    std::istringstream stream(raw);
    std::string line;

    if (!std::getline(stream, line))
         throw (std::runtime_error("Empty HTTP request"));
    if (!line.empty() && line[line.size() - 1] == '\r') // (CR character from Windows-style line endings)
        line.erase(line.size() - 1);

    try {
        parseRequestLine(line);
    }
    catch (std::exception &e) {
        throw (std::runtime_error("Invalid request line: " + std::string(e.what())));
    }
    // HEADERS (map)
    while (std::getline(stream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty())
            break;
        try {
            parseHeaderLine(line); 
        } catch (std::exception &e) {
            throw (std::runtime_error("Malformed header: " + std::string(e.what())));
        }
    }

    // BODY
    if (headers.count("content-length"))
    {
        size_t len = 0;
        try {
            len = std::strtoul(headers["content-length"].c_str(), NULL, 10);
        }
        catch (std::exception &e) {
            throw (std::runtime_error("Invalid Content-Length value"));
        }

        body.resize(len);
        stream.read(&body[0], len);
        if (static_cast<size_t>(stream.gcount()) != len)
            throw (std::runtime_error("Request body truncated"));
    }

    return (true);
}