/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msoriano <msoriano@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/10 20:21:23 by msoriano          #+#    #+#             */
/*   Updated: 2025/12/10 20:22:23 by msoriano         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <exception>
#include <iostream>

class Webserv
{
    public:
        Webserv(const std::string &configFile)
        {
            std::cout << "Webserv initialized with config: " << configFile << std::endl;
        }

        void run()
        {
            std::cout << "Webserv running..." << std::endl;
        }

        ~Webserv() {}
};

#endif
