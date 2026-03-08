/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kpineda- <kpineda-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/21 00:44:52 by usuario           #+#    #+#             */
/*   Updated: 2026/03/08 13:14:20 by kpineda-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*-----------------------------------------------------------------------
 *                          🔧UTILITY FUNCTIONS🔧
 *
 * This file contains helper functions used to simplify common operations
 *
 * Index:
 *  - intToString(): safely converts an integer into a string
 *  - safeClose(int fd): safely closes fds, checxking beforehand if its 
 *      valid (>= 0)
 *  - vecToCharPtr(std::vector<std::string>& src): converts a vector of 
 *      strings into a NULL-terminated array of char *
 * -----------------------------------------------------------------------
 */

#include "../includes/utils.hpp"

std::string intToString(int n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

void safeClose(int fd)
{
	if (fd >= 0)
		close(fd);
}

std::vector<char*> vecToCharPtr(std::vector<std::string>& src)
{
	std::vector<char*> out;
	for (size_t i = 0; i < src.size(); ++i)
		out.push_back(const_cast<char*>(src[i].c_str()));
	out.push_back(NULL);
	return (out);
}