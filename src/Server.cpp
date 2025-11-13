/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: luis <luis@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/09 01:35:42 by joserra           #+#    #+#             */
/*   Updated: 2025/11/07 14:46:41 by luis             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <fcntl.h>   // ✅ fcntl(), F_GETFL, F_SETFL, O_NONBLOCK
#include <sstream>
#include <stdexcept>



std::map<int, int> Server::_globalSocketMap;  // Definición: puerto -> socket
std::map<int, std::string> Server::_portToIpPort;  // Definición: puerto -> ipPort original

int Server::normalizePort(const std::string& ipPort) {
	std::string ip = "0.0.0.0";
	int port = 0;
	
	size_t colon = ipPort.find(':');
	if (colon != std::string::npos) {
		ip = ipPort.substr(0, colon);
		port = std::atoi(ipPort.substr(colon + 1).c_str());
	} else {
		port = std::atoi(ipPort.c_str());
	}
	
	// Validar puerto
	if (port <= 0 || port > 65535) {
		return -1;
	}
	
	return port;
}

Server::Server(const ServerConfig& config) : _config(config) {
	for (size_t i = 0; i < _config.listen.size(); i++) {
		const std::string& ipPort = _config.listen[i];
		int port = normalizePort(ipPort);
		
		if (port == -1) {
			std::ostringstream oss;
			oss << "Error: Puerto inválido en " << ipPort;
			throw std::runtime_error(oss.str());
		}
		
		// Verificar si el puerto ya está en uso
		if (_globalSocketMap.find(port) == _globalSocketMap.end()) {
			// Puerto no usado, crear socket
			int sock = createSocket(ipPort);
			if (sock == -1) {
				// Error al crear socket - verificar si es porque el puerto ya está en uso
				if (errno == EADDRINUSE) {
					std::ostringstream oss;
					oss << "Error: Puerto " << port << " ya está en uso";
					throw std::runtime_error(oss.str());
				}
				// Otro error
				std::ostringstream oss;
				oss << "Error: Failed to create socket for " << ipPort;
				throw std::runtime_error(oss.str());
			}
			_globalSocketMap[port] = sock;
			_portToIpPort[port] = ipPort;  // Guardar el ipPort original
		}
		
		// Verificar que el socket es válido antes de agregarlo
		std::map<int, int>::iterator it = _globalSocketMap.find(port);
		if (it != _globalSocketMap.end() && it->second >= 0) {
			_listenSockets.push_back(it->second);
		} else {
			std::ostringstream oss;
			oss << "Error: Invalid socket for port " << port;
			throw std::runtime_error(oss.str());
		}
	}
}

Server::~Server() {
	// ⚠️ No cerramos los sockets aquí, porque son compartidos por múltiples servidores.
}

int Server::createSocket(const std::string& ipPort) {
	std::string ip = "0.0.0.0";
	int port = 0;

	size_t colon = ipPort.find(':');
	if (colon != std::string::npos) {
		ip = ipPort.substr(0, colon);
		port = std::atoi(ipPort.substr(colon + 1).c_str());
	} else {
		port = std::atoi(ipPort.c_str());
	}

	// Validar puerto
	if (port <= 0 || port > 65535) {
		std::cerr << "Error: Puerto inválido: " << port << std::endl;
		return -1;
	}

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket");
		return -1;
	}

	int opt = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		perror("setsockopt");

	// 🔹 Hacer socket no bloqueante
	int flags = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if (inet_pton(AF_INET, ip.c_str(), &(addr.sin_addr)) <= 0) {
		std::cerr << "Error: dirección IP inválida: " << ip << std::endl;
		close(sockfd);
		return -1;
	}

	if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("bind");
		close(sockfd);
		return -1;
	}

	if (listen(sockfd, 128) < 0) {
		perror("listen");
		close(sockfd);
		return -1;
	}

	return sockfd;
}

const std::vector<int>& Server::getSockets() const {
	return _listenSockets;
}
