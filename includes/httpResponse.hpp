/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   httpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 13:48:43 by kpineda-          #+#    #+#             */
/*   Updated: 2026/04/15 14:42:06 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <sstream>
#include <fstream>
#include <string>
#include <cstdio>
#include <ctime>
#include <map>
#include <vector>

struct Location
{
    // In C++98, if you don't define a constructor, those fields can remain undefined
    //  (Memory garbage → random errors)
    Location()
        : autoindex(false), path("/"), root(""), alias(""), index(""), redir(""), upload_path(""),
          client_max_body_size(0)
    {
    }

    bool autoindex;
    std::string path;
    std::string root;
    std::string alias;
    std::string index;
    std::string redir;
    std::string upload_path;
    size_t client_max_body_size;
    std::vector<std::string> allow_methods;
    std::map<std::string, std::string> cgi_needs;
};

class HttpResponse
{
    // Response components
    bool _isCgi;
    std::string version; // "HTTP/1.1"
    int statusCode;      // 200, 404, etc.
    std::string body;
    std::map<std::string, std::string> headers; // Map of header key-value pairs

    // Static maps for status messages and MIME types
    static std::map<int, std::string> statusMessages;    // Map of status codes to messages
    static std::map<std::string, std::string> mimeTypes; // Map of file extensions to MIME types

    // Static initialization methods
    static void initializeMimeTypes();      // Initialize the MIME type mapping
    static void initializeStatusMessages(); // Initialize the status code to message mapping

  public:
    HttpResponse();
    ~HttpResponse();

    // setters, getters and utility methods
    void setStatusCode(int code);
    int getStatusCode() const;
    void setBody(const std::string& body);
    const std::string& getBody() const { return body; }
    void addHeader(const std::string& key, const std::string& value);
    void handleGet(const std::string& path);
    void clear();

    // File loading method
    void loadFile(const std::string& path);
    // Generate AutoIndex
    std::string generateAutoIndex(const std::string& path);
    void handleGet(const std::string& url, const Location& loc);

    // DELETE handling method
    void handleDelete(const std::string& url, const Location& loc);

    // POST handling method
    bool savePostFile(const std::string& uploadPath, const std::string& body,
                      const std::string& filename);
    // void handlePost(const std::string &body, const Location &loc);
    void handlePost(const std::string& resolved, const std::string& body, const Location& loc);

    void setRedirect(const std::string& location, int code);

    // method to convert response to string format
    // std::string toString() const;
    // new function also includes HEAD
    std::string toString(bool omitBody = false) const;

    void setIsCgi(bool value) { _isCgi = value; }
    bool getIsCgi() const { return _isCgi; }
};

#endif
