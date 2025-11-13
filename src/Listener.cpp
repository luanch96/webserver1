/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Listener.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: luis <luis@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/29 22:54:48 by luis              #+#    #+#             */
/*   Updated: 2025/10/29 22:54:48 by luis             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Listener.hpp"
#include "ClientConnection.hpp"
#include "ConfigParser.hpp"
#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <ctime>
#include <cerrno>

Listener::Listener(const std::vector<Server*>& servers, const std::vector<ServerConfig>& configs)
	: _servers(servers), _serverConfigs(&configs) {
	setupPollFDs();
}

Listener::~Listener() {
	for (size_t i = 0; i < _connections.size(); ++i) {
		delete _connections[i];
	}
}

void Listener::setupPollFDs() {
	_pollfds.clear();
	for (size_t i = 0; i < _servers.size(); ++i) {
		const std::vector<int>& sockets = _servers[i]->getSockets();
		for (size_t j = 0; j < sockets.size(); ++j) {
			pollfd pfd;
			pfd.fd = sockets[j];
			pfd.events = POLLIN;
			pfd.revents = 0;
			_pollfds.push_back(pfd);
		}
	}
}

void Listener::run() {
	while (true) {
		_pollfds.clear();
		
		// Add listening sockets
		for (size_t i = 0; i < _servers.size(); ++i) {
			const std::vector<int>& sockets = _servers[i]->getSockets();
			for (size_t j = 0; j < sockets.size(); ++j) {
				pollfd pfd;
				pfd.fd = sockets[j];
				pfd.events = POLLIN;
				pfd.revents = 0;
				_pollfds.push_back(pfd);
			}
		}
		
		// Add client connections (monitor read and write at the same time)
		for (size_t i = 0; i < _connections.size(); ++i) {
			if (!_connections[i]->shouldClose()) {
				// Si hay CGI activo, monitorear sus pipes
				if (_connections[i]->isCGIActive()) {
					// Pipe para escribir al CGI
					int cgiWriteFd = _connections[i]->getCGIWriteFd();
					if (cgiWriteFd >= 0) {
						pollfd pfd;
						pfd.fd = cgiWriteFd;
						pfd.events = POLLOUT;
						pfd.revents = 0;
						_pollfds.push_back(pfd);
					}
					// Pipe para leer del CGI
					int cgiReadFd = _connections[i]->getCGIReadFd();
					if (cgiReadFd >= 0) {
						pollfd pfd;
						pfd.fd = cgiReadFd;
						pfd.events = POLLIN;
						pfd.revents = 0;
						_pollfds.push_back(pfd);
					}
				} else {
					// Conexión normal de cliente
					pollfd pfd;
					pfd.fd = _connections[i]->getFd();
					// La scale exige vigilar lectura y escritura simultáneamente
					pfd.events = POLLIN | POLLOUT;
					pfd.revents = 0;
					_pollfds.push_back(pfd);
				}
			}
		}
		
		if (_pollfds.empty()) {
			checkTimeouts();
			continue;
		}
		
		int ret = poll(&_pollfds[0], _pollfds.size(), 1000); // 1 second timeout
		if (ret < 0) {
			perror("poll");
			break;
		}
		
		if (ret == 0) {
			// Timeout - check for timed out connections
			checkTimeouts();
			continue;
		}
		
		std::vector<ClientConnection*> newConnections;
		
		// First, handle listening sockets (new connections)
		for (size_t i = 0; i < _pollfds.size(); ++i) {
			if (_pollfds[i].revents == 0) continue;
			if (_pollfds[i].fd < 0) continue;
			
			if (isListeningSocket(_pollfds[i].fd)) {
				if (_pollfds[i].revents & POLLIN) {
					handleNewConnection(_pollfds[i].fd, newConnections);
				}
			}
		}
		
		// Add new connections before handling client connections
		for (size_t i = 0; i < newConnections.size(); ++i) {
			_connections.push_back(newConnections[i]);
		}
		
		// Then handle existing client connections and CGI pipes
		for (size_t i = 0; i < _pollfds.size(); ++i) {
			if (_pollfds[i].revents == 0) continue;
			if (_pollfds[i].fd < 0) continue;
			
			if (!isListeningSocket(_pollfds[i].fd)) {
				// Verificar si es un pipe de CGI
				ClientConnection* cgiConn = findConnectionByCGIFd(_pollfds[i].fd);
				if (cgiConn) {
					handleCGIPipe(cgiConn, _pollfds[i].fd, _pollfds[i].revents);
				} else {
					// Es una conexión normal de cliente
					handleClientConnection(_pollfds[i].fd, _pollfds[i].revents);
				}
			}
		}
		
		// Clean up closed connections
		cleanupConnections();
	}
}

bool Listener::isListeningSocket(int fd) const {
	for (size_t i = 0; i < _servers.size(); ++i) {
		const std::vector<int>& sockets = _servers[i]->getSockets();
		for (size_t j = 0; j < sockets.size(); ++j) {
			if (sockets[j] == fd)
				return true;
		}
	}
	return false;
}

void Listener::handleNewConnection(int fd, std::vector<ClientConnection*>& newConnections) {
	if (!isListeningSocket(fd)) {
		return; // Safety check
	}
	
	struct sockaddr_in clientAddr;
	socklen_t addrLen = sizeof(clientAddr);
	int clientFd = accept(fd, (struct sockaddr*)&clientAddr, &addrLen);
	
	if (clientFd < 0) {
		// accept failed: could be EAGAIN/EWOULDBLOCK (normal for non-blocking)
		// or actual error - don't check errno, just return and retry on next poll
		return;
	}
	
	ClientConnection* conn = new ClientConnection(clientFd);
	newConnections.push_back(conn);
}

void Listener::handleClientConnection(int fd, short revents) {
	ClientConnection* conn = findConnection(fd);
	if (!conn) return;
	
	if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
		// Error en el socket - cerrar y marcar para eliminación
		conn->close(); // close() ahora también setea _shouldClose = true
		return;
	}
	// Solo una operación por cliente por iteración
	if (conn->getState() == READING_REQUEST) {
		if (revents & POLLIN) {
			if (conn->readRequest()) {
				conn->processRequest(*_serverConfigs);
			}
		}
	} else if (conn->getState() == WRITING_RESPONSE) {
		if (revents & POLLOUT) {
			conn->writeResponse();
		}
	}
	// Estados WRITING_TO_CGI y READING_FROM_CGI se manejan en handleCGIPipe
}

void Listener::handleCGIPipe(ClientConnection* conn, int fd, short revents) {
	if (!conn) return;
	
	if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
		// Error en el pipe - cerrar conexión
		conn->close();
		return;
	}
	
	// Verificar qué pipe es
	if (fd == conn->getCGIWriteFd()) {
		// Pipe para escribir al CGI
		if (revents & POLLOUT) {
			conn->writeToCGI();
		}
	} else if (fd == conn->getCGIReadFd()) {
		// Pipe para leer del CGI
		if (revents & POLLIN) {
			conn->readFromCGI();
		}
	}
}

ClientConnection* Listener::findConnection(int fd) {
	for (size_t i = 0; i < _connections.size(); ++i) {
		if (_connections[i]->getFd() == fd) {
			return _connections[i];
		}
	}
	return NULL;
}

ClientConnection* Listener::findConnectionByCGIFd(int fd) {
	for (size_t i = 0; i < _connections.size(); ++i) {
		if (_connections[i]->isCGIActive()) {
			if (_connections[i]->getCGIWriteFd() == fd || 
				_connections[i]->getCGIReadFd() == fd) {
				return _connections[i];
			}
		}
	}
	return NULL;
}

void Listener::cleanupConnections() {
	for (std::vector<ClientConnection*>::iterator it = _connections.begin();
		 it != _connections.end();) {
		if ((*it)->shouldClose()) {
			delete *it;
			it = _connections.erase(it);
		} else {
			++it;
		}
	}
}

void Listener::checkTimeouts() {
	for (size_t i = 0; i < _connections.size(); ++i) {
		if (_connections[i]->isTimeout(30)) { // 30 second timeout
			_connections[i]->close();
		}
	}
}
