/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tokenizer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msoriano <msoriano@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:46:08 by usuario           #+#    #+#             */
/*   Updated: 2026/02/14 13:34:11 by msoriano         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*-----------------------------------------------------------------------
 *                          🌪️CONFIG TOKENIZER🌪️
 *
 * This tokenizer converts the raw .conf file into meaningful tokens 
 * It removes irrelevant characters (spaces or #) and separates 
 * special characters 
 * ('{' ,  '}' ,  ';')
 * 
 * This step simplifies logic and saves it into our token struct to
 * operate on a clean and normalized token stream
 * -----------------------------------------------------------------------
 */

#include "../includes/tokenizer.hpp"

std::vector<Token> Tokenizer::tokenize(const std::string& path)
{
    std::ifstream file(path.c_str());
    if (!file.is_open())
        throw (std::runtime_error("Cannot open config file"));

    std::vector<Token> tokens;
    std::string line;
    int i = 0;

    while (std::getline(file, line)) 
    {
        i++;
        std::string current;

        for (size_t j = 0; j < line.size(); j++) 
        {
            char c = line[j];

            if (c == '#')
                break;

            if (std::isspace(c)) 
            {
                if (!current.empty()) 
                {
                    tokens.push_back(Token(current, i));
                    current.clear();
                }
            }
            else if (c == '{' || c == '}' || c == ';') 
            {
                if (!current.empty()) 
                {
                    tokens.push_back(Token(current, i));
                    current.clear();
                }
                tokens.push_back(Token(std::string(1, c), i));
            }
            else 
                current += c;
        }
        if (!current.empty())
            tokens.push_back(Token(current, i));
    }
    return (tokens);
}