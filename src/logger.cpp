/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/10 00:00:00 by usuario           #+#    #+#             */
/*   Updated: 2026/04/15 14:43:04 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*-----------------------------------------------------------------------
 *                      🧾LOGGER SYSTEM🧾
 *
 * This module provides simple logging utilities with timestamp support
 * Used to print formatted messages to standard output for:
 *  - Debug messages (conditional via DEBUG flag)
 *  - Informational logs
 *  - Colored outputs
 *
 * This helps tracking server behavior and debugging runtime issues.
 *-----------------------------------------------------------------------*/
#include "logger.hpp"

/* Prints message with timestamp and optional color
 * Format: [YYYY-MM-DD HH:MM:SS] message
 * Falls back to default timestamp if localtime fails
 */
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

/* Prints debug message if DEBUG flag is enabled */
void logDebug(const std::string& msg)
{
    if (!DEBUG)
        return;

    printTimestamped("", msg);
}

/* Prints colored debug message if DEBUG flag is enabled */
void logDebug(const std::string& color, const std::string& msg)
{
    if (!DEBUG)
        return;

    printTimestamped(color, msg);
}

/* Prints informational message (default blue color) */
void logInfo(const std::string& msg) { printTimestamped(BLUE, msg); }

/* Prints informational message with custom color */
void logInfo(const std::string& color, const std::string& msg) { printTimestamped(color, msg); }
