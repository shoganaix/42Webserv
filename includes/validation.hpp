/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   validation.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/11 21:35:18 by usuario           #+#    #+#             */
/*   Updated: 2026/02/11 21:51:48 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef VALIDATION_HPP
#define VALIDATION_HPP

#include "webserv.hpp"

void validateServer(const Config& cfg);
void validateAllServers(const std::vector<Config>& cfgs);

#endif
