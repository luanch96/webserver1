/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Listener.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: joserra <joserra@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/09 01:36:58 by joserra           #+#    #+#             */
/*   Updated: 2025/08/12 23:50:16 by joserra          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LISTENER_HPP
#define LISTENER_HPP

#include "Server.hpp"
#include "ClientConnection.hpp"
#include "ServerConfig.hpp"
#include <vector>
#include <poll.h>
#include <ctime>

class Listener {
	public:
		Listener(const std::vector<Server*>& servers, const std::vector<ServerConfig>& configs);
		~Listener();

		void run(); //Bucle principal de eventos

		bool isListeningSocket(int fd) const;

	private:
		std::vector<pollfd> _pollfds;
		std::vector<Server*> _servers;
		std::vector<ClientConnection*> _connections;
		const std::vector<ServerConfig>* _serverConfigs;

		void setupPollFDs();
		void handleNewConnection(int fd, std::vector<ClientConnection*>& newConnections);
		void handleClientConnection(int fd, short revents);
		void handleCGIPipe(ClientConnection* conn, int fd, short revents);
		ClientConnection* findConnection(int fd);
		ClientConnection* findConnectionByCGIFd(int fd);
		void cleanupConnections();
		void checkTimeouts();
};

#endif