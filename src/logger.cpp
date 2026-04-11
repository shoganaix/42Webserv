/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: macastro <macastro@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/10 00:00:00 by usuario           #+#    #+#             */
/*   Updated: 2026/04/11 15:02:35 by macastro         ###   ########.fr       */
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

void logDebug(const std::string& msg)
{
    if (!DEBUG)
        return;

    printTimestamped("", msg);
}

void logDebug(const std::string& color, const std::string& msg)
{
    if (!DEBUG)
        return;

    printTimestamped(color, msg);
}

void logInfo(const std::string& msg) { printTimestamped(BLUE, msg); }

void logInfo(const std::string& color, const std::string& msg) { printTimestamped(color, msg); }
