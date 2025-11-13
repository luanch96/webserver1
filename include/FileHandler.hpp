/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FileHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: luis <luis@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/29 22:54:48 by luis              #+#    #+#             */
/*   Updated: 2025/10/29 22:54:48 by luis             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FILE_HANDLER_HPP
#define FILE_HANDLER_HPP

#include "Request.hpp"
#include "Response.hpp"
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"

class FileHandler {
public:
	static void handleGet(const Request& request, const std::string& filePath,
						  const ServerConfig* server, const LocationConfig* location,
						  Response& response);
	
	static void handlePost(const Request& request, const std::string& filePath,
						   const ServerConfig* server, const LocationConfig* location,
						   Response& response);
	
	static void handleDelete(const Request& request, const std::string& filePath,
							 const ServerConfig* server, const LocationConfig* location,
							 Response& response);

private:
	static void handleError(int code, const ServerConfig* server, Response& response);
	static std::string findIndexFile(const std::string& dirPath, const std::string& index);
};

#endif

