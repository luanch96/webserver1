/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FileHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: luis <luis@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/29 22:54:48 by luis              #+#    #+#             */
/*   Updated: 2025/10/29 22:54:48 by luis             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "FileHandler.hpp"
#include "Utils.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <ctime>
#include <cstdio>

void FileHandler::handleGet(const Request& request, const std::string& filePath,
							const ServerConfig* server, const LocationConfig* location,
							Response& response) {
	
	// Check if filePath is a directory first (before checking if file exists)
	// This handles the case where buildFilePath might return index.html but we want directory listing
	std::string actualPath = filePath;
	if (location && location->autoindex && Utils::isDirectory(filePath)) {
		// Force directory path (remove index.html if present)
		if (filePath.find("index.html") != std::string::npos) {
			size_t idxPos = filePath.find("index.html");
			actualPath = filePath.substr(0, idxPos);
			// Remove trailing slash if any
			if (actualPath.length() > 0 && actualPath[actualPath.length() - 1] == '/') {
				actualPath = actualPath.substr(0, actualPath.length() - 1);
			}
		}
	}
	
	if (!Utils::fileExists(actualPath) && !Utils::isDirectory(actualPath)) {
		handleError(404, server, response);
		return;
	}
	
	if (Utils::isDirectory(actualPath)) {
		// If autoindex is enabled, show directory listing (priority)
		if (location && location->autoindex) {
			std::string autoindex = Utils::generateAutoindex(filePath, request.getPath());
			response.setStatus(200);
			response.setBody(autoindex);
			response.setHeader("Content-Type", "text/html; charset=utf-8");
			return;
		}
		
		// Otherwise, try to serve index file
		std::string indexFile = findIndexFile(filePath, server->index);
		if (!indexFile.empty() && Utils::fileExists(indexFile)) {
			// Serve the index file directly
			std::string content = Utils::readFile(indexFile);
			response.setStatus(200);
			response.setBody(content);
			response.setHeader("Content-Type", Utils::getMimeType(indexFile));
			return;
		}
		
		// No index file and no autoindex - forbidden
		handleError(403, server, response);
		return;
	}
	
	if (!Utils::fileExists(filePath)) {
		handleError(404, server, response);
		return;
	}
	
	std::string content = Utils::readFile(filePath);
	response.setStatus(200);
	response.setBody(content);
	response.setHeader("Content-Type", Utils::getMimeType(filePath));
}

void FileHandler::handlePost(const Request& request, const std::string& filePath,
							  const ServerConfig* server, const LocationConfig* location,
							  Response& response) {
    // Enforce client_max_body_size (location overrides server)
    size_t limit = 0;
    if (server) {
        limit = server->clientMaxBodySize;
    }
    if (location && location->clientMaxBodySize > 0) {
        limit = location->clientMaxBodySize;
    }
    if (limit > 0 && request.getBody().size() > limit) {
        handleError(413, server, response);
        return;
    }

	std::string uploadPath = filePath;
	if (Utils::isDirectory(filePath)) {
		std::ostringstream oss;
		oss << uploadPath << "/upload_" << time(NULL);
		uploadPath = oss.str();
	}
	
	std::ofstream file(uploadPath.c_str(), std::ios::binary);
	if (!file.is_open()) {
		handleError(500, server, response);
		return;
	}
	
	file.write(request.getBody().c_str(), request.getBody().size());
	file.close();
	
	response.setStatus(201);
	response.setBody("File uploaded successfully");
	response.setHeader("Location", uploadPath);
}

void FileHandler::handleDelete(const Request& /*request*/, const std::string& filePath,
								const ServerConfig* server, const LocationConfig* /*location*/,
								Response& response) {
	
	if (!Utils::fileExists(filePath)) {
		handleError(404, server, response);
		return;
	}
	
	if (Utils::isDirectory(filePath)) {
		handleError(403, server, response);
		return;
	}
	
	if (std::remove(filePath.c_str()) == 0) {
		response.setStatus(204);
		response.setBody("");
	} else {
		handleError(500, server, response);
	}
}

void FileHandler::handleError(int code, const ServerConfig* server, Response& response) {
	response.setStatus(code);
	
	if (server) {
		std::map<int, std::string>::const_iterator it = server->errorPages.find(code);
		if (it != server->errorPages.end()) {
			std::string errorPath = server->root + it->second;
			if (Utils::fileExists(errorPath)) {
				std::string content = Utils::readFile(errorPath);
				response.setBody(content);
				response.setHeader("Content-Type", "text/html; charset=utf-8");
				return;
			}
		}
	}
	
	std::ostringstream oss;
	oss << "<html><head><title>" << code << " Error</title></head>";
	oss << "<body><h1>" << code << " Error</h1></body></html>";
	response.setBody(oss.str());
	response.setHeader("Content-Type", "text/html; charset=utf-8");
}

std::string FileHandler::findIndexFile(const std::string& dirPath, const std::string& index) {
	if (index.empty()) {
		if (Utils::fileExists(dirPath + "/index.html")) {
			return dirPath + "/index.html";
		}
		return "";
	}
	return dirPath + "/" + index;
}

