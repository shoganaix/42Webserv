/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   debug.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/11 20:53:33 by usuario           #+#    #+#             */
/*   Updated: 2026/02/11 21:20:51 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/debug.hpp"

// TESTING CONF PARSER (DELETE LATER)
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
static void printLocation(const Location& loc)
{
    std::cout << "    " << YELLOW << "\n--- Location " << RESET << loc.path << std::endl;
    std::cout << "      Path: " << loc.path << std::endl;
    std::cout << "      Root: " << (loc.root.empty() ? "(empty)" : loc.root) << std::endl;
    std::cout << "      Index: " << (loc.index.empty() ? "(empty)" : loc.index) << std::endl;
    std::cout << "      Autoindex: " << (loc.autoindex ? "on" : "off") << std::endl;
    std::cout << "      Redirection: " << (loc.redir.empty() ? "(none)" : loc.redir) << std::endl;
    std::cout << "      Upload path: " << (loc.upload_path.empty() ? "(none)" : loc.upload_path) << std::endl;

    std::cout << "Allowed methods: ";
    if (loc.allow_methods.empty())
        std::cout << " (none)";
    else
    {
        for (size_t i = 0; i < loc.allow_methods.size(); i++)
            std::cout << loc.allow_methods[i] << " ";
    }
    std::cout << std::endl;

    std::cout << "CGI handlers:\n";
    if (loc.cgi_needs.empty())
        std::cout << " (none)\n";
    else
    {
        for (std::map<std::string, std::string>::const_iterator it = loc.cgi_needs.begin();
            it != loc.cgi_needs.end(); ++it)
        {
            std::cout << "  " << it->first << " -> " << it->second << std::endl;
        }
    }
}
void printConfig(const Config& cfg)
{
    std::cout << BLUE << "\n========== SERVER CONFIG =========="<< RESET << "\n";
    std::cout << "  Server name: " << cfg.server_name << std::endl;
    std::cout << "  Host: " << cfg.host << std::endl;
    std::cout << "  Port: " << cfg.port << std::endl;
    std::cout << "  Root: " << cfg.root << std::endl;
    std::cout << "  Index: " << cfg.index << std::endl;
    std::cout << "  Client max body size: " << cfg.client_max_body_size << std::endl;

    std::cout << "\nError pages:\n";
    if (cfg.error_pages.empty())
        std::cout << " (none)\n";
    else
    {
        for (std::map<int, std::string>::const_iterator it = cfg.error_pages.begin();
            it != cfg.error_pages.end(); ++it)
        {
            std::cout << "  " << it->first << " -> " << it->second << std::endl;
        }
    }

    std::cout << "\nLocations (" << cfg.locations.size() << "):\n";
    for (size_t i = 0; i < cfg.locations.size(); i++)
        printLocation(cfg.locations[i]);

    std::cout << BLUE << "==================================="<< RESET << "\n";
}

void printAllConfigs(const std::vector<Config>& cfgs)
{
    std::cout << GREEN << "Parsed servers: " << RESET << cfgs.size() << std::endl;
    for (size_t i = 0; i < cfgs.size(); ++i)
    {
        std::cout << GREEN << "--- Server #" << (i + 1) << RESET << std::endl;
        printConfig(cfgs[i]);
    }
}