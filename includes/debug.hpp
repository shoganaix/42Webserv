/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   debug.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: macastro <macastro@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/11 20:37:23 by usuario           #+#    #+#             */
/*   Updated: 2026/04/09 20:53:30 by macastro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef DEBUG_HPP
#define DEBUG_HPP

#include "webserv.hpp"
#include "colors.hpp"

void printConfig(const Config& cfg);
void printAllConfigs(const std::vector<Config>& cfgs);
void debugTestLocationMatching(const std::vector<Config>& cfgs);
void debugTestPathResolution(const std::vector<Config>& cfgs);
void debugTestRoutingAndResolution(const std::vector<Config>& cfgs);
void debugTestCgiDetection(const std::vector<Config>& cfgs);
void debugTestCgiEnv(const Config& cfg);
void debugTestCgiExecution(const Config& cfg);
void testlocationMatches();
void testmatchLocation();

#endif
