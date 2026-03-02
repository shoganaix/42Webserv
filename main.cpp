/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kpineda- <kpineda-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 19:24:44 by kpineda-          #+#    #+#             */
/*   Updated: 2026/03/02 22:26:59 by kpineda-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpResponse.hpp"
#include <iostream>

int main() {

    std::cout << "--- PRUEBA 1: Respuesta 200 OK ---" << std::endl;
    HttpResponse res200;
    res200.setStatusCode(200);
    res200.setBody("<html><body><h1>Hola desde mi servidor!</h1></body></html>");
    res200.addHeader("Content-Type", "text/html");
    res200.addHeader("Server", "MiWebserver/1.0");
    
    std::cout << res200.toString() << std::endl;

    std::cout << "\n--- PRUEBA 2: Respuesta 404 Not Found ---" << std::endl;
    HttpResponse res404;
    res404.setStatusCode(404);
    res404.setBody("<html><body><h1>404 Pagina no encontrada</h1></body></html>");
    res404.addHeader("Content-Type", "text/html");
    
    std::cout << res404.toString() << std::endl;

	std::cout << "--- PRUEBA 3: Verificando Content-Length automático ---" << std::endl;
	HttpResponse testLen;
	testLen.setStatusCode(200);
	testLen.setBody("12345"); // Esto mide 5 bytes
	// No llamamos a addHeader("Content-Length", ...)

	std::cout << testLen.toString() << std::endl;

    std::cout << "--- PRUEBA 4: Cargando archivo HTML real ---" << std::endl;
    HttpResponse res1;
    res1.loadFile("hola.html"); 
    std::cout << res1.toString() << std::endl;

    std::cout << "--- PRUEBA 5: Cargando archivo de texto ---" << std::endl;
    HttpResponse res2;
    res2.loadFile("info.txt");
    std::cout << res2.toString() << std::endl;

    std::cout << "--- PRUEBA 6: Intentando cargar archivo inexistente ---" << std::endl;
    HttpResponse res3;
    res3.loadFile("archivo_fantasma.html");
    std::cout << res3.toString() << std::endl;

    std::cout << "--- PRUEBA 7: Reutilización de objeto ---" << std::endl;
    HttpResponse res; // Creamos UN solo objeto

    // Cliente 1
    res.loadFile("hola.html");
    std::cout << "Respuesta 1 enviada." << std::endl;
    res.clear(); // <--- ¡Limpiamos!

    // Cliente 2
    res.setStatusCode(403);
    res.setBody("Acceso denegado");
    std::cout << res.toString() << std::endl;

    std::cout << "--- PRUEBA 8: Probando Directorio ---" << std::endl;
    HttpResponse resDir;
    resDir.loadFile("test_folder"); 
    std::cout << resDir.toString() << std::endl; 

	 std::cout << "--- PRUEBA 9: Probando AutoIndex ---" << std::endl;
    HttpResponse resDir1;
    resDir1.loadFile("fotos"); 
    std::cout << resDir1.toString() << std::endl; 
    return 0;
}