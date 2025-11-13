/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: joscastr <joscastr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/01 22:37:34 by joscastr          #+#    #+#             */
/*   Updated: 2025/08/01 23:25:55 by joscastr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
    : root(""), index(""), clientMaxBodySize(0) {}
	
	/* 🔧 Constructor por defecto:

Inicializa root e index como cadenas vacías ("")

Inicializa clientMaxBodySize en 0
Estos valores serán actualizados posteriormente con los datos del archivo de configuración. */

void ServerConfig::addListen(const std::string& ipPort) {
    listen.push_back(ipPort);
}
/* 📡 Método para añadir una directiva listen (ej: "127.0.0.1:8080") al vector listen.
Esto permite múltiples listen en un mismo bloque server. */

void ServerConfig::addServerName(const std::string& name) {
    serverNames.push_back(name);
}
/* 🌐 Añade un nombre de servidor (server_name) al vector serverNames.
Ej: "localhost", "test.local", etc. */

void ServerConfig::addErrorPage(int code, const std::string& path) {
    errorPages[code] = path;
}
/* ⚠️ Asocia un código de error HTTP (404, 500, etc.) a una ruta de archivo que se debe servir como página de error.
Se guarda en un std::map<int, std::string> llamado errorPages. */

void ServerConfig::addLocation(const LocationConfig& loc) {
    locations.push_back(loc);
}
/* 📁 Añade un bloque location (que contiene su propia configuración) al vector locations.
Cada LocationConfig representa un sub-bloque location { ... } dentro del bloque server. */
