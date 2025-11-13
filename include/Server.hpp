/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: joserra <joserra@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/09 01:36:38 by joserra           #+#    #+#             */
/*   Updated: 2025/08/12 23:51:35 by joserra          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include "ServerConfig.hpp"
#include <map>
#include <string>
#include <vector>

class Server {
public:
	Server(const ServerConfig& config);
	~Server();

	static int createSocket(const std::string& ipPort);
	const std::vector<int>& getSockets() const;

private:
	ServerConfig _config;
	std::vector<int> _listenSockets;

	static std::map<int, int> _globalSocketMap;  // 🧠 Sockets compartidos por puerto
	static std::map<int, std::string> _portToIpPort;  // Mapeo puerto -> ipPort original
	
	// Normalizar puerto: extrae el número de puerto de diferentes formatos
	static int normalizePort(const std::string& ipPort);
};

#endif
// incluimos cambios
