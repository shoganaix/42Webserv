/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tokenizer.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msoriano <msoriano@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/21 00:17:06 by usuario           #+#    #+#             */
/*   Updated: 2026/02/14 14:03:11 by msoriano         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include <vector>
#include <string>
#include <iostream>
#include <fstream>

struct Token 
{
    std::string value;
    int line;

    Token(const std::string& v, int l) : value(v), line(l) {}
};

class Tokenizer 
{
    public:
        static std::vector<Token> tokenize(const std::string& path);
};

#endif