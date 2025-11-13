/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: luis <luis@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/09 01:35:13 by joserra           #+#    #+#             */
/*   Updated: 2025/10/30 13:21:34 by luis             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"         // Incluye el header de la clase ConfigParser
#include <fstream>                  // Para trabajar con archivos (ifstream)
#include <sstream>                  // Para manejar buffers de texto
#include <iostream>                 // Para imprimir errores o debug
#include <stdexcept>   
#include "ServerConfig.hpp"            // Para lanzar excepciones como runtime_error

// Constructor que recibe la ruta del archivo .conf
ConfigParser::ConfigParser(const std::string& filepath)
	: _filepath(filepath) {}       // Guarda la ruta del archivo como atributo

// Función principal para parsear la configuración
void ConfigParser::parse() {
	readFile();                    // Leer el archivo a string (_fileContent)
	removeComments();              // Eliminar los comentarios (líneas con #)
	splitServerBlocks();           // Separar bloques server {...}
}

// Lee el contenido completo del archivo de configuración
void ConfigParser::readFile() {
	std::ifstream file(_filepath.c_str());         // Abrir archivo
	if (!file.is_open())                           // Verificar si se abrió correctamente
		throw std::runtime_error("Error: Cannot open config file."); // Lanzar error si falla

	std::stringstream buffer;
	buffer << file.rdbuf();                         // Leer todo el contenido en buffer
	_fileContent = buffer.str();                    // Guardar como string en _fileContent
	file.close();                                    // Cerrar el archivo
}

// Elimina comentarios de tipo '#' en cada línea
void ConfigParser::removeComments() {
	std::string cleaned;
	std::istringstream stream(_fileContent);       // Convertir string a stream línea por línea
	std::string line;

	while (std::getline(stream, line)) {           // Leer cada línea
		size_t commentPos = line.find('#');        // Buscar la posición del '#'
		if (commentPos != std::string::npos)
			line = line.substr(0, commentPos);     // Cortar el comentario
		cleaned += line + '\n';                    // Agregar la línea limpia al string final
	}

	_fileContent = cleaned;                        // Reemplazar el contenido original sin comentarios
}

// Divide el contenido en bloques de configuración de servidor
void ConfigParser::splitServerBlocks() {
    size_t pos = 0;                                // Posición para recorrer el string

    while ((pos = _fileContent.find("server", pos)) != std::string::npos) {
        size_t braceStart = _fileContent.find("{", pos);          // Buscar la llave de apertura
        if (braceStart == std::string::npos)
            throw std::runtime_error("Error: Invalid server block.");

        // Encontrar la llave de cierre emparejada con profundidad
        size_t i = braceStart + 1;
        int depth = 1;
        for (; i < _fileContent.size(); ++i) {
            char c = _fileContent[i];
            if (c == '{') depth++;
            else if (c == '}') {
                depth--;
                if (depth == 0) break;
            }
        }
        if (i >= _fileContent.size())
            throw std::runtime_error("Error: Unterminated server block.");

        size_t braceEnd = i;

        // Extraer contenido entre las llaves
        std::string blockContent = _fileContent.substr(braceStart + 1, braceEnd - braceStart - 1);
        std::istringstream blockStream(blockContent);     // Stream para leer línea por línea
        std::string line;

		ServerConfig server;      // Crear instancia de ServerConfig (vacía)

		while (std::getline(blockStream, line)) {
			std::istringstream lineStream(line);           // Stream para separar palabras
			std::string directive;
			lineStream >> directive;                       // Leer la primera palabra (la directiva)

			if (directive == "listen") {                   // Si la directiva es listen
				std::string value;
				while (lineStream >> value) {              // Leer todos los valores siguientes
					if (!value.empty() && value[value.size() - 1] == ';')
						value.erase(value.size() - 1);

					server.addListen(value);               // Guardar valor en el objeto server
				}
			}
			else if (directive == "server_name") {
				std::string value;
				while (lineStream >> value) {
					if (!value.empty() && value[value.size() - 1] == ';')
						value.erase(value.size() - 1);

					server.addServerName(value);
				}
			}
            else if (directive == "root") {
                std::string value;
                lineStream >> value;
                if (!value.empty() && value[value.size() - 1] == ';')
                    value.erase(value.size() - 1);

                server.root = value;
            }
            else if (directive == "index") {
                std::string value;
                lineStream >> value;
                if (!value.empty() && value[value.size() - 1] == ';')
                    value.erase(value.size() - 1);

                server.index = value;
            }
            else if (directive == "error_page") {
                std::vector<int> codes;
                std::string token;
                std::string path;
                
                // Leer todos los códigos hasta encontrar el path
                while (lineStream >> token) {
                    if (token[token.size() - 1] == ';') {
                        token.erase(token.size() - 1);
                        path = token;
                        break;
                    }
                    // Intentar parsear como número
                    int code = 0;
                    std::istringstream codeStream(token);
                    codeStream >> code;
                    if (code > 0 && codeStream.eof()) {
                        // Es un código válido
                        codes.push_back(code);
                    } else {
                        // No es un número, debe ser el path
                        path = token;
                        if (!path.empty() && path[path.size() - 1] == ';')
                            path.erase(path.size() - 1);
                        break;
                    }
                }
                
                // Registrar todos los códigos con el mismo path
                if (!path.empty()) {
                    for (size_t i = 0; i < codes.size(); ++i) {
                        server.addErrorPage(codes[i], path);
                    }
                }
            }
            else if (directive == "client_max_body_size") {
                std::string value;
                lineStream >> value;
                if (!value.empty() && value[value.size() - 1] == ';')
                    value.erase(value.size() - 1);
                // admite sufijos como k, m (opcional)
                size_t multiplier = 1;
                if (!value.empty()) {
                    char last = value[value.size() - 1];
                    if (last == 'k' || last == 'K') { multiplier = 1024; value.erase(value.size() - 1); }
                    else if (last == 'm' || last == 'M') { multiplier = 1024 * 1024; value.erase(value.size() - 1); }
                }
                size_t base = 0;
                std::istringstream iss(value);
                iss >> base;
                server.clientMaxBodySize = base * multiplier;
            }
            else if (directive == "location") {
                // Espera formato: location <path> { ... }
                std::string locPath;
                lineStream >> locPath; // puede ser "/" o "/uploads/" etc.

                // Leer el bloque con emparejamiento de llaves correcto
                std::string locBlock;
                int depth = 0;
                // Si en esta misma línea hay '{', cuenta una apertura
                if (line.find('{') != std::string::npos) depth = 1; 
                std::string tmpLine;
                // Si no estaba la '{' en la misma línea, avanza hasta encontrarla y cuenta
                if (depth == 0) {
                    while (std::getline(blockStream, tmpLine)) {
                        if (tmpLine.find('{') != std::string::npos) { depth = 1; break; }
                    }
                }
                // Acumular hasta cerrar el bloque (depth vuelve a 0)
                while (std::getline(blockStream, tmpLine)) {
                    if (tmpLine.find('{') != std::string::npos) depth++;
                    if (tmpLine.find('}') != std::string::npos) {
                        depth--;
                        if (depth == 0) break; // no añadimos la línea de cierre
                        // eliminar solo la '}' y continuar añadiendo el resto
                        // pero por simplicidad, si contiene '}', no la añadimos
                        continue;
                    }
                    locBlock += tmpLine + "\n";
                }

                // Parsear locBlock
                LocationConfig loc;
                loc.path = locPath;
                std::istringstream locStream(locBlock);
                std::string locLine;
                while (std::getline(locStream, locLine)) {
                    std::istringstream ls(locLine);
                    std::string d;
                    ls >> d;
                    if (d == "root") {
                        std::string v; ls >> v; if (!v.empty() && v[v.size()-1]==';') v.erase(v.size()-1); loc.root = v;
                    } else if (d == "autoindex") {
                        std::string v; ls >> v; if (!v.empty() && v[v.size()-1]==';') v.erase(v.size()-1); loc.setAutoindex(v);
                    } else if (d == "index") {
                        std::string v; ls >> v; if (!v.empty() && v[v.size()-1]==';') v.erase(v.size()-1); /* index por location opcional: podrías guardarlo en loc.root + v si lo usas */
                    } else if (d == "allow_methods") {
                        std::string m;
                        while (ls >> m) { if (!m.empty() && m[m.size()-1]==';') m.erase(m.size()-1); if (!m.empty()) loc.addAllowedMethod(m); }
                    } else if (d == "cgi_pass") {
                        std::string ext, exec; ls >> ext >> exec; if (!exec.empty() && exec[exec.size()-1]==';') exec.erase(exec.size()-1); if (!ext.empty() && !exec.empty()) loc.addCgiPass(ext, exec);
                    } else if (d == "return") {
                        std::string url; ls >> url; if (!url.empty() && url[url.size()-1]==';') url.erase(url.size()-1); loc.redirect = url;
                    } else if (d == "client_max_body_size") {
                        std::string v; ls >> v; if (!v.empty() && v[v.size()-1]==';') v.erase(v.size()-1);
                        size_t mult = 1; if (!v.empty()) { char last = v[v.size()-1]; if (last=='k'||last=='K'){ mult=1024; v.erase(v.size()-1);} else if (last=='m'||last=='M'){ mult=1024*1024; v.erase(v.size()-1);} }
                        size_t base=0; std::istringstream iss2(v); iss2>>base; loc.clientMaxBodySize = base*mult;
                    }
                }
                server.addLocation(loc);
            }

            // otras directivas se ignorarán por ahora
		}

        _servers.push_back(server);                        // Guardar el servidor en la lista
        pos = braceEnd + 1;                                // Mover posición para buscar el siguiente bloque
	}
}

// Función para acceder a la lista de servidores parseados
const std::vector<ServerConfig>& ConfigParser::getServers() const {
	return _servers;
}
