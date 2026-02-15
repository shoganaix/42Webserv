/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msoriano <msoriano@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/21 00:44:52 by usuario           #+#    #+#             */
/*   Updated: 2026/02/15 11:16:56 by msoriano         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*-----------------------------------------------------------------------
 *                          🔧UTILITY FUNCTIONS🔧
 *
 * This file contains helper functions used to simplify common operations
 *
 * Index:
 *  - intToString(): safely converts an integer into a string
 *  - ...
 *
 * -----------------------------------------------------------------------
 */

#include "../includes/utils.hpp"

std::string intToString(int n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}