/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientConnection.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: luis <luis@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/29 22:54:48 by luis              #+#    #+#             */
/*   Updated: 2025/10/29 22:54:48 by luis             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_CONNECTION_HPP
#define CLIENT_CONNECTION_HPP

#include "Request.hpp"
#include "Response.hpp"
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"
#include <string>
#include <ctime>
#include <vector>
#include <sys/types.h>

enum ConnectionState {
	READING_REQUEST,
	PROCESSING,
	WRITING_TO_CGI,
	READING_FROM_CGI,
	WRITING_RESPONSE,
	CLOSING
};

class ClientConnection {
public:
	ClientConnection(int fd);
	~ClientConnection();
	
	int getFd() const;
	ConnectionState getState() const;
	
	bool readRequest();
	bool processRequest(const std::vector<ServerConfig>& servers);
	bool writeResponse();
	
	// CGI async methods
	bool initCGI(const std::string& scriptPath, const Request& request,
				 const ServerConfig& server, const LocationConfig* location);
	bool writeToCGI();
	bool readFromCGI();
	bool isCGIActive() const;
	int getCGIWriteFd() const;
	int getCGIReadFd() const;
	pid_t getCGIPid() const;
	
	void updateLastActivity();
	time_t getLastActivity() const;
	bool isTimeout(time_t timeoutSeconds) const;
	
	bool shouldClose() const;
	void close();

private:
	int _fd;
	ConnectionState _state;
	Request _request;
	Response _response;
	std::string _responseBuffer;
	size_t _responseSent;
	time_t _lastActivity;
	bool _shouldClose;
	
	// CGI async state
	int _cgiPipeIn[2];   // pipeIn[1] es para escribir al CGI
	int _cgiPipeOut[2];  // pipeOut[0] es para leer del CGI
	pid_t _cgiPid;
	bool _cgiActive;
	std::string _cgiRequestBody;
	size_t _cgiBodySent;
	std::string _cgiOutput;
	std::string _cgiContentType;
	
	bool validateRequest(const ServerConfig* server, const LocationConfig* location);
	std::string toLowerCase(const std::string& str) const;
	void cleanupCGI();
};

#endif

