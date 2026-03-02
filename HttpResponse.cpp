/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kpineda- <kpineda-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 13:48:49 by kpineda-          #+#    #+#             */
/*   Updated: 2026/03/02 22:33:56 by kpineda-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpResponse.hpp"

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
	if ( !statusMessages.empty() )
		return;
	statusMessages[200] = "OK";
	statusMessages[404] = "Not Found";
	statusMessages[403] = "Forbidden";
	statusMessages[500] = "Internal Server Error";
}

void HttpResponse::initializeMimeTypes()
{
	if ( !mimeTypes.empty() )
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

void HttpResponse::addHeader(const std::string& key, const std::string& value)
{
	headers[key] = value;
}

void HttpResponse::setBody(const std::string& body)
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
			if (name == ".") continue;

			if (name[0] == '.' && name != "..") continue;
			
			html << "<li><a href=\"" << name;

			if (name == ".." || entry->d_type == DT_DIR)
				html <<"/";
				
			html << "\">" << name << "</a></li>";
		}
		closedir(dir);
	}
	else
		return "<html><body><h1>Error al leer el directorio</h1></body></html>";
	html << "</ul><hr></body></html>";
	return html.str();	
}

void HttpResponse::loadFile(const std::string& path)
{
	struct stat path_stat;
	// Verify if the file exists and is a regular file
	if (stat(path.c_str(), &path_stat) != 0)
	{
		// File doesn't exist or can't be accessed
		setStatusCode(404);
		setBody("<html><body><h1>404 Not Found</h1><p>The requested resource was not found on this server.</p></body></html>");
		addHeader("Content-Type", "text/html");
		return;
	}
	// Is it a directory?
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
	// It's a regular file, try to read it
	if (S_ISREG(path_stat.st_mode))
	{
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
			setStatusCode (403);
			setBody("<html><body><h1>403 Forbidden</h1><p>You don't have permission to access this resource.</p></body></html>");
			addHeader("Content-Type", "text/html");
		}
	}
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
	response << "\r\n" << body;
	return response.str();
}