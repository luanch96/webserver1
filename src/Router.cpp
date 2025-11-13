/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: luis <luis@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/29 22:54:48 by luis              #+#    #+#             */
/*   Updated: 2025/10/29 22:54:48 by luis             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Router.hpp"
#include "Request.hpp"
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"
#include <algorithm>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

Router::RoutingResult Router::route(
	const std::vector<ServerConfig>& servers,
	const Request& request,
	int serverSocketFd
) {
	RoutingResult result;
	
	result.server = findServer(servers, request, serverSocketFd);
	if (!result.server) {
		return result;
	}
	
	result.location = findLocation(result.server, request.getPath());
	
	if (result.location) {
		// Check if method is allowed
		const std::vector<std::string>& allowed = result.location->allowedMethods;
		bool methodAllowed = false;
		for (size_t i = 0; i < allowed.size(); ++i) {
			if (allowed[i] == request.getMethod()) {
				methodAllowed = true;
				break;
			}
		}
		if (!methodAllowed && !allowed.empty()) {
			return result; // Will return 405
		}
	}
	
	result.filePath = buildFilePath(result.server, result.location, request.getPath());
	result.isCGI = isCGIRequest(result.location, result.filePath);
	if (result.isCGI) {
		result.cgiExecutor = getCGIExecutor(result.location, result.filePath);
	}
	
	return result;
}

const ServerConfig* Router::findServer(
	const std::vector<ServerConfig>& servers,
	const Request& request,
	int serverSocketFd
) {
	std::string hostHeader = request.getHeader("host");
	std::string hostName;
	
	if (!hostHeader.empty()) {
		size_t colon = hostHeader.find(':');
		if (colon != std::string::npos) {
			hostName = hostHeader.substr(0, colon);
		} else {
			hostName = hostHeader;
		}
	}
	
	// Get socket address info
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	getsockname(serverSocketFd, (struct sockaddr*)&addr, &len);
	int socketPort = ntohs(addr.sin_port);
	
	const ServerConfig* bestMatch = NULL;
	const ServerConfig* defaultServer = NULL;
	
	for (size_t i = 0; i < servers.size(); ++i) {
		const ServerConfig& server = servers[i];
		
		// Check if server listens on this socket's port
		bool listensOnPort = false;
		for (size_t j = 0; j < server.listen.size(); ++j) {
			size_t colon = server.listen[j].find(':');
			int port = 0;
			if (colon != std::string::npos) {
				port = atoi(server.listen[j].substr(colon + 1).c_str());
			} else {
				port = atoi(server.listen[j].c_str());
			}
			if (port == socketPort || (port == 0 && socketPort == 80)) {
				listensOnPort = true;
				break;
			}
		}
		
		if (!listensOnPort) continue;
		
		if (defaultServer == NULL) {
			defaultServer = &server;
		}
		
		// Match by server_name
		if (!hostName.empty()) {
			for (size_t j = 0; j < server.serverNames.size(); ++j) {
				if (server.serverNames[j] == hostName) {
					return &server; // Exact match
				}
			}
		}
	}
	
	return bestMatch ? bestMatch : defaultServer;
}

const LocationConfig* Router::findLocation(
	const ServerConfig* server,
	const std::string& path
) {
	if (!server) return NULL;
	
	const LocationConfig* bestMatch = NULL;
	size_t bestLength = 0;
	
	for (size_t i = 0; i < server->locations.size(); ++i) {
		const LocationConfig& loc = server->locations[i];
		if (path.find(loc.path) == 0 && loc.path.length() > bestLength) {
			bestMatch = &loc;
			bestLength = loc.path.length();
		}
	}
	
	return bestMatch;
}

std::string Router::buildFilePath(
	const ServerConfig* server,
	const LocationConfig* location,
	const std::string& requestPath
) {
	if (!server) return "";
	
	std::string root = server->root;
	if (location && !location->root.empty()) {
		root = location->root;
	}
	
	if (root.empty()) {
		root = "./www";
	}
	
	std::string path = requestPath;
	if (location && !location->path.empty()) {
		// Remove location path prefix
		if (path.find(location->path) == 0) {
			path = path.substr(location->path.length());
		}
	}
	
	if (path.empty() || path == "/") {
		path = "/";
		if (!server->index.empty()) {
			return root + path + server->index;
		}
		return root + "/";
	}
	
	// Remove leading slash if root doesn't end with one
	if (path[0] == '/' && root[root.length() - 1] == '/') {
		path = path.substr(1);
	} else if (path[0] != '/' && root[root.length() - 1] != '/') {
		path = "/" + path;
	}
	
	return root + path;
}

bool Router::isCGIRequest(
	const LocationConfig* location,
	const std::string& filePath
) {
	if (!location || location->cgiPass.empty()) {
		return false;
	}
	
	size_t dot = filePath.find_last_of('.');
	if (dot == std::string::npos) {
		return false;
	}
	
	std::string ext = filePath.substr(dot);
	return location->cgiPass.find(ext) != location->cgiPass.end();
}

std::string Router::getCGIExecutor(
	const LocationConfig* location,
	const std::string& filePath
) {
	if (!location) return "";
	
	size_t dot = filePath.find_last_of('.');
	if (dot == std::string::npos) {
		return "";
	}
	
	std::string ext = filePath.substr(dot);
	std::map<std::string, std::string>::const_iterator it = location->cgiPass.find(ext);
	if (it != location->cgiPass.end()) {
		return it->second;
	}
	
	return "";
}

