/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   validation.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msoriano <msoriano@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/11 21:35:18 by usuario           #+#    #+#             */
/*   Updated: 2026/02/14 13:45:07 by msoriano         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef VALIDATION_HPP
#define VALIDATION_HPP

#include "webserv.hpp"

void validateServer(const Config& cfg);
void validateAllServers(const std::vector<Config>& cfgs);

#endif
