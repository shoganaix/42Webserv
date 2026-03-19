/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   debug.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/11 20:53:33 by usuario           #+#    #+#             */
/*   Updated: 2026/03/19 19:30:43 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/debug.hpp"
#include "../includes/matchLocation.hpp"
#include "../includes/pathResolver.hpp"
#include "../includes/webserv.hpp"
#include "../includes/cgiHandler.hpp"
#include "../includes/httpRequest.hpp"


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
 *                      🧪FULL PATH RESOLVER TESTER🧪
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
 *                      🖨️DEBUG: CGI PIPELINE🖨️
 *
 * This block of functions validates the CGI execution pipeline
 *
 * The debug tests verify:
 *
 *  - CGI detection based on file extension and location configuration
 *  - Correct resolution of CGI handler, script path and working directory
 *  - Environment variable generation passed to the CGI process
 *  - Execution of the CGI script using fork() + execve()
 *  - Reading the CGI output from stdout
 *  - Parsing the CGI response into a valid HttpResponse
 * -----------------------------------------------------------------------
 */

/*-----------------------------------------------------------------------
 *                      🧪CGI DETECTION TESTER🧪
 * Verifies whether a requested resource should be executed as CGI
 * -----------------------------------------------------------------------
 */
void debugTestCgiDetection(const std::vector<Config>& cfgs)
{
    std::cout << BLUE << "\n======= CGI DETECTION TEST =======\n" << RESET;

    std::vector<std::string> testUris;
    testUris.push_back("/cgi-bin/time.py");
    testUris.push_back("/cgi-bin/test.sh");
    testUris.push_back("/tours/index.html");
    testUris.push_back("/unknown/file.py");

    for (size_t i = 0; i < cfgs.size(); ++i)
    {
        std::cout << YELLOW << "\nServer #" << (i + 1) << RESET << std::endl;

        for (size_t j = 0; j < testUris.size(); ++j)
        {
            const std::string& uri = testUris[j];
            const Location* loc = matchLocation(cfgs[i], uri);

            std::cout << "\nURI: " << uri << std::endl;

            if (!loc)
            {
                std::cout << "  matched location: NULL\n";
                continue;
            }

            ResolvedPath rp = resolvePath(*loc, uri);
            CgiTarget target = CgiHandler::detectCgi(*loc, rp.fsPath);

            std::cout << "  matched location: " << loc->path << std::endl;
            std::cout << "  fsPath: " << rp.fsPath << std::endl;
            std::cout << "  isCgi: " << (target.isCgi ? "yes" : "no") << std::endl;
            std::cout << "  extension: " << (target.extension.empty() ? "(empty)" : target.extension) << std::endl;
            std::cout << "  handlerPath: " << (target.handlerPath.empty() ? "(empty)" : target.handlerPath) << std::endl;
            std::cout << "  scriptPath: " << (target.scriptPath.empty() ? "(empty)" : target.scriptPath) << std::endl;
            std::cout << "  workingDir: " << (target.workingDir.empty() ? "(empty)" : target.workingDir) << std::endl;
        }
    }

    std::cout << BLUE << "==================================\n" << RESET;
}

/*-----------------------------------------------------------------------
 *                      🧪CGI ENV TESTER🧪
 * Builds the env variables that will be passed to the CGI
 * process and prints them to verify correctness
 * -----------------------------------------------------------------------
 */
void debugTestCgiEnv(const Config& cfg)
{
    const Location* loc = matchLocation(cfg, "/cgi-bin/time.py");
    if (!loc)
        return;

    ResolvedPath rp = resolvePath(*loc, "/cgi-bin/time.py");
    CgiTarget target = CgiHandler::detectCgi(*loc, rp.fsPath);
    if (!target.isCgi)
        return;

    HttpRequest req;
    std::string raw =
        "POST /cgi-bin/time.py?name=maria&debug=1 HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "hello=world";

    if (!req.parse(raw))
    {
        std::cerr << "Failed to parse debug CGI request" << std::endl;
        return ;
    }
    std::map<std::string, std::string> env = CgiHandler::buildEnv(req, target, cfg.server_name, cfg.port, "127.0.0.1");

    std::cout << BLUE << "\n======= CGI ENV TEST =======\n" << RESET;
    for (std::map<std::string, std::string>::const_iterator it = env.begin(); it != env.end(); ++it)
        std::cout << it->first << "=" << it->second << std::endl;
}

/*-----------------------------------------------------------------------
 *                      🧪CGI EXEC TESTER🧪
 * Executes a CGI script using the configured handler and captures
 * its raw output
 * -----------------------------------------------------------------------
 */
void debugTestCgiExecution(const Config& cfg)
{
    const Location* loc = matchLocation(cfg, "/cgi-bin/time.py");
    if (!loc)
        return;

    ResolvedPath rp = resolvePath(*loc, "/cgi-bin/time.py");
    CgiTarget target = CgiHandler::detectCgi(*loc, rp.fsPath);
    if (!target.isCgi)
        return;

    HttpRequest req;
    std::string raw =
        "GET /cgi-bin/time.py HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "\r\n";

    if (!req.parse(raw))
    {
        std::cerr << "Failed to parse debug CGI request" << std::endl;
        return ;
    }

    CgiResult result = CgiHandler::execute(req, target, cfg.server_name, cfg.port,"127.0.0.1");

    std::cout << BLUE << "\n======= CGI EXECUTION TEST =======\n" << RESET;
    std::cout << "Exit code: " << result.exitCode << std::endl;
    std::cout << "Raw output:\n" << result.rawOutput << std::endl;

    HttpResponse res = CgiHandler::parseCgiOutput(result.rawOutput);
    std::cout << "\nParsed CGI response:\n";
    std::cout << res.toString() << std::endl;
}

















































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
