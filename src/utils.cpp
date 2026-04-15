/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/21 00:44:52 by usuario           #+#    #+#             */
/*   Updated: 2026/04/15 14:40:48 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*-----------------------------------------------------------------------
 *                          🔧UTILITY FUNCTIONS🔧
 *
 * This file contains helper functions used to simplify common operations
 *
 * Index:
 *  - intToString(): safely converts an integer into a string
 *  - safeClose(int fd): safely closes fds, checxking beforehand if its
 *      valid (>= 0)
 *  - vecToCharPtr(std::vector<std::string>& src): converts a vector of
 *      strings into a NULL-terminated array of char *
 *  - getExtension(const std::string& path): extracts file extension
 *      from path (including '.')
 *  - isCgiRequest(const Location& loc, const std::string& fsPath):
 *      checks whether a request should be handled as CGI
 *       1. Extracts file extension
 *       2. Looks up extension in location.cgi_needs map
 *       3. Returns true if CGI handler exists
 *  - toLower(std::string s): converts string to lowercase
 * -----------------------------------------------------------------------
 */

#include "../includes/utils.hpp"

std::string intToString(int n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

void safeClose(int fd)
{
    if (fd >= 0)
        close(fd);
}

std::vector<char*> vecToCharPtr(std::vector<std::string>& src)
{
    std::vector<char*> out;
    for (size_t i = 0; i < src.size(); ++i)
        out.push_back(const_cast<char*>(src[i].c_str()));
    out.push_back(NULL);
    return (out);
}

std::string getExtension(const std::string& path)
{
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return "";
    return path.substr(dot);
}

bool isCgiRequest(const Location& loc, const std::string& fsPath)
{
    std::string ext = getExtension(fsPath);
    if (ext.empty())
        return false;

    std::map<std::string, std::string>::const_iterator it = loc.cgi_needs.find(ext);
    return (it != loc.cgi_needs.end());
}

std::string toLower(std::string s)
{
    for (size_t i = 0; i < s.length(); ++i)
        s[i] = std::tolower(static_cast<unsigned char>(s[i]));
    return s;
}
