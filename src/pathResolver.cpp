/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pathResolver.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: angnavar <angnavar@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/16 12:51:20 by usuario           #+#    #+#             */
/*   Updated: 2026/04/06 10:24:52 by angnavar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*-----------------------------------------------------------------------
 *                          📍PATH RESOLVER📍
 *
 * This resolver transforms a request URI into a valid filesystem path
 *
 * Process:
 *  - Caculates the remaining URI after removing matchedlocation (REST)
 *  - Joins the REST with the location root directory
 *  - If the URI targets a directory (ends with '/') -> appends the index file
 *
 * The result determines the final filesystem path
 * -----------------------------------------------------------------------
 */

/*                ⚠️ · 🚨 · 📌 IMPORTANT ⚠️ · 🚨 · 📌
 * -----------------------------------------------------------------------
 * `matchLocation(CONFIG, URI)`-> which location governs that URI
 * `resolvePath(LOC, URI)`-> which file on disk corresponds to that URI
 *
 *  Check wiki to check & unnderstand location path resolving ❗
 *    -
 * https://github.com/shoganaix/42Webserv/wiki/Configuration-Parsing-System:-PATH-RESOLVING-CASES
 * -----------------------------------------------------------------------
 */

#include "../includes/pathResolver.hpp"

/*
 * Joins fspath segments, removes duplicated '/' & ensures result contains only one
 *  Example:
 *      joinPaths("docs/root", "file") -> "docs/root/file"
 *      joinPaths("docs/root/", "/file") -> "docs/root/file"
 */
static std::string joinPaths(const std::string& a, const std::string& b)
{
    if (a.empty())
        return b;
    if (b.empty())
        return a;

    std::string left = a;
    std::string right = b;

    while (!left.empty() && left[left.size() - 1] == '/')
        left.erase(left.size() - 1);

    while (!right.empty() && right[0] == '/')
        right.erase(0, 1);

    if (right.empty())
        return left;

    return left + "/" + right;
}

/*
 * Checks whether str ends with '/', returns TRUE if it does (its a directory)
 */
static bool endsWithSlash(const std::string& str)
{
    return (!str.empty() && str[str.size() - 1] == '/');
}

/*
 * - Defines the part of the URI that remains after removing matched location path (REST)
 * - Returns the relative path to be appended to the location root.
 *
 *  Example:
 *      loc.path="/"        uri="/tours/a"     -> "tours/a"
 *      loc.path="/tours"   uri="/tours/a"     -> "a"
 */
static std::string computeRemainder(const std::string& locPath, const std::string& uriPath)
{
    if (locPath == "/")
    {
        if (!uriPath.empty() && uriPath[0] == '/')
            return uriPath.substr(1);
        return uriPath;
    }

    if (uriPath.size() == locPath.size())
        return "";

    size_t start = locPath.size();
    if (start < uriPath.size() && uriPath[start] == '/')
        start += 1;

    if (start >= uriPath.size())
        return "";

    return uriPath.substr(start);
}

/*
 * - Resolves final fspath for a request
 * - Combines location root with rest
 * - Appends file if the request targets a directory
 *    Example:
 *        If user asks: "/tours/" -> ends with '/'-> is a directory -> attach file
 *        Server returns: "/root/tours/index.html"
 */

ResolvedPath resolvePath(const Location& loc, const std::string& uriPath)
{
    ResolvedPath out;
    out.appendIndex = false;

    out.resPath = computeRemainder(loc.path, uriPath);

    // If resPath is not empty, we're accessing a file/subfolder within the location
    // OR if the location path is "/" (the rest of the path)
    // We should combine: root + (location path minus trailing slash if exists) + resPath
    // UNLESS the location path is "/" itself
    if (!out.resPath.empty() || loc.path == "/")
    {
        if (!loc.alias.empty())
            out.fsPath = joinPaths(loc.alias, out.resPath);
        else
        {
            // For normal locations with resPath, we need to include the location folder name
            std::string folderPrefix = loc.path;
            // Remove trailing slash from location path
            while (!folderPrefix.empty() && folderPrefix[folderPrefix.size() - 1] == '/')
                folderPrefix = folderPrefix.substr(0, folderPrefix.size() - 1);
            
            if (folderPrefix != "")  // Only add if not root location
                out.fsPath = joinPaths(loc.root, joinPaths(folderPrefix, out.resPath));
            else
                out.fsPath = joinPaths(loc.root, out.resPath);
        }
    }
    else
    {
        // resPath is empty (accessing location directly like /some_folder/)
        if (!loc.alias.empty())
            out.fsPath = joinPaths(loc.alias, out.resPath);
        else
        {
            out.fsPath = joinPaths(loc.root, out.resPath);
        }
    }
    
    // If request is location, then search for index
    // (/directory, /directory/ or URI ending on '/')
    if (out.resPath.empty() || endsWithSlash(uriPath))
    {
        // If resPath is empty and location path ends with /, it means we're accessing the location directly
        // In this case, we need to append the location path to the root first, then add index
        // BUT: only do this if there's NO alias (alias already points to the correct destination)
        if (out.resPath.empty() && endsWithSlash(loc.path) && loc.path != "/" && loc.alias.empty())
        {
            // Remove trailing slash from location path and use as folder name
            std::string folderName = loc.path;
            while (!folderName.empty() && folderName[folderName.size() - 1] == '/')
                folderName = folderName.substr(0, folderName.size() - 1);
            
            // Append this folder to the root (no alias case)
            out.fsPath = joinPaths(loc.root, folderName);
        }
        
        out.fsPath = joinPaths(out.fsPath, loc.index);
        out.appendIndex = true;
    }
    
    return (out);
}
