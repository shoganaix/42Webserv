/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   matchLocation.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: usuario <usuario@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/14 14:00:09 by msoriano          #+#    #+#             */
/*   Updated: 2026/02/16 13:27:45 by usuario          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MATCHLOCATION_HPP
#define MATCHLOCATION_HPP

#include "webserv.hpp"

const Location* matchLocation(const Config &cfg, const std::string &uri);

#endif