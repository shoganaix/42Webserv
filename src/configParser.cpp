/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   configParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/21 00:25:24 by usuario           #+#    #+#             */
/*   Updated: 2026/01/21 01:29:21 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*-----------------------------------------------------------------------
 *                           🌪️CONFIG PARSER🌪️
 *
 * The ConfigParser is responsible for syntactic and semantic analysis
 * of the tokenized conf file
 * 
 * It CONSUMES the token stream produced earlier (on TOKENIZER)
 * and fills the Config and Location structs with the parsed values
 * 
 * It also ensures that directives are used in valid contexts
 * (e.g. location blocks inside server blocks)
 *
 * Any invalid or unknown directive results in a configuration error
 * -----------------------------------------------------------------------
 */

#include "../includes/configParser.hpp"
#include "../includes/tokenizer.hpp"
#include "../includes/utils.hpp"

/*
 * Returns the current token pointed by the parser cursor
 * If parser tries to read beyond list = configuration ended unexpectedly = exception 
 */
Token& ConfigParser::current() 
{
    if (pos >= tokens.size())
        throw (std::runtime_error("Unexpected end of file"));
    return (tokens[pos]);
}

/*
 * Checks if the current token matches the expected value
 *      - If it matches-> parser advances to next token then returns true
 *      - If it DOESNT match-> nothing is consumed and false is returned
 * 
 *      ++++++++ Used for OPTIONAL or branching GRAMMAR RULES ++++++++
 */
bool ConfigParser::accept(const std::string& v)
{
    if (current().value == v)
    {
        pos++;
        return (true);
    }
    return (false);
}
/*
 * Ensures that the current token matches the expected value = STRICT version of accept()
 *      - If it DOESNT match-> THROWS EXCEPTION
 * 
 *    ++++++++ Used when the GRAMMAR REQUIRES a SPECIFIC token/rule ++++++++
 */ 
void ConfigParser::expect(const std::string& v)
{
    if (!accept(v))
        throw (std::runtime_error("Expected '" + v + "' at line " + intToString(current().line)));
}

/*
 * Parses a SINGLE SERVER-level directive
 *
 *     Supported directives include:
 *         - listen
 *         - host
 *         - root
 *         - index
 *         - server_name
 *         - client_max_body_size
 *         - error_page
 *
 * Unknown directive -> configuration error
 */
void ConfigParser::parseServerDirective(Config& cfg)
{
    if (accept("listen"))
    {
        cfg.port = std::atoi(current().value.c_str());
        pos++; 
        expect(";");
    }
    else if (accept("host"))
    {
        cfg.host = current().value;
        pos++; 
        expect(";");
    }
    else if (accept("root"))
    {
        cfg.root = current().value;
        pos++; 
        expect(";");
    }
    else if (accept("index"))
    {
        cfg.index = current().value;
        pos++; 
        expect(";");
    }
    else if (accept("server_name"))
    {
        cfg.server_name = current().value;
        pos++; 
        expect(";");
    }
    else if (accept("client_max_body_size"))
    {
        cfg.client_max_body_size = std::atol(current().value.c_str());
        pos++; 
        expect(";");
    }
    else if (accept("error_page"))
    {
        int code = std::atoi(current().value.c_str());
        pos++;
        cfg.error_pages[code] = current().value;
        pos++; 
        expect(";");
    }
    else
        throw (std::runtime_error("Unknown directive '" + current().value + "' at line " + intToString(current().line)));
}

/*
 * Parses a SINGLE LOCATION block
 *
 *     Supported directives inside a location include:
 *         - autoindex
 *         - root
 *         - index
 *         - return
 *         - allow_methods
 *         - cgi_path / cgi_ext
 *
 * Unknown directive -> configuration error
 */
void ConfigParser::parseLocation(Config& cfg)
{
    Location loc;

    expect("location");
    loc.path = current().value;
    pos++; 
    expect("{");

    while (!accept("}"))
    {
        if (accept("autoindex"))
        {
            loc.autoindex = (current().value == "on");
            pos++; 
            expect(";");
        }
        else if (accept("root"))
        {
            loc.root = current().value;
            pos++;
            expect(";");
        }
        else if (accept("index"))
        {
            loc.index = current().value;
            pos++;
            expect(";");
        }
        else if (accept("return"))
        {
            loc.redir = current().value;
            pos++;
            expect(";");
        }
        else if (accept("allow_methods"))
        {
            while (current().value != ";") 
            {
                loc.allow_methods.push_back(current().value);
                pos++;
            }
            expect(";");
        }
        else if (accept("cgi_path")) 
        {
            std::vector<std::string> paths;
            while (current().value != ";")
            {
                paths.push_back(current().value);
                pos++;
            }
            expect(";");

            expect("cgi_ext");
            std::vector<std::string> exts;
            while (current().value != ";")
            {
                exts.push_back(current().value);
                pos++;
            }
            expect(";");

            if (paths.size() != exts.size())
                throw (std::runtime_error("cgi_path / cgi_ext mismatch"));

            for (size_t i = 0; i < paths.size(); i++)
                loc.cgi_needs[exts[i]] = paths[i];
        }
        else 
            throw (std::runtime_error("Unknown location directive '" + current().value + "' at line " + intToString(current().line)));
    }
    cfg.locations.push_back(loc);
}

/*
 * Parses the main SERVER BLOCK until '}' is found
 */
void ConfigParser::parseServer(Config& cfg)
{
    expect("server");
    expect("{");

    while (!accept("}")) 
    {
        if (current().value == "location")
            parseLocation(cfg);
        else
            parseServerDirective(cfg);
    }
}

/*
 * - Tokenizes the configuration file
 * - Resets the parser cursor
 * - Parses the server block
 * - Returns a fully populated Config structure
 */
Config ConfigParser::parse(const std::string& path)
{
    tokens = Tokenizer::tokenize(path);
    pos = 0;

    Config cfg;
    parseServer(cfg);
    return (cfg);
}
