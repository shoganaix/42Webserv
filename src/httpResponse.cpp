/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   httpResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kpineda- <kpineda-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 13:48:49 by kpineda-          #+#    #+#             */
/*   Updated: 2026/03/21 21:37:31 by kpineda-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "httpResponse.hpp"
#include "pathResolver.hpp"
#include "colors.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// Static member definitions
std::map<int, std::string> HttpResponse::statusMessages;
std::map<std::string, std::string> HttpResponse::mimeTypes;

HttpResponse::HttpResponse() : version("HTTP/1.1"), statusCode(200), body("")
{
	initializeStatusMessages();
	initializeMimeTypes();
}

HttpResponse::~HttpResponse()
{
}

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

void HttpResponse::setStatusCode(int code)
{
	statusCode = code;
}

int HttpResponse::getStatusCode() const
{
	return (statusCode);
}

void HttpResponse::addHeader(const std::string &key, const std::string &value)
{
	headers[key] = value;
}

void HttpResponse::setBody(const std::string &body)
{
	this->body = body;

	std::stringstream ss;
	ss << body.length();
	addHeader("Content-Length", ss.str());
}

void HttpResponse::clear()
{
	version = "HTTP/1.1";
	statusCode = 200;
	body.clear();
	headers.clear();
}

std::string HttpResponse::toLower(std::string s)
{
	for (size_t i = 0; i < s.length(); ++i)
		s[i] = std::tolower(static_cast<unsigned char>(s[i]));
	return s;
}

std::string HttpResponse::generateAutoIndex(const std::string &path)
{
	DIR *dir;
	struct dirent *entry;
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

void HttpResponse::loadFile(const std::string &path)
{
	struct stat path_stat;
	std::cout << GREEN << path << RESET << std::endl;

	// Verify if the file exists and is a regular file
	if (stat(path.c_str(), &path_stat) != 0)
	{
		// File doesn't exist or can't be accessed
		setStatusCode(404);
		setBody("<html><body><h1>404 Not Found</h1><p>The requested resource was not found on this server.</p></body></html>");
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
	// NO NEED IF WE USE PATH RESOLVER
	//  Is it a directory?
	/*
	if (S_ISDIR(path_stat.st_mode))
	{
		std::string index = path + "/index.html";
		struct stat index_stat;
		if (stat(index.c_str(), &index_stat) == 0 && S_ISREG(index_stat.st_mode))
			return loadFile(index);
		setStatusCode(200);
		setBody(generateAutoIndex(path));
		addHeader("Content-Type", "text/html");
		return;
	}
	*/
	// It's a regular file, try to read it
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
		setBody("<html><body><h1>403 Forbidden</h1><p>You don't have permission to access this resource.</p></body></html>");
		addHeader("Content-Type", "text/html");
	}
}

void HttpResponse::handleGet(const std::string &resolved, const Location &loc)
{
	//-----MOVED TO ROUTE
	/*
	bool allowed = false;
	for (size_t i = 0; i < loc.allow_methods.size(); ++i)
	{
		if (loc.allow_methods[i] == "GET")
		{
			allowed = true;
			break;
		}
	}

	if (!allowed)
	{
		setStatusCode(405);
		setBody("<html><body><h1>405 Method Not Allowed</h1><p>GET method is not allowed for this resource.</p></body></html>");
		addHeader("Content-Type", "text/html");
		return;
	}
	*/

	//----- Assumes rout is always loc.rooth + url
	// std::string fullPath = loc.root + url;
	// loadFile(fullPath);
	//----- Uses resolvedPath
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
		if (loc.autoindex)
		{
			setStatusCode(200);
			setBody(generateAutoIndex(resolved));
			addHeader("Content-Type", "text/html");
			return;
		}

		setStatusCode(403);
		setBody("<html><body><h1>403 Forbidden</h1></body></html>");
		addHeader("Content-Type", "text/html");
		return;
	}

	loadFile(resolved);
}

void HttpResponse::handleDelete(const std::string &resolved, const Location &loc)
{
	(void)loc; // Avoid unused parameter warning
	//-----MOVED TO ROUTE
	/*
	bool allowed = false;
	for (size_t i = 0; i < loc.allow_methods.size(); ++i)
	{
		if (loc.allow_methods[i] == "DELETE")
		{
			allowed = true;
			break;
		}
	}

	if (!allowed)
	{
		setStatusCode(405);
		setBody("<html><body><h1>405 Method Not Allowed</h1><p>DELETE method is not allowed for this resource.</p></body></html>");
		addHeader("Content-Type", "text/html");
		return;
	}
	*/

	/*----- Lorena's old Path resolver
	std::string root = loc.root;
	std::string path = url;

	if (!root.empty() && root[root.length() - 1] == '/' && !path.empty() && path[0] == '/')
		path = path.substr(1);
	else if (!root.empty() && root[root.length() - 1] != '/' && !path.empty() && path[0] != '/')
		root += "/";

	std::string fullPath = root + path;
	*/

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
		setBody("<html><body><h1>403 Forbidden</h1><p>No se pueden borrar directorios.</p></body></html>");
		addHeader("Content-Type", "text/html");
		return;
	}
	else if (unlink(fullPath.c_str()) == 0)
	{
		setStatusCode(200);
		setBody("<html><body><h1>File Deleted</h1><p>El recurso ha sido eliminado.</p></body></html>");
		addHeader("Content-Type", "text/html");
	}
	else
	{
		setStatusCode(500);
		setBody("<html><body><h1>500 Internal Server Error</h1><p>No se pudo borrar el archivo.</p></body></html>");
		addHeader("Content-Type", "text/html");
	}
}

bool HttpResponse::savePostFile(const std::string &uploadPath, const std::string &body, const std::string &filename)
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

void HttpResponse::handlePost(const std::string &body, const Location &loc)
{
	//-----MOVED TO ROUTE
	/*
	bool allowed = false;
	for (size_t i = 0; i < loc.allow_methods.size(); ++i)
	{
		if (loc.allow_methods[i] == "POST")
		{
			allowed = true;
			break;
		}
	}
	if (!allowed)
	{
		setStatusCode(405);
		setBody("<html><body><h1>405 Method Not Allowed</h1><p>POST method is not allowed for this resource.</p></body></html>");
		addHeader("Content-Type", "text/html");
		return;
	}
	if (body.length() > maxSize)
	{
		setStatusCode(413);
		setBody("<html><body><h1>413 Payload Too Large</h1><p>The uploaded data exceeds the maximum allowed size.</p></body></html>");
		addHeader("Content-Type", "text/html");
		return;
	}
	*/
	std::stringstream fileName;
	fileName << "upload_" << time(0) << ".txt";

	std::string path = loc.upload_path.empty() ? "." : loc.upload_path;

	if (savePostFile(path, body, fileName.str()))
	{
		setStatusCode(201);
		setBody("<html><body><h1>201 Created</h1><p>File uploaded successfully.</p></body></html>");
		addHeader("Content-Type", "text/html");
	}
	else
	{
		setStatusCode(500);
		setBody("<html><body><h1>500 Internal Server Error</h1><p>Failed to save the uploaded file.</p></body></html>");
		addHeader("Content-Type", "text/html");
	}
}

void HttpResponse::setRedirect(const std::string &location, int code)
{
	clear();
	setStatusCode(code);
	addHeader("Location", location);
	setBody("<html><body><h1>Redirecting...</h1><p>You are being redirected to <a href=\"" + location + "\">" + location + "</a>.</p></body></html>");
	addHeader("Content-Type", "text/html");
}

std::string HttpResponse::toString() const
{
	std::stringstream response;

	std::string msg = "Unknown Status";

	if (statusMessages.find(statusCode) != statusMessages.end())
		msg = statusMessages.at(statusCode);

	response << version << " " << statusCode << " " << msg << "\r\n";

	std::map<std::string, std::string>::const_iterator it;
	for (it = headers.begin(); it != headers.end(); ++it)
		response << it->first << ": " << it->second << "\r\n";
	response << "\r\n"
			 << body;
	return response.str();
}
