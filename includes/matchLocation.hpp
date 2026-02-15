/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   matchLocation.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msoriano <msoriano@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/14 14:00:09 by msoriano          #+#    #+#             */
/*   Updated: 2026/02/15 11:43:43 by msoriano         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MATCHLOCATION_HPP
#define MATCHLOCATION_HPP

#include <string>
#include "webserv.hpp"

const Location* matchLocation(const Config &cfg, const std::string &uri);

#endif