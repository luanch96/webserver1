/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: joserra <joserra@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/09 01:35:32 by joserra           #+#    #+#             */
/*   Updated: 2025/10/29 22:00:00 by joserra          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include "Server.hpp"
#include "Listener.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
	try {
		std::string filename = "conf/default.conf";
		if (argc > 1) {
			filename = argv[1];
		}

		ConfigParser parser(filename);
		parser.parse();

		const std::vector<ServerConfig>& serverConfigs = parser.getServers();
		if (serverConfigs.empty()) {
			std::cerr << "Error: No server blocks found in config file" << std::endl;
			return 1;
		}

		// Create Server instances
		std::vector<Server*> servers;
		for (size_t i = 0; i < serverConfigs.size(); ++i) {
			servers.push_back(new Server(serverConfigs[i]));
		}

		// Create Listener and run
		Listener listener(servers, serverConfigs);
		listener.run();

		// Cleanup (unreachable in normal operation)
		for (size_t i = 0; i < servers.size(); ++i) {
			delete servers[i];
		}
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << '\n';
		return 1;
	}

	return 0;
}
