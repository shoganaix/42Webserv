/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msoriano <msoriano@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/10 20:21:20 by msoriano          #+#    #+#             */
/*   Updated: 2025/12/10 20:22:19 by msoriano         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>

enum LogState { OFF = 0, ON = 1 };
enum Output { CONSOLE_OUTPUT = 0 };

class Logger
{
public:
    static void setState(LogState state) { _state = state; }

    static void logMsg(const std::string &color, Output dest, const std::string &msg)
    {
        if (_state == ON)
        {
            if (dest == CONSOLE_OUTPUT)
                std::cout << color << msg << "\033[0m" << std::endl;
        }
    }

private:
    static LogState _state;
};

LogState Logger::_state = ON;

#endif
