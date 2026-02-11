/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   debug.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/11 20:37:23 by usuario           #+#    #+#             */
/*   Updated: 2026/02/11 21:09:52 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef DEBUG_HPP
#define DEBUG_HPP

#include "webserv.hpp"
#include "colors.hpp"

void printConfig(const Config& cfg);
void printAllConfigs(const std::vector<Config>& cfgs);

#endif