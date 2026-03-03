#include "HttpParser.hpp"

bool HttpParser::parseRequest(std::string &buffer, HttpRequest &req) 
{
    // Buscar el final de los headers
    size_t pos = buffer.find("\r\n\r\n");
    if (pos == std::string::npos)
        return false; // Todavía no están los headers completos, sigue leyendo

    // Si llegamos aquí, ya tenemos headers. El Rol 1 los separa:
    std::string rawHeaders = buffer.substr(0, pos);
    
	/*
    // El Rol 1 debe mirar si hay Content-Length para saber si falta el Body
    if (req.hasBody(rawHeaders)) {
        size_t expectedSize = req.getContentLength();
        std::string body = buffer.substr(pos + 4);
        if (body.size() < expectedSize)
            return false; // Falta cuerpo por leer
    }
    
    // Si es "Chunked", el Rol 1 debe procesar los trozos aquí
    if (req.isChunked(rawHeaders)) {
        return processChunks(buffer, req); // Devuelve true solo si llega el chunk 0
    }*/

	std::string line;
	std::istringstream iss(rawHeaders);
	std::getline(iss, line); // Obtiene la primera línea: "GET /index.html HTTP/1.1"

	std::istringstream lineStream(line);
	lineStream >> req.method >> req.path >> req.version;

    return true; // Petición completa
}