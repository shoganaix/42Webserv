/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgiHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: root <root@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/02 18:57:01 by root              #+#    #+#             */
/*   Updated: 2026/03/02 21:49:22 by root             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP
#include "webserv.hpp"

class CgiHandler
{
    public:
        std::string execute(const std::string& interpreter, const std::string& scriptPath, const std::string& method,const std::string& body);
};

#endif
