/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: luis <luis@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/29 22:54:48 by luis              #+#    #+#             */
/*   Updated: 2025/10/29 22:54:48 by luis             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "ServerConfig.hpp"
#include "LocationConfig.hpp"
#include "Request.hpp"
#include <vector>

class Router {
public:
	struct RoutingResult {
		const ServerConfig* server;
		const LocationConfig* location;
		std::string filePath;
		bool isCGI;
		std::string cgiExecutor;
		
		RoutingResult() : server(NULL), location(NULL), isCGI(false) {}
	};
	
	static RoutingResult route(
		const std::vector<ServerConfig>& servers,
		const Request& request,
		int serverSocketFd
	);
	
private:
	static const ServerConfig* findServer(
		const std::vector<ServerConfig>& servers,
		const Request& request,
		int serverSocketFd
	);
	
	static const LocationConfig* findLocation(
		const ServerConfig* server,
		const std::string& path
	);
	
	static std::string buildFilePath(
		const ServerConfig* server,
		const LocationConfig* location,
		const std::string& requestPath
	);
	
	static bool isCGIRequest(
		const LocationConfig* location,
		const std::string& filePath
	);
	
	static std::string getCGIExecutor(
		const LocationConfig* location,
		const std::string& filePath
	);
};

#endif

