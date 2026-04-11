/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: macastro <macastro@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/10 20:21:20 by msoriano          #+#    #+#             */
/*   Updated: 2026/04/11 13:40:52 by macastro         ###   ########.fr       */
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

#define LOG_BUFFER_SIZE 4096
#define DEBUG 1

enum Output
{
    CONSOLE_OUTPUT,
    FILE_OUTPUT
};

void logDebug(const char* fmt, ...);
void logDebug(const std::string& color, const char* fmt, ...);

#endif
