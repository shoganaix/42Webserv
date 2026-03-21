/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pathResolver.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kpineda- <kpineda-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/16 12:51:20 by usuario           #+#    #+#             */
/*   Updated: 2026/03/21 21:01:30 by kpineda-         ###   ########.fr       */
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
 *    - https://github.com/shoganaix/42Webserv/wiki/Configuration-Parsing-System:-PATH-RESOLVING-CASES
 * -----------------------------------------------------------------------
 */

#include "../includes/pathResolver.hpp"

/*
 * Joins fspath segments, removes duplicated '/' & ensures result contains only one
 *  Example:
 *      joinPaths("docs/root", "file") -> "docs/root/file"
 *      joinPaths("docs/root/", "/file") -> "docs/root/file"
 */
static std::string joinPaths(const std::string &a, const std::string &b)
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
static bool endsWithSlash(const std::string &str)
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
static std::string computeRemainder(const std::string &locPath, const std::string &uriPath)
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
ResolvedPath handleRoot(const Location &loc, const std::string &uriPath)
{
	ResolvedPath out;
	out.appendIndex = false;

	std::cout << YELLOW << "Resolving path for URI: " << uriPath << " with location path: " << loc.path << RESET << std::endl;

	out.resPath = computeRemainder(loc.path, uriPath);

	out.fsPath = joinPaths(loc.root, out.resPath);

	return out;
}

ResolvedPath resolvePath(const Location &loc, const std::string &uriPath)
{
	ResolvedPath out;

	out = handleRoot(loc, uriPath);
	std::cout << BLUE << "loc: " << loc.path << " alias: " << loc.alias << RESET << std::endl;
	if (!loc.alias.empty()) // replace all with alias^
	{

		out.fsPath = loc.alias;
		if (endsWithSlash(out.fsPath))
		{
			out.fsPath = joinPaths(out.fsPath, loc.index);
			out.appendIndex = true;
		}
	}
	else if (endsWithSlash(uriPath))
	{
		out.fsPath = joinPaths(out.fsPath, loc.index);
		out.appendIndex = true;
	}

	std::cout << GREEN << "Resolved filesystem path: " << out.fsPath << RESET << std::endl;

	return out;
}