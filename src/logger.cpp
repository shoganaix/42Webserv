/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: macastro <macastro@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/10 00:00:00 by usuario           #+#    #+#             */
/*   Updated: 2026/04/11 13:40:44 by macastro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "logger.hpp"

void printTimestamped(const std::string& color, const std::string& msg)
{
    std::time_t now = std::time(NULL);
    std::tm* local = std::localtime(&now);
    char timestamp[20] = {0};

    if (local)
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", local);
    else
        std::snprintf(timestamp, sizeof(timestamp), "%s", "1970-01-01 00:00:00");

    std::cout << color << "[" << timestamp << "] " << msg << RESET << std::endl;
}

void formatAndPrint(const std::string& color, const char* fmt, va_list args)
{
    char msg[LOG_BUFFER_SIZE] = {0};
    std::vsnprintf(msg, sizeof(msg), fmt, args);
    printTimestamped(color, msg);
}

void logDebug(const char* fmt, ...)
{
    if (!DEBUG)
        return;

    va_list args;
    va_start(args, fmt);
    formatAndPrint("", fmt, args);
    va_end(args);
}

void logDebug(const std::string& color, const char* fmt, ...)
{
    if (!DEBUG)
        return;

    va_list args;
    va_start(args, fmt);
    formatAndPrint(color, fmt, args);
    va_end(args);
}
