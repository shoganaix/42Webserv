/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   matchLocation.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: macastro <macastro@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/14 14:00:09 by msoriano          #+#    #+#             */
/*   Updated: 2026/04/09 21:16:49 by macastro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MATCHLOCATION_HPP
#define MATCHLOCATION_HPP

#include "webserv.hpp"

const Location* matchLocation(const Config& cfg, const std::string& uri);
bool locationMatches(const std::string& locPath, const std::string& requestPath);

#endif
