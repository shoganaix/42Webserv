/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   matchLocation.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: macastro <macastro@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/14 13:51:36 by msoriano          #+#    #+#             */
/*   Updated: 2026/04/09 20:50:31 by macastro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*-----------------------------------------------------------------------
 *                          📍MATCH LOCATION ALGORITHM📍
 *
 * Responsible for selecting the correct location block for HTTP request
 *
 * 'LONGEST PREFIX MATCH' strat:
 *  - Compares the request URI against all locations
 *  - Selects the location whose path matches the beginning of URI
 *  - If multiple matches exist-> chooses the most specific (longest matching path)
 *
 * The location determines: Root directory, Allowed HTTP methods, Redirections,
 *  CGI execution rules and Upload behavior
 * -----------------------------------------------------------------------
 */

/*             ⚠️ · 🚨 · 📌 IMPORTANT ⚠️ · 🚨 · 📌
 * -----------------------------------------------------------------------
 * `matchLocation(CONFIG, URI)`-> which location governs that URI
 * `resolvePath(LOC, URI)`-> which file on disk corresponds to that URI
 * -----------------------------------------------------------------------
 */

#include "../includes/matchLocation.hpp"

/**
 * Checks if a location path matches a request URI path.
 *
 * Ejemplos:
 *   - path /images matches location /images and /images/
 *   - path /image does not match location /images
 *   - path /images/pic.jpg matches location /images/
 *
 * @param locationPath: The path defined in the location block (e.g., "/tours")
 * @param uriPath: The path from the HTTP request (e.g., "/tours/asia")
 *
 * @returns true if the location matches the URI, false otherwise
 */
bool locationMatches(const std::string& locationPath, const std::string& uriPath)
{
    if (locationPath == "/")
        return true; // root matches everything
    // Normaliza: quita barra final de locationPath y uriPath si la tienen (excepto si son solo "/")
    std::string norm_localPath = locationPath;
    std::string norm_uriPath = uriPath;
    if (norm_localPath.length() > 1 && norm_localPath[norm_localPath.length() - 1] == '/')
        norm_localPath.erase(norm_localPath.length() - 1);
    if (norm_uriPath.length() > 1 && norm_uriPath[norm_uriPath.length() - 1] == '/')
        norm_uriPath.erase(norm_uriPath.length() - 1);
    // Coincidencia exacta
    if (norm_uriPath == norm_localPath)
        return true;
    // uriPath es subdirectorio o archivo dentro de locationPath
    if (norm_uriPath.size() > norm_localPath.size() &&
        norm_uriPath.compare(0, norm_localPath.size(), norm_localPath) == 0 &&
        norm_uriPath[norm_localPath.size()] == '/')
        return true;
    return false;
}

/**
 * Matches a URI path to the most specific location configuration.
 *
 * - Iterates through all configured locations to find the best match for uriPath
 *
 * - For each location, checks if paths match:
 *      If it matches-> Compares path length to select the most specific (longest)
 *
 * @param cfg: Server configuration containing all location blocks
 * @param uriPath: The path from the HTTP request to match (e.g., "/tours")
 *
 * @returns a pointer to the best matching location, NULL if none
 *
 */
const Location* matchLocation(const Config& cfg, const std::string& uriPath)
{
    const Location* best = NULL;
    size_t bestLen = 0;

    for (size_t i = 0; i < cfg.locations.size(); i++)
    {
        const Location& loc = cfg.locations[i];

        if (locationMatches(loc.path, uriPath))
        {
            if (loc.path.size() > bestLen)
            {
                best = &loc;
                bestLen = loc.path.size();
            }
        }
    }
    return (best);
}
