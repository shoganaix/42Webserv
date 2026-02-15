/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   matchLocation.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msoriano <msoriano@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/14 13:51:36 by msoriano          #+#    #+#             */
/*   Updated: 2026/02/15 12:16:46 by msoriano         ###   ########.fr       */
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

 #include "../includes/matchLocation.hpp"

 /* 
 * Checks whether str >= prefix and contains with the specified prefix
 * If str starts with prefix- > returns TRUE, otherwise FALSE
 */
static bool startsWith(const std::string &str, const std::string &prefix)
{
    return(str.size() >= prefix.size() && str.compare(0, prefix.size(),prefix) == 0);
}
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
    if(!startsWith(uriPath,localPath))
        return (false);
    if(localPath == "/")
        return (true);
    if(uriPath.size() == localPath.size())
        return (true);
    return (uriPath[localPath.size()] ==  '/');
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