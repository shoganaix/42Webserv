/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   httpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/03 21:07:34 by root              #+#    #+#             */
/*   Updated: 2026/03/19 19:06:10 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/httpRequest.hpp"

/* --------------------------GETTERS--------------------------
 */
const std::string& HttpRequest::getMethod() const { return method; }
const std::string& HttpRequest::getPath() const { return path; }
const std::string& HttpRequest::getBody() const { return body; }
const std::string& HttpRequest::getVersion() const { return version; }
const std::string& HttpRequest::getQuery() const { return query; }
const std::map<std::string, std::string>& HttpRequest::getHeaders() const { return headers; }

 /* Parses HTTP request line and extracts method, path, query and version
 * 1. Creates a string stream for parsing space-separated tokens (obj fields)
 * 2. Extracts tokens from line: method, path:(ex: /index.html), version (eX., HTTP/1.1)
 * 3. Checks if path contains query (-> indicated by '?'):
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
    size_t pos = path.find('?');
    if (pos != std::string::npos)
    {
        query = path.substr(pos + 1);
        path = path.substr(0, pos);
    }
    else
    {
        query.clear();
    }
}

 /* Parses HTTP header line and stores it in 'headers' map (key + value)
 * 1. Searches for ':' that divides header key and value
 * 2. If ':' is not found -> Exception (invalid header format)
 *    If it is            -> Extracts:
 *      a) key: substring before ':'
 *      b) value: substring after ':'
 * 3. Trims spaces or tabs from value
 * 4. Normalizes key by converting all characters to lowercase
 * 5. Stores in headers map
 */
void HttpRequest::parseHeaderLine(const std::string& line)
{
    size_t pos = line.find(':');
    if (pos == std::string::npos)
        throw (std::runtime_error("Invalid header format"));

    std::string key = line.substr(0, pos);
    std::string value = line.substr(pos + 1);

    while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
        value.erase(0, 1);

    for (size_t i = 0; i < key.size(); ++i)
        key[i] = std::tolower(static_cast<unsigned char>(key[i]));

    headers[key] = value;
}


/*
 * Parses chunked HTTP body (Transfer-Encoding: chunked)
 * 1. Reads chunk size in hexadecimal
 * 2. Appends each chunk to body
 * 3. Stops when chunk of size 0 is found
 * 4. Validates CRLF structure between chunks
 *      If chunked body is  received      -> returns true
 *      If  incomplete (waiting for data) -> returns false
 */
bool HttpRequest::parseChunkedBody(const std::string& raw, size_t bodyStart)
{
    body.clear();
    size_t pos = bodyStart;

    while (true)
    {
        size_t lineEnd = raw.find("\r\n", pos);
        if (lineEnd == std::string::npos)
            return false;

        std::string sizeLine = raw.substr(pos, lineEnd - pos);

        std::stringstream ss;
        size_t chunkSize = 0;
        ss << std::hex << sizeLine;
        ss >> chunkSize;

        if (ss.fail())
            throw std::runtime_error("Invalid chunk size");

        pos = lineEnd + 2;

        if (chunkSize == 0)
        {
            if (raw.size() < pos + 2)
                return false;

            if (raw.substr(pos, 2) != "\r\n")
                throw std::runtime_error("Invalid final chunk ending");

            return true;
        }

        if (raw.size() < pos + chunkSize + 2)
            return false;

        body.append(raw, pos, chunkSize);
        pos += chunkSize;

        if (raw.substr(pos, 2) != "\r\n")
            throw std::runtime_error("Missing CRLF after chunk data");

        pos += 2;
    }
}

/* Parses + validates HTTP message body
 * 1. Checks if'transfer-encoding' & calls parseChunkedBody() for every element with it
 * 2. Initializes body length to 0
 * 3. Checks whether the "content-length" header is present
 * 4.  If present:
 *      a) converts value from string to unsigned long
 *      b) validates that value is numeric and not empty
 *      c) stores body size as size_t
 * 5. Checks whether the raw request contains full body
 * 6.  If body is incomplete -> Returns false
 *     Otherwise             -> Extracts expectedBodyLen bytes from request(raw)
 *                             & stores it in obj
 * 7. Returns true ONLY when body has been successfully parsed
 */
bool HttpRequest::parseBody(const std::string& raw, size_t bodyStart)
{
    if (headers.count("transfer-encoding"))
    {
        std::string te = headers["transfer-encoding"];
        for (size_t i = 0; i < te.size(); ++i)
            te[i] = std::tolower(static_cast<unsigned char>(te[i]));

        if (te.find("chunked") != std::string::npos)
            return (parseChunkedBody(raw, bodyStart));
    }
    
    size_t expectedBodyLen = 0;

    if (headers.count("content-length"))
    {
        char *endptr = NULL;
        unsigned long n = std::strtoul(headers["content-length"].c_str(), &endptr, 10);

        if (*headers["content-length"].c_str() == '\0' || (endptr && *endptr != '\0'))
            throw (std::runtime_error("Invalid Content-Length value"));

        expectedBodyLen = static_cast<size_t>(n);
    }

    if (raw.size() < bodyStart + expectedBodyLen)
        return (false);

    body = raw.substr(bodyStart, expectedBodyLen);
    return (true);
}

/* Parses HTTP lines from header
 * 1. Creates a string stream to read section line by line
 * 2. Removes trailing '\r' if present (CR from Windows-style line endings)
 * 3. Parses request line to extract fields (method, path, query and version)
 * 4. Removes trailing '\r' from each header line before parsing
 * 5. Stops if empty line is found
 * 6. Parses and stores each header in the headers MAP
 * ! Throws EXCEPTION if: request empty,
 *                       request line invalid,
 *                       header line malformed 
 */
void HttpRequest::parseStartLineAndHeaders(const std::string& headerPart)
{
    std::istringstream stream(headerPart);
    std::string line;

    if (!std::getline(stream, line))
        throw (std::runtime_error("Empty HTTP request"));

    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);

    try
    {
        parseRequestLine(line);
    }
    catch (std::exception &e)
    {
        throw (std::runtime_error("Invalid request line: " + std::string(e.what())));
    }

    while (std::getline(stream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        if (line.empty())
            break;

        try
        {
            parseHeaderLine(line);
        }
        catch (std::exception &e)
        {
            throw (std::runtime_error("Malformed header: " + std::string(e.what())));
        }
    }
}

/*
 * Parses the raw HTTP request
 * 0. Resets all fields
 * 1. Searches for "\r\n\r\n" sequence marking the end
 *    - If 'full header section' not received -> return false
 * 2. Extracts header section & calls parseStartLineAndHeaders();
 * 3. Calculates bodyStart (position + 4) & calls parseBody()
 * 4. Returns true ONLY when request has been successfully parsed
 */
bool HttpRequest::parse(const std::string &raw)
{
    method.clear();
    path.clear();
    query.clear();
    version.clear();
    headers.clear();
    body.clear();

    size_t headersEnd = raw.find("\r\n\r\n");
    if (headersEnd == std::string::npos)
        return (false);
    std::string headerPart = raw.substr(0, headersEnd);
    parseStartLineAndHeaders(headerPart);
    size_t bodyStart = headersEnd + 4;
    return (parseBody(raw, bodyStart));
}