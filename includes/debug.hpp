/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   debug.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/11 20:37:23 by usuario           #+#    #+#             */
/*   Updated: 2026/03/09 00:51:00 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef DEBUG_HPP
#define DEBUG_HPP

#include "webserv.hpp"
#include "colors.hpp"

void printConfig(const Config &cfg);
void printAllConfigs(const std::vector<Config> &cfgs);
void debugTestLocationMatching(const std::vector<Config> &cfgs);
void debugTestPathResolution(const std::vector<Config> &cfgs);
void debugTestRoutingAndResolution(const std::vector<Config> &cfgs);
void debugTestCgiDetection(const std::vector<Config>& cfgs);
void debugTestCgiEnv(const Config& cfg);
void debugTestCgiExecution(const Config& cfg);

#endif