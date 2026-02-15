/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   configParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msoriano <msoriano@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:44:57 by usuario           #+#    #+#             */
/*   Updated: 2026/02/14 13:52:19 by msoriano         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <iostream>
#include <vector>
#include <cstdlib>      //calling aoi/atol

#include "webserv.hpp"
#include "tokenizer.hpp"

class ConfigParser 
{
    public:
        //using vector to prepare multiple config files
        std::vector<Config> parse(const std::string& path);

    private:
        std::vector<Token> tokens;
        size_t pos;

        Token& current();
        bool accept(const std::string& v);
        void expect(const std::string& v);

        void parseServer(Config& cfg);
        void parseServerDirective(Config& cfg);
        void parseLocation(Config& cfg);
};

#endif
