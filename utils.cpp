/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kpineda- <kpineda-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 19:50:35 by kpineda-          #+#    #+#             */
/*   Updated: 2026/03/02 15:58:42 by kpineda-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <fstream>
#include <sstream>

std::string readFile(const std::string& path) {
    // Abrimos el archivo en modo lectura
    std::ifstream file(path.c_str());
    
    // Si no se puede abrir (no existe o no hay permisos)
    if (!file.is_open()) {
        return ""; 
    }

    std::stringstream buffer;
    buffer << file.rdbuf(); // Metemos todo el archivo al buffer
    return buffer.str();
}