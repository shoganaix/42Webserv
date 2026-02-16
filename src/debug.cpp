/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   debug.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/11 20:53:33 by usuario           #+#    #+#             */
/*   Updated: 2026/02/16 14:01:10 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/debug.hpp"
#include "../includes/matchLocation.hpp"
#include "../includes/pathResolver.hpp"
#include "../includes/webserv.hpp"


/*-----------------------------------------------------------------------
 *                      🖨️DEBUG: CONFIG PARSER🖨️
 *
 * This block of functions print the parsed configuration. We check:
 * 
 *  - Server blocks
 *  - Location blocks
 *  - Error pages
 *  - Allowed HTTP methods
 *  - CGI handlers
 *
 * This verifies that parsing and normalization were performed
 * correctly before starting the server runtime
 * -----------------------------------------------------------------------
 */

// ---------------------------- MULTI-SERVER CONF PARSER ------------------------
static void printLocation(const Location &loc)
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
void printConfig(const Config &cfg)
{
    std::cout << BLUE << "\n==================== SERVER CONFIG ==================="<< RESET << "\n";
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

    std::cout << BLUE << "========================================================"<< RESET << "\n";
}

void printAllConfigs(const std::vector<Config> &cfgs)
{
    std::cout << GREEN << "Parsed servers: " << RESET << cfgs.size() << std::endl;
    for (size_t i = 0; i < cfgs.size(); ++i)
    {
        std::cout << GREEN << "--- Server #" << (i + 1) << RESET << std::endl;
        printConfig(cfgs[i]);
    }
}
//-----------------------------------------------------------------------------------------

/*-----------------------------------------------------------------------
 *                      🧪LOCATION MATCH TESTER🧪
 *
 * Simulates HTTP request URIs and verifies that the location matching & 
 * path resolving algorithm selects the correct final FILESYSTEM
 * -----------------------------------------------------------------------
 */
// --------------------------------- ROUTING RESOL. MINITEST ------------------------------

void debugTestRoutingAndResolution(const std::vector<Config> &cfgs)
{
    std::cout << BLUE << "\n======= MATCHLOCATION + PATH RESOLUTION MINITEST =======\n" << RESET;

    std::vector<std::string> testUris;
    testUris.push_back("/");
    testUris.push_back("/tours");
    testUris.push_back("/tours/");
    testUris.push_back("/tours/summer.html");
    testUris.push_back("/cgi-bin/time.py");
    testUris.push_back("/cgi-bin/");
    testUris.push_back("/unknown/path");
    testUris.push_back("/unknown/path/");

    for (size_t i = 0; i < cfgs.size(); ++i)
    {
        std::cout << YELLOW << "\nServer #" << (i + 1) << RESET << std::endl;

        for (size_t j = 0; j < testUris.size(); ++j)
        {
            //THESE TWO LINES CHANGE COLOR (IGNORE)
            std::vector<std::string> colors; colors.push_back(RED);colors.push_back(GREEN);colors.push_back(BLUE);
            const std::string& blockColor = colors[j % colors.size()];

            const std::string& uri = testUris[j];
            const Location* loc = matchLocation(cfgs[i], uri);

            std::cout << blockColor << "\nURI: " << uri << RESET << "\n";
            std::cout << "  Matched location: " << (loc ? loc->path : "NULL") << "\n";

            if (!loc)
                continue;

            ResolvedPath rp = resolvePath(*loc, uri);

            std::cout << "  Config file loc.root: "  << (loc->root.empty() ? "(empty)" : loc->root) << "\n";
            std::cout << "  Config file loc.index: " << (loc->index.empty() ? "(empty)" : loc->index) << "\n";
            std::cout << blockColor << "REST Path: " << (rp.resPath.empty() ? "(empty)" : rp.resPath) << "\n";
            std::cout << "FINAL FS Path: " << (rp.fsPath.empty() ? "(empty)" : rp.fsPath) << RESET  << "\n";
            std::cout << "[Appended index: " << (rp.appendIndex ? "yes" : "no") << "]\n";
        }
    }

    std::cout << BLUE << "==========================================================\n" << RESET;
}
//-----------------------------------------------------------------------------------------






















































/*-----------------------------------------------------------------------
 *                      🧪LOCATION MATCH TESTER🧪
 *
 * Simulates HTTP request URIs and verifies that the
 * location matching algorithm selects the correct location
 * -----------------------------------------------------------------------
 */
// ---------------------------- URI / LOC PATH-MATCHING MINITEST ------------------------
void debugTestLocationMatching(const std::vector<Config> &cfgs)
{
    std::cout << BLUE << "\n======= LOCATION MATCH TEST =======\n" << RESET;

    // Simulated URIs (replicating real HTTP requests)
    std::vector<std::string> testUris;
    testUris.push_back("/");
    testUris.push_back("/tours");
    testUris.push_back("/tours/summer.html");
    testUris.push_back("/red");
    testUris.push_back("/cgi-bin/time.py");
    testUris.push_back("/unknown/path");


    for (size_t i = 0; i < cfgs.size(); ++i)
    {
        std::cout << YELLOW << "\nServer #" << i + 1 << RESET << std::endl;

        for (size_t j = 0; j < testUris.size(); ++j)
        {
            const Location* loc = matchLocation(cfgs[i], testUris[j]);

            std::cout << "URI: " << testUris[j]
                      << " -> matched: "
                      << (loc ? loc->path : "NULL")
                      << std::endl;
        }
    }

    std::cout << BLUE << "===================================\n" << RESET;
}
//-----------------------------------------------------------------------------------------

/*-----------------------------------------------------------------------
 *                      🧪RESOLVER PATH TESTER🧪
 *
 * Simulates HTTP request URIs and verifies the final FILESYSTEM 
 * resolution process after matchLocation()
 * -----------------------------------------------------------------------
 */
// ---------------------------- FINAL FILESYSTEM MINITEST ----------------
void debugTestPathResolution(const std::vector<Config> &cfgs)
{
    std::cout << BLUE << "\n======= PATH RESOLUTION TEST =======\n" << RESET;

    std::vector<std::string> testUris;
    testUris.push_back("/");
    testUris.push_back("/tours");
    testUris.push_back("/tours/summer.html");

    for (size_t i = 0; i < cfgs.size(); ++i)
    {
        std::cout << YELLOW << "\nServer #" << (i + 1) << RESET << std::endl;

        for (size_t j = 0; j < testUris.size(); ++j)
        {
            const std::string& uri = testUris[j];
            const Location* loc = matchLocation(cfgs[i], uri);

            std::cout << "\nURI: " << uri << "\n";

            if (!loc)
            {
                std::cout << "  matched: NULL\n";
                continue;
            }

            ResolvedPath rp = resolvePath(*loc, uri);

            std::cout << "  matched: " << loc->path << "\n";
            std::cout << "  loc.root: " << (loc->root.empty() ? "(empty)" : loc->root) << "\n";
            std::cout << "  loc.index: " << (loc->index.empty() ? "(empty)" : loc->index) << "\n";
            std::cout << "  resPath: " << (rp.resPath.empty() ? "(empty)" : rp.resPath) << "\n";
            std::cout << "  fsPath: " << (rp.fsPath.empty() ? "(empty)" : rp.fsPath) << "\n";
            std::cout << "  appended index: " << (rp.appendIndex ? "yes" : "no") << "\n";
        }
    }

    std::cout << BLUE << "====================================\n" << RESET;
}
//-----------------------------------------------------------------------------------------
