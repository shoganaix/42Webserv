/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kpineda- <kpineda-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/21 00:49:35 by usuario           #+#    #+#             */
/*   Updated: 2026/04/05 00:56:40 by kpineda-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
#define UTILS_HPP

#include "webserv.hpp"

std::string intToString(int n);
void safeClose(int fd);
std::vector<char*> vecToCharPtr(std::vector<std::string>& src);
std::string getExtension(const std::string& path);
bool isCgiRequest(const Location& loc, const std::string& fsPath);
template <typename T>

std::string to_string(T value)
{
    std::ostringstream os;
    os << value;
    return os.str();
}

#endif
