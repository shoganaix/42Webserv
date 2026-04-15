/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   httpResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 13:48:49 by kpineda-          #+#    #+#             */
/*   Updated: 2026/04/15 14:41:53 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*  ....
*/

#include "httpResponse.hpp"
#include "pathResolver.hpp"
#include "colors.hpp"
#include "utils.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

/* Static member definitions */
std::map<int, std::string> HttpResponse::statusMessages;
std::map<std::string, std::string> HttpResponse::mimeTypes;

/* Initializes HTTP response with default values
 * - Sets HTTP version to 1.1
 * - Default status code = 200 OK
 * - Initializes status messages and MIME types (only once)
 */
HttpResponse::HttpResponse() : _isCgi(false), version("HTTP/1.1"), statusCode(200), body("")
{
    initializeStatusMessages();
    initializeMimeTypes();
}

/* Default destructor */
HttpResponse::~HttpResponse() {}

/* Initializes HTTP status code -> message map
 * Ensures initialization happens only once (static storage)
 */
void HttpResponse::initializeStatusMessages()
{
    if (!statusMessages.empty())
        return;
    statusMessages[200] = "OK";
    statusMessages[201] = "Created";
    statusMessages[204] = "No Content";
    statusMessages[301] = "Moved Permanently";
    statusMessages[302] = "Found";
    statusMessages[400] = "Bad Request";
    statusMessages[403] = "Forbidden";
    statusMessages[404] = "Not Found";
    statusMessages[405] = "Method Not Allowed";
    statusMessages[413] = "Payload Too Large";
    statusMessages[500] = "Internal Server Error";
}

/* Initializes file extension -> MIME type map
 * Used to set correct Content-Type header when serving files
 */
void HttpResponse::initializeMimeTypes()
{
    if (!mimeTypes.empty())
        return;
    mimeTypes["html"] = "text/html";
    mimeTypes["htm"] = "text/html";
    mimeTypes["css"] = "text/css";
    mimeTypes["js"] = "application/javascript";
    mimeTypes["jpg"] = "image/jpeg";
    mimeTypes["jpeg"] = "image/jpeg";
    mimeTypes["png"] = "image/png";
    mimeTypes["gif"] = "image/gif";
    mimeTypes["txt"] = "text/plain";
}

void HttpResponse::setStatusCode(int code) { statusCode = code; }
int HttpResponse::getStatusCode() const { return (statusCode); }

/* Adds or overrides a header key-value pair */
void HttpResponse::addHeader(const std::string& key, const std::string& value)
{
    headers[key] = value;
}

/* Sets response body
 * Automatically updates Content-Length header
 */
void HttpResponse::setBody(const std::string& body)
{
    this->body = body;

    std::stringstream ss;
    ss << body.length();
    addHeader("Content-Length", ss.str());
}

/* Resets response to default state
 * - Clears headers and body
 * - Resets status code and version
 */
void HttpResponse::clear()
{
    version = "HTTP/1.1";
    statusCode = 200;
    body.clear();
    headers.clear();
}

/* Generates HTML directory listing (autoindex)
 * 1. Opens directory
 * 2. Iterates over entries (files & folders)
 * 3. Skips hidden files (except "..")
 * 4. Builds HTML list with links
 * 5. Adds '/' suffix for directories
 * 6. Returns complete HTML page
 */
std::string HttpResponse::generateAutoIndex(const std::string& path)
{
    DIR* dir;
    struct dirent* entry;
    std::stringstream html;

    html << "<html><head><title>Index of " << path << "</title></head><body>";
    html << "<h1>Index of " << path << "</h1><hr><ul>";

    if ((dir = opendir(path.c_str())) != NULL)
    {
        while ((entry = readdir(dir)) != NULL)
        {
            std::string name = entry->d_name;
            if (name == ".")
                continue;

            if (name[0] == '.' && name != "..")
                continue;

            html << "<li><a href=\"" << name;

            if (name == ".." || entry->d_type == DT_DIR)
                html << "/";

            html << "\">" << name << "</a></li>";
        }
        closedir(dir);
    }
    else
        return "<html><body><h1>Error al leer el directorio</h1></body></html>";
    html << "</ul><hr></body></html>";
    return html.str();
}

/* Loads file from filesystem and prepares HTTP response
 * 1. Checks if file exists (stat)
 *      - If not -> 404 Not Found
 * 2. Checks if it's a regular file
 *      - If not -> 403 Forbidden
 * 3. Opens file in binary mode
 * 4. Reads entire content into response body
 * 5. Extracts file extension
 * 6. Sets appropriate Content-Type using MIME map
 *      - Defaults to application/octet-stream if unknown
 * 7. Sets status code 200 on success
 * 8. Handles permission errors (403)
 */
void HttpResponse::loadFile(const std::string& path)
{
    struct stat path_stat;
    // Verify if the file exists and is a regular file
    if (stat(path.c_str(), &path_stat) != 0)
    {
        // File doesn't exist or can't be accessed
        setStatusCode(404);
        setBody("<html><body><h1>404 Not Found</h1><p>The requested resource was not found on this "
                "server.</p></body></html>");
        addHeader("Content-Type", "text/html");
        return;
    }

    if (!S_ISREG(path_stat.st_mode))
    {
        setStatusCode(403);
        setBody("<html><body><h1>403 Forbidden</h1></body></html>");
        addHeader("Content-Type", "text/html");
        return;
    }
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
    if (file.is_open())
    {
        // 1. Read file content into body
        std::stringstream ss;
        ss << file.rdbuf();
        setBody(ss.str());

        // 2. Find file extension
        size_t dot_pos = path.find_last_of(".");
        if (dot_pos != std::string::npos)
        {
            // Extract extension
            std::string ext = path.substr(dot_pos + 1);
            // Convert to lowercase for case-insensitive matching
            std::string lowerExt = toLower(ext);

            // Search for MIME type based on extension
            if (mimeTypes.count(lowerExt))
                addHeader("Content-Type", mimeTypes[lowerExt]);
            else
                addHeader("Content-Type", "application/octet-stream");
        }
        setStatusCode(200);
        file.close();
    }
    else
    {
        // File exists but can't be opened (permissions issue)
        setStatusCode(403);
        setBody("<html><body><h1>403 Forbidden</h1><p>You don't have permission to access this "
                "resource.</p></body></html>");
        addHeader("Content-Type", "text/html");
    }
}

/* Handles HTTP GET request
 * 1. Checks if resolved path exists
 *      - If not -> 404
 * 2. If path is a directory:
 *      a) Try to serve index file (loc.index)
 *      b) If no index and autoindex ON -> generate directory listing
 *      c) Otherwise -> 404
 * 3. If path is a file:
 *      - Calls loadFile()
 */
void HttpResponse::handleGet(const std::string& resolved, const Location& loc)
{
    struct stat s;
    if (stat(resolved.c_str(), &s) != 0)
    {
        setStatusCode(404);
        setBody("<html><body><h1>404 Not Found</h1></body></html>");
        addHeader("Content-Type", "text/html");
        return;
    }

    if (S_ISDIR(s.st_mode))
    {
        // Try index inside directory
        if (!loc.index.empty())
        {
            std::string indexPath = resolved;
            if (!indexPath.empty() && indexPath[indexPath.size() - 1] != '/')
                indexPath += "/";
            indexPath += loc.index;

            struct stat indexStat;
            if (stat(indexPath.c_str(), &indexStat) == 0 && S_ISREG(indexStat.st_mode))
            {
                loadFile(indexPath);
                return;
            }
        }
        // If no index AND autoindex ON
        if (loc.autoindex)
        {
            setStatusCode(200);
            setBody(generateAutoIndex(resolved));
            addHeader("Content-Type", "text/html");
            return;
        }
        // If no index nor autoindex
        setStatusCode(404);
        setBody("<html><body><h1>404 Not Found</h1></body></html>");
        addHeader("Content-Type", "text/html");
        return;
    }

    loadFile(resolved);
}

/* Handles HTTP DELETE request
 * 1. Checks if file exists
 *      - If not -> 404
 * 2. If target is a directory -> 403 (forbidden)
 * 3. Attempts to delete file using unlink()
 *      - Success -> 200 OK
 *      - Failure -> 500 Internal Server Error
 */
void HttpResponse::handleDelete(const std::string& resolved, const Location& loc)
{
    (void)loc; // Avoid unused parameter warning
    //----- Uses resolvedPath
    std::string fullPath = resolved;
    //------
    struct stat s;

    if (stat(fullPath.c_str(), &s) != 0)
    {
        setStatusCode(404);
        setBody("<html><body><h1>404 Not Found</h1><p>El archivo no existe.</p></body></html>");
        addHeader("Content-Type", "text/html");
        return;
    }
    else if (S_ISDIR(s.st_mode))
    {
        setStatusCode(403);
        setBody("<html><body><h1>403 Forbidden</h1><p>No se pueden borrar "
                "directorios.</p></body></html>");
        addHeader("Content-Type", "text/html");
        return;
    }
    else if (unlink(fullPath.c_str()) == 0)
    {
        setStatusCode(200);
        setBody(
            "<html><body><h1>File Deleted</h1><p>El recurso ha sido eliminado.</p></body></html>");
        addHeader("Content-Type", "text/html");
    }
    else
    {
        setStatusCode(500);
        setBody("<html><body><h1>500 Internal Server Error</h1><p>No se pudo borrar el "
                "archivo.</p></body></html>");
        addHeader("Content-Type", "text/html");
    }
}

/* Saves POST body into file
 * 1. Builds full file path (uploadPath + filename)
 * 2. Opens file in binary write mode
 * 3. Writes body content
 * 4. Returns true on success, false otherwise
 */
bool HttpResponse::savePostFile(const std::string& uploadPath, const std::string& body,
                                const std::string& filename)
{
    std::string fullPath = uploadPath + "/" + filename;

    std::ofstream file(fullPath.c_str(), std::ios::out | std::ios::binary);
    if (file.is_open())
    {
        file << body;
        file.close();
        return true;
    }
    return false;
}

/* Handles HTTP POST request (file write)
 * 1. Checks if file already exists
 * 2. Opens/creates file at resolved path
 * 3. Writes request body into file
 * 4. If write fails -> 500 error
 * 5. If file existed before -> 200 OK
 *    Otherwise            -> 201 Created
 */
void HttpResponse::handlePost(const std::string& resolved, const std::string& body,
                              const Location& loc)
{
    (void)loc;

    struct stat s;
    bool existedBefore = (stat(resolved.c_str(), &s) == 0);

    std::ofstream file(resolved.c_str(), std::ios::out | std::ios::binary);
    if (!file.is_open())
    {
        setStatusCode(500);
        setBody("<html><body><h1>500 Internal Server Error</h1><p>Failed to save "
                "file.</p></body></html>");
        addHeader("Content-Type", "text/html");
        return;
    }

    file.write(body.data(), body.size());
    file.close();

    if (!file)
    {
        setStatusCode(500);
        setBody("<html><body><h1>500 Internal Server Error</h1><p>Write failed.</p></body></html>");
        addHeader("Content-Type", "text/html");
        return;
    }

    setStatusCode(existedBefore ? 200 : 201);
    setBody("<html><body><h1>POST OK</h1></body></html>");
    addHeader("Content-Type", "text/html");
}

/* Sets HTTP redirection response
 * 1. Clears current response
 * 2. Sets status code (301, 302, etc.)
 * 3. Adds "Location" header
 * 4. Generates simple HTML body with redirect link
 */
void HttpResponse::setRedirect(const std::string& location, int code)
{
    clear();
    setStatusCode(code);
    addHeader("Location", location);
    setBody("<html><body><h1>Redirecting...</h1><p>You are being redirected to <a href=\"" +
            location + "\">" + location + "</a>.</p></body></html>");
    addHeader("Content-Type", "text/html");
}

/* Builds full HTTP response string
 * 1. Writes status line (version + code + message)
 * 2. Appends all headers
 * 3. Adds "Connection: close"
 * 4. Adds empty line (header/body separator)
 * 5. Appends body unless omitBody = true (used for HEAD)
 */
std::string HttpResponse::toString(bool omitBody) const
{
    std::stringstream response;

    std::string msg = "Unknown Status";

    if (statusMessages.find(statusCode) != statusMessages.end())
        msg = statusMessages.at(statusCode);

    response << version << " " << statusCode << " " << msg << "\r\n";

    std::map<std::string, std::string>::const_iterator it;
    for (it = headers.begin(); it != headers.end(); ++it)
        response << it->first << ": " << it->second << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    if (!omitBody)
        response << body;

    return response.str();
}
