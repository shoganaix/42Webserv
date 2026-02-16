/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: angnavar <angnavar@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/08 18:51:13 by angnavar          #+#    #+#             */
/*   Updated: 2026/02/16 22:59:40 by angnavar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*-----------------------------------------------------------------------
 *                          🧠WEBSERV BRAIN🧠
 *
 * This class represents the main Webserver engine
 *
 * Responsibilities:
 *  - Load and store parsed configuration
 *  - Initialize listening sockets
 *  - Manage the main event loop
 *  - Handle client connections
 * 	- ...
 *
 * The run() method starts the infinite event loop
 * where the server waits for incoming connections
 * and processes client events.
 * -----------------------------------------------------------------------
 */

	#include "webserv.hpp"
	#include "configParser.hpp"
	#include "validation.hpp"

	/*
	* - Receives path to conf at startup
	* - Loads and parses using ConfigParser
	* - Normalizes + validates
	* - Stores the resulting configurations internally for later use 
	* - ... 
	*/
	Webserv::Webserv(const std::string &configFile)
	{
		std::cout << BLUE << "Webserv initialized with config: " << RESET << configFile << std::endl;
		
	    // 1) Parse configuration
		ConfigParser parser;
		this->config = parser.parse(configFile);

		// 2) Normalize + validate configuration
		validateAllServers(this->config);
	}

	/*
	* - 📌TO DO:📌 [REVISAR]
	* - Create listening sockets based on server blocks
	* - Add listening sockets to poll()
	* - Store mapping
	* - ....
	*/
	void Webserv::setSockets()
	{
    	// for each Config in this->config:
    	//     1. Create socket, set O_NONBLOCK, bind host:port, listen
    	//     2. Add to poll fd vector
    	//     3. Keep a map listeningFd -> serverIndex
		for (size_t i = 0; i < this->config.size(); ++i)
		{
			int fd = socket(AF_INET, SOCK_STREAM, 0);
			if (fd < 0) continue;

			int opt = 1;
			setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
			fcntl(fd, F_SETFL, O_NONBLOCK);
			sockaddr_in addr;
			std::memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_port = htons(config[i].port);
			addr.sin_addr.s_addr = inet_addr(config[i].host.c_str());

			if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
			{
				std::cerr << "Error binding port " << config[i].port << std::endl;
				close(fd);
				continue;
			}

			if (listen(fd, 128) < 0)
			{
				close(fd);
				continue;
			}

			fds.push_back(fd);
		}
	}

	/*
	* Main event loop using poll():
	* - Accept clients from listening sockets
	* - Read from client sockets, accumulate data until a full HTTP request is available
	* - Once request is parsed, build response via routing pipeline (below)
	* - ...
	*
	* - 📌TO DO:📌 [REVISAR] 
	* 	- poll(fds, ...)
	*	- handle events:
	*		   1) listening fd -> accept() new client -> add client fd to poll
	*		   2) client fd POLLIN -> read request data -> parse HttpRequest
	*		      -> when request complete: run routing pipeline (calls matchLocation + resolvePath)
	*		   3) client fd POLLOUT -> send pending response bytes
	*		   4) CGI pipe fds -> progress CGI sessions (read/write) + finalize response
	*/
	void Webserv::run()
	{
		setSockets();
		
		std::cout << GREEN << "Webserv running..." << RESET <<std::endl;
		while(true)
		{
			/*int ret = poll(fds.data(), fds.size(), -1); 
			if (ret < 0) throw std::runtime_error("poll error");

			for (size_t i = 0; i < fds.size(); i++) {
				if (fds[i].revents == 0) continue;
			}*/

			// 🤖 AI BASE STRUCTURE FOR RUN FUNCTION 🤖 [REVISAR] ❗❗
			// - 1. Select server config:
			// 		* by listening socket (port) and optionally Host header
			// - 2. Location match:
			//      * const Location* loc = matchLocation(serverCfg, req.path);
			// - 3. Apply location directives before filesystem:
	     	//	    * if loc->redir -> return 3xx response
	    	//	    * if method not allowed -> 405
			// - 4. File resolution:
	   		//      * ResolvedPath rp = resolvePath(*loc, req.path);
	  		//        // rp.fsPath is the target path on disk
			// - 5. Decide static vs autoindex vs CGI:
	  		//      * stat(rp.fsPath) to detect file/dir
	   		//      * if directory:
	    	//	      - if autoindex on -> generate listing
	   		//        - else -> append index and retry, or 403/404 depending on policy
	    	//	    * CGI detection:
	      	//	      CgiTarget cgi = detectCgi(*loc, rp.fsPath);
	       	//	      if cgi.isCgi -> run CGI subsystem
	 		// - 6. CGI execution (poll-integrated):
	  		//     * Create CgiSession (fork/exec + pipes)
	   		//     * Add CGI pipe fds to poll set
	   		//     * POLLOUT: write request body to CGI stdin, then close (EOF)
	   		//     * POLLIN: read CGI stdout until EOF / Content-Length satisfied
	   		//     * Parse CGI headers/body and build final HttpResponse
			// - 7. Send response:
	  		//     * write() response to client socket (non-blocking)
	   		//     * close or keep-alive depending on headers

		}
	}
