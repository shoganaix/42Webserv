/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_post.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kpineda- <kpineda-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/08 12:39:05 by kpineda-          #+#    #+#             */
/*   Updated: 2026/03/08 14:06:10 by kpineda-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpResponse.hpp"
#include <iostream>
#include <sys/stat.h>
#include <cstdlib> // Para system()

int main() {
    // 1. Creamos la instancia de tu clase
    HttpResponse res;

    // 2. CREAMOS LA VARIABLE 'loc' (Aquí es donde se define)
    Location loc; 
    loc.root = "./uploads_test";
    loc.upload_path = "./uploads_test";
    loc.allow_methods.push_back("POST");
    loc.allow_methods.push_back("DELETE");
	loc.allow_methods.push_back("GET");

    // --- PRUEBA DE POST ---
    std::cout << "🚀 Probando POST..." << std::endl;
    res.handlePost("Contenido de prueba", loc, 1000000);
    std::cout << res.toString() << std::endl;

    // --- PRUEBA DE DELETE ---
    std::cout << "\n🚀 Probando DELETE..." << std::endl;
    // IMPORTANTE: Cambia este nombre por uno que SI exista en tu carpeta uploads_test
    std::string archivoABorrar = "./uploads_test/file.txt"; 
    
    res.handleDelete(archivoABorrar, loc); // Ahora 'loc' ya está definido arriba
    std::cout << res.toString() << std::endl;

	// En el main de test_post.cpp
	std::cout << "\n🚀 Probando GET..." << std::endl;

	// 1. Asegúrate de que existe un archivo para leer
	system("echo 'Hola desde el servidor' > ./uploads_test/test.txt");

	loc.root = "./uploads_test"; // Cambiamos el root para la prueba
	res.handleGet("/test.txt", loc);

	std::cout << "--- RESPUESTA GET ---\n" << res.toString() << std::endl;

    return 0;
}
