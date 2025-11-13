/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: joserra <joserra@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/09 01:36:13 by joserra           #+#    #+#             */
/*   Updated: 2025/08/09 01:36:15 by joserra          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP
#include "LocationConfig.hpp"
#include <string>
#include <vector>
#include <map>
/* 📦 Inclusión de dependencias necesarias:

LocationConfig.hpp: definición de la clase LocationConfig, usada para representar cada bloque 
location {}.
string, vector, map: contenedores estándar para manejar listas y mapas de configuraciones. */


class ServerConfig {
// 📘 Definición de la clase ServerConfig, que encapsula todos los datos de configuración de un bloque server.
	public:
// 🔒 Variables públicas:

    std::vector<std::string> listen;
	//📡 Lista de valores de la directiva listen, por ejemplo ["127.0.0.1:8080", "localhost:3000"].

    std::vector<std::string> serverNames;
	// 🌐 Lista de nombres de dominio (server_name) que este servidor debe atender.


    std::string root;
	// 📁 Ruta del directorio base de documentos (root), como /www.

    std::string index;
	// 📄 Nombre del archivo por defecto que se servirá cuando se acceda a una ruta (index, ej: "index.html").

    std::map<int, std::string> errorPages;
	/* ⚠️ Mapa de códigos de error HTTP a rutas de archivos personalizados de error.
	Ejemplo: {404: "/404.html", 500: "/50x.html"} */

    size_t clientMaxBodySize;
	/* 📏 Tamaño máximo permitido del cuerpo de una petición HTTP en bytes.
	Usado para limitar uploads o peticiones muy grandes. */
    std::vector<LocationConfig> locations;
	// 📍 Vector que contiene todos los bloques location {} definidos dentro del bloque server.

	// 🔧 Métodos públicos:

    ServerConfig();
	// 🔧 Constructor por defecto. Inicializa los miembros con valores seguros (lo vimos en el .cpp).

    void addListen(const std::string& ipPort);
	// 📥 Añade un valor a listen.

    void addServerName(const std::string& name);
	// 📥 Añade un nombre de dominio a serverNames.

    void addErrorPage(int code, const std::string& path);
	// 📥 Añade una página personalizada de error al mapa errorPages.


    void addLocation(const LocationConfig& loc);
	// 📥 Añade una estructura LocationConfig al vector de locations.

};
#endif

/*  En resumen:
Esta clase se encarga de modelar la configuración de un único servidor según el archivo .conf, agrupando directivas como listen, server_name, root, index, error_page, client_max_body_size, y los bloques location.

¿Quieres seguir con LocationConfig.hpp y .cpp, o pasamos a implementar una nueva directiva en el parser (como server_name, root, etc.)? */
