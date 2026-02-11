/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   validation.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/11 21:35:50 by usuario           #+#    #+#             */
/*   Updated: 2026/02/11 22:06:27 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/validation.hpp"
#include "../includes/utils.hpp"        // intToString
#include <stdexcept>                    // EXCEPTIONS AND RUNTIME (std::runtime_error)
#include <set>                          // set (std::set<std::string> seen;)
#include <cstdlib>                      // atoi/atol

// *ADD METHODS* (GET, POST and DELETE mandatory)
static bool isValidMethod(const std::string& m)
{
    //return (m == "GET" || m == "POST" || m == "DELETE");
    return (m == "GET" || m == "POST" || m == "PUT" || m == "DELETE" ||
             m == "HEAD" || m == "OPTIONS" || m == "PATCH");  
}

static bool isValidHttpErrorCode(int code)
{
    return (code >= 400 && code <= 599);
}

/*
 * - Validates single location block inside server
 * - Ensures location starts with '/'
 * - Verifies that root and index are not empty AFTER inheritance (normalizeServer(cfg))
 * - Checks that all declared HTTP methods are supported
 * - Validates redirection format (must start with '/')
 * - Verifies CGI extensions and their paths (".sh" -> "/bin/bash")
 * - If any rule is violated -> throws std::runtime_error 
 */
static void validateLocation(const Location& loc, const Config& cfg)
{
    if (loc.path.empty() || loc.path[0] != '/')
        throw std::runtime_error("Invalid location path: '" + loc.path + "' (must start with '/')");
    if (loc.root.empty())
        throw std::runtime_error("Location '" + loc.path + "': root is empty (inheritance/normalization missing?)");
    if (loc.index.empty())
        throw std::runtime_error("Location '" + loc.path + "': index is empty (inheritance/normalization missing?)");

    for (size_t i = 0; i < loc.allow_methods.size(); ++i)
    {
        if (!isValidMethod(loc.allow_methods[i]))
            throw std::runtime_error("Location '" + loc.path + "': invalid method '" + loc.allow_methods[i] + "'");
    }

    if (!loc.redir.empty() && loc.redir[0] != '/')
        throw std::runtime_error("Location '" + loc.path + "': invalid return/redirection '" + loc.redir + "' (must start with '/')");

    for (std::map<std::string, std::string>::const_iterator it = loc.cgi_needs.begin();
         it != loc.cgi_needs.end(); ++it)
    {
        const std::string& ext = it->first;
        const std::string& bin = it->second;

        if (ext.empty() || ext[0] != '.')
            throw std::runtime_error("Location '" + loc.path + "': invalid CGI extension '" + ext + "' (must start with '.')");

        if (bin.empty())
            throw std::runtime_error("Location '" + loc.path + "': CGI handler path is empty for extension '" + ext + "'");
    }

    (void)cfg; // in case we want location/server cross checks
}

/*
 * - Validates a complete server
 * - Checks that listening port is within the range (1–65535)
 * - Ensures host, root, and index are not empty
 * - Validates client_max_body_size is non-negative
 * - Verifies all error_page codes are valid
 * - Detects duplicated lines using SET
 *          - SET:  container which stores unique elements (Search, insert, and delete) 
 * - Calls validateLocation() for each location block
 * - If validation fails -> throws std::runtime_error
 */
void validateServer(const Config& cfg)
{
    if (cfg.port < 1 || cfg.port > 65535)
        throw std::runtime_error("Invalid listen port: " + intToString(cfg.port) + " (must be 1..65535)");
    if (cfg.host.empty())
        throw std::runtime_error("Host is empty");
    if (cfg.client_max_body_size < 0)
        throw std::runtime_error("client_max_body_size is negative");
    if (cfg.root.empty())
        throw std::runtime_error("Server root is empty");
    if (cfg.index.empty())
        throw std::runtime_error("Server index is empty");
    for (std::map<int, std::string>::const_iterator it = cfg.error_pages.begin();
         it != cfg.error_pages.end(); ++it)
    {
        if (!isValidHttpErrorCode(it->first))
            throw std::runtime_error("Invalid error_page code: " + intToString(it->first));

        if (it->second.empty())
            throw std::runtime_error("error_page " + intToString(it->first) + ": path is empty");
    }

    std::set<std::string> seen;
    for (size_t i = 0; i < cfg.locations.size(); ++i)
    {
        const std::string& p = cfg.locations[i].path;
        if (seen.count(p))
            throw std::runtime_error("Duplicate location block for path: '" + p + "'");
        seen.insert(p);
    }

    for (size_t i = 0; i < cfg.locations.size(); ++i)
        validateLocation(cfg.locations[i], cfg);
}

void validateAllServers(const std::vector<Config>& cfgs)
{
    if (cfgs.empty())
        throw std::runtime_error("No server blocks found in configuration");

    for (size_t i = 0; i < cfgs.size(); ++i)
        validateServer(cfgs[i]);
}
