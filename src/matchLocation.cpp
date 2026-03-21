/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   matchLocation.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kpineda- <kpineda-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/14 13:51:36 by msoriano          #+#    #+#             */
/*   Updated: 2026/03/21 20:46:32 by kpineda-         ###   ########.fr       */
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

 /* 
 * - Checks if uriPath starts with localPath: startsWith()
 *   - Otherwise, FALSE
 * - If localPath is '/', matches everything -> returns TRUE
 * - If uri and loc have = length -> exact match -> returns TRUE
 *   - Otherwise, verifies that next character in uri is '/' to ensure a valid directory
 *      - If so -> returns TRUE, otherwise FALSE
 *
 */
static bool locationMatches(const std::string &uriPath, const std::string &localPath)
{
    // Normaliza: quita barra final de localPath y uriPath si la tienen (excepto si son solo "/")
    std::string norm_localPath = localPath;
    std::string norm_uriPath = uriPath;
	if (localPath == "/") return true; // root matches everything
    if (norm_localPath.length() > 1 && norm_localPath[norm_localPath.length() - 1] == '/')
        norm_localPath.erase(norm_localPath.length() - 1);
    if (norm_uriPath.length() > 1 && norm_uriPath[norm_uriPath.length() - 1] == '/')
        norm_uriPath.erase(norm_uriPath.length() - 1);
    // Coincidencia exacta
    if (norm_uriPath == norm_localPath)
        return true;
    // uriPath es subdirectorio o archivo dentro de localPath
    if (norm_uriPath.size() > norm_localPath.size() && norm_uriPath.compare(0, norm_localPath.size(), norm_localPath) == 0 && norm_uriPath[norm_localPath.size()] == '/')
        return true;
    return false;
}

 /* 
 * - Iterates through all configured locations to find the best match for uriPath
 * - For each location, checks if paths match: locationMatches()
 *      If it matches-> Compares path length to select the most specific (longest)
 * 
 * Returns a pointer to the best matching location, NULL if none
 */
const Location* matchLocation(const Config &cfg, const std::string &uriPath)
{
    const Location* best = NULL;
    size_t bestLen = 0;
    
    for(size_t i = 0; i < cfg.locations.size(); i++)
    {
        const Location &loc = cfg.locations[i];

        if(locationMatches(uriPath, loc.path))
        {
            if(loc.path.size() > bestLen)
            {
                best = &loc;
                bestLen = loc.path.size();
            }
        }
    }
    return (best);
}
