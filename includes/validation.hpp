/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   validation.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: root <root@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/11 21:35:18 by usuario           #+#    #+#             */
/*   Updated: 2026/03/03 21:20:00 by root             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef VALIDATION_HPP
#define VALIDATION_HPP

#include "webserv.hpp"
#include "httpRequest.hpp"

//Validates CONFIG on runtime
static bool isValidMethod(const std::string& m);
static bool isValidHttpErrorCode(int code);
static void validateLocation(const Location& loc, const Config& cfg);

void validateServer(const Config& cfg);
void validateAllServers(const std::vector<Config>& cfgs);

//Validates REQUEST on runtime - NO LONGER NEEDED. VERIFICATION MADE ON HTTPRESPONSE!!! 
/*
bool isMethodAllowed(const HttpRequest& req, const Location& loc);
bool isBodySizeValid(const HttpRequest& req, const Location& loc, const Config& cfg);
int  validateRequest(const HttpRequest& req, const Location& loc, const Config& cfg);
*/
#endif
