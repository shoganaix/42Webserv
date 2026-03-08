/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kpineda- <kpineda-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/21 00:49:35 by usuario           #+#    #+#             */
/*   Updated: 2026/03/08 13:13:48 by kpineda-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
#define UTILS_HPP

#include "webserv.hpp"

std::string intToString(int n);
void safeClose(int fd);
std::vector<char*> vecToCharPtr(std::vector<std::string>& src);

#endif
