/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   httpResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 13:48:49 by kpineda-          #+#    #+#             */
/*   Updated: 2026/04/27 22:03:20 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*-----------------------------------------------------------------------
 *                        🌐 HTTP RESPONSE 🌐
 *
 * Builds and manages HTTP responses sent to the client
 *
 * Handles:
 *  - Status codes and headers
 *  - Response body and Content-Length
 *  - Static file serving and MIME types
 *  - GET, POST (uploads), and DELETE methods
 *  - Autoindex generation and redirections
 *
 * Ensures valid HTTP/1.1 responses with basic error handling
 *-----------------------------------------------------------------------*/

#include "httpResponse.hpp"
#include "httpRequest.hpp"
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

/* Extracts filename from a multipart/form-data request body
 * 1. Searches for the "filename=" field inside the multipart headers
 * 2. If no filename found or parsing fails -> returns default name ("upload")
 * 3. Extracts filename value between quotes & sanitizes filename by replacing dangerous characters
 *    ('/', '\', ':') with '_'
 * 4. Returns sanitized filename
 */
static std::string extractFileNameFromMultipart(const std::string& body,
                                                const std::string& boundary)
{
    (void)boundary;
    size_t filenamePos = body.find("filename=\"");
    if (filenamePos == std::string::npos)
        return "upload";

    filenamePos += 10;
    size_t endPos = body.find("\"", filenamePos);

    if (endPos == std::string::npos)
        return "upload";

    std::string fileName = body.substr(filenamePos, endPos - filenamePos);

    for (size_t i = 0; i < fileName.length(); ++i)
    {
        if (fileName[i] == '/' || fileName[i] == '\\' || fileName[i] == ':')
            fileName[i] = '_';
    }

    return fileName;
}

/* Extracts file content from a multipart/form-data request body
 * 1. Locates end headers (double CRLF or LF)
 *    - If no header separator is found -> returns full body as fallback
 * 2. Determines start of actual file content (after headers)
 * 3. Searches for multipart boundary marker to find where content ends
 *    - Tries both closing boundary ("--boundary--") and normal boundary
 *    - If not found -> assumes content goes until the end of the body
 * 4. Trims trailing newline chars (\r\n or \n) before boundary
 * 5. Returns extracted file content as substring of the body
 */
static std::string extractFileContentFromMultipart(const std::string& body,
                                                   const std::string& boundary)
{
    size_t headerEnd = body.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
    {
        headerEnd = body.find("\n\n");
        if (headerEnd == std::string::npos)
            return body;

        size_t contentStart = headerEnd + 2;

        std::string boundaryMarker = "--" + boundary + "--";
        size_t contentEnd = body.find(boundaryMarker);
        if (contentEnd == std::string::npos)
            contentEnd = body.find("--" + boundary);

        if (contentEnd == std::string::npos)
            contentEnd = body.length();

        if (contentEnd > 1 && body[contentEnd - 1] == '\n')
            contentEnd -= 1;

        return body.substr(contentStart, contentEnd - contentStart);
    }

    size_t contentStart = headerEnd + 4;

    std::string boundaryMarker = "--" + boundary + "--";
    size_t contentEnd = body.find(boundaryMarker);
    if (contentEnd == std::string::npos)
        contentEnd = body.find("--" + boundary);

    if (contentEnd == std::string::npos)
        contentEnd = body.length();

    if (contentEnd > 2 && body[contentEnd - 2] == '\r' && body[contentEnd - 1] == '\n')
        contentEnd -= 2;

    return body.substr(contentStart, contentEnd - contentStart);
}

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

/* Handles HTTP POST request for file uploads.
 * 1. Verifies that uploads are allowed (upload_path must be set),
 *    - Otherwise → returns 405 Method Not Allowed
 * 2. Checks request body size against client_max_body_size,
 *    - If exceeded → returns 413 Payload Too Large 
 * 3. Reads the Content-Type header to determine how to process body
 *    - If multipart/form-data → extracts filename and file content
 *    - Otherwise → treats entire body as raw file content
 * 4. Resolves final upload directory path based on config
 * 5. Creates/overwrites target file and writes content
 *    - If file cannot be opened or written → 500 Internal Server Error
 * 6. On success, returns:
 *    - 200 OK (file is written or overwritten)
 *    - HTML response including uploaded filename
 */
void HttpResponse::handlePost(const std::string& resolved,
                              const std::string& body,
                              const Location& loc,
                              const HttpRequest& req)
{
    (void)resolved;

    if (loc.upload_path.empty())
    {
        setStatusCode(405);
        setBody("<html><body><h1>405 Method Not Allowed</h1></body></html>");
        addHeader("Content-Type", "text/html");
        return;
    }

    if (loc.client_max_body_size > 0 &&
        body.size() > loc.client_max_body_size)
    {
        setStatusCode(413);
        setBody("<html><body><h1>413 Payload Too Large</h1></body></html>");
        addHeader("Content-Type", "text/html");
        return;
    }

    std::string contentType;
    const std::map<std::string, std::string>& headers = req.getHeaders();

    std::map<std::string, std::string>::const_iterator it = headers.find("Content-Type");
    if (it == headers.end())
        it = headers.find("content-type");
    if (it != headers.end())
        contentType = it->second;

    std::string fileName = "upload";
    std::string fileContent = body;

    if (contentType.find("multipart/form-data") != std::string::npos)
    {
        size_t pos = contentType.find("boundary=");
        if (pos != std::string::npos)
        {
            std::string boundary = contentType.substr(pos + 9);

            if (!boundary.empty() && boundary[0] == '"')
                boundary = boundary.substr(1, boundary.find('"', 1) - 1);

            fileName = extractFileNameFromMultipart(body, boundary);
            fileContent = extractFileContentFromMultipart(body, boundary);
        }
    }

    std::string uploadDir = loc.upload_path;

    if (uploadDir.substr(0, 2) == "./")
        uploadDir = uploadDir.substr(2);

    if (!uploadDir.empty() && uploadDir[0] != '/')
        uploadDir = loc.root + "/" + uploadDir;

    if (!uploadDir.empty() && uploadDir[uploadDir.size() - 1] != '/')
        uploadDir += "/";

    std::string fullPath = uploadDir + fileName;

    std::ofstream file(fullPath.c_str(), std::ios::out | std::ios::binary);
    if (!file.is_open())
    {
        setStatusCode(500);
        setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
        addHeader("Content-Type", "text/html");
        return;
    }

    file.write(fileContent.data(), fileContent.size());
    file.close();

    if (!file)
    {
        setStatusCode(500);
        setBody("<html><body><h1>500 Write Error</h1></body></html>");
        addHeader("Content-Type", "text/html");
        return;
    }

    setStatusCode(200);  // ← CLAVE (no 201)
    setBody("<html><body><h1>POST OK</h1><p>File: " + fileName + "</p></body></html>");
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
