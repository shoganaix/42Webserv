/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/21 00:49:35 by usuario           #+#    #+#             */
/*   Updated: 2026/03/19 19:11:12 by usuario          ###   ########.fr       */
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

#endif
