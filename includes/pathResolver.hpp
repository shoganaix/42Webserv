/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pathResolver.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/16 12:51:20 by usuario           #+#    #+#             */
/*   Updated: 2026/02/16 13:33:50 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PATHRESOLVER_HPP
#define PATHRESOLVER_HPP

#include "webserv.hpp"

struct ResolvedPath
{
    std::string fsPath;     // FILESYSTEM path (root + resPath)
    std::string resPath;    // REST path (URI part after location prefix)
    bool appendIndex;       // true if URI ends with '/' -> then, appended loc.index
};

ResolvedPath resolvePath(const Location& loc, const std::string& uriPath);

#endif
