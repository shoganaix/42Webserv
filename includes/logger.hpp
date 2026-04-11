/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: macastro <macastro@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/10 20:21:20 by msoriano          #+#    #+#             */
/*   Updated: 2026/04/11 15:02:59 by macastro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <ctime>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include "colors.hpp"

#define DEBUG 0

enum Output
{
    CONSOLE_OUTPUT,
    FILE_OUTPUT
};

void logDebug(const std::string& msg);
void logDebug(const std::string& color, const std::string& msg);
void logInfo(const std::string& msg);
void logInfo(const std::string& color, const std::string& msg);

#endif
