/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientConnection.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: luis <luis@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/29 22:54:48 by luis              #+#    #+#             */
/*   Updated: 2025/10/29 22:54:48 by luis             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ClientConnection.hpp"
#include "Router.hpp"
#include "FileHandler.hpp"
#include "Utils.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <cerrno>
#include <iostream>
#include <cstring>
#include <sstream>
#include <map>
#include <cstdlib>

ClientConnection::ClientConnection(int fd) 
	: _fd(fd), _state(READING_REQUEST), _responseSent(0), _shouldClose(false),
	  _cgiPid(-1), _cgiActive(false), _cgiBodySent(0) {
	updateLastActivity();
	
	// Set non-blocking
	int flags = fcntl(_fd, F_GETFL, 0);
	fcntl(_fd, F_SETFL, flags | O_NONBLOCK);
	
	// Initialize CGI pipes to invalid
	_cgiPipeIn[0] = -1;
	_cgiPipeIn[1] = -1;
	_cgiPipeOut[0] = -1;
	_cgiPipeOut[1] = -1;
}

ClientConnection::~ClientConnection() {
	cleanupCGI();
	close();
}

int ClientConnection::getFd() const {
	return _fd;
}

ConnectionState ClientConnection::getState() const {
	return _state;
}

bool ClientConnection::readRequest() {
	char buffer[4096];
	ssize_t bytes = recv(_fd, buffer, sizeof(buffer) - 1, 0);
	
    if (bytes == -1) {
        // Error real
        _shouldClose = true;
        return false;
    }
    if (bytes == 0) {
        // EOF/cierre de conexión
        _shouldClose = true;
        return false;
    }
	
	buffer[bytes] = '\0';
	std::string chunk(buffer, bytes);
	
	if (_request.parseChunk(chunk)) {
		_state = PROCESSING;
		updateLastActivity();
		return true;
	}
	
	updateLastActivity();
	return false;
}

bool ClientConnection::processRequest(const std::vector<ServerConfig>& servers) {
	Router::RoutingResult routing = Router::route(servers, _request, _fd);
	
	if (!routing.server) {
		_response.setStatus(500);
		_response.setBody("Internal Server Error");
		_state = WRITING_RESPONSE;
		_responseBuffer = _response.buildResponse();
		return true;
	}
	
    if (routing.location) {
        // Redirección si la location lo define
        if (!routing.location->redirect.empty()) {
            _response.setStatus(301, "Moved Permanently");
            _response.setHeader("Location", routing.location->redirect);
            _response.setBody("");
            _state = WRITING_RESPONSE;
            _responseBuffer = _response.buildResponse();
            return true;
        }
		const std::vector<std::string>& allowed = routing.location->allowedMethods;
		bool methodAllowed = false;
		for (size_t i = 0; i < allowed.size(); ++i) {
			if (allowed[i] == _request.getMethod()) {
				methodAllowed = true;
				break;
			}
		}
		if (!methodAllowed && !allowed.empty()) {
			_response.setStatus(405, "Method Not Allowed");
			_response.setBody("405 Method Not Allowed");
			_state = WRITING_RESPONSE;
			_responseBuffer = _response.buildResponse();
			return true;
		}
	}
	
    // Priorizar client_max_body_size de location sobre el del server
    size_t maxBodySize = routing.server->clientMaxBodySize;
    if (routing.location && routing.location->clientMaxBodySize > 0) {
        maxBodySize = routing.location->clientMaxBodySize;
    }
    // Validar por Content-Length y también por tamaño real del cuerpo parseado
    size_t effectiveBody = _request.getContentLength();
    if (effectiveBody == 0) {
        effectiveBody = _request.getBody().size();
    }
    if (maxBodySize > 0 && effectiveBody > maxBodySize) {
		_response.setStatus(413, "Payload Too Large");
		_response.setBody("413 Payload Too Large");
		_state = WRITING_RESPONSE;
		_responseBuffer = _response.buildResponse();
		return true;
	}
	
	if (routing.isCGI) {
		// Inicializar CGI de forma asíncrona
		if (initCGI(routing.filePath, _request, *routing.server, routing.location)) {
			// Si hay body para enviar, empezar escribiendo al CGI
			if (!_cgiRequestBody.empty()) {
				_state = WRITING_TO_CGI;
			} else {
				// Si no hay body, empezar leyendo del CGI
				_state = READING_FROM_CGI;
			}
		} else {
			_response.setStatus(500);
			_response.setBody("CGI Execution Failed");
			_state = WRITING_RESPONSE;
			_responseBuffer = _response.buildResponse();
		}
		return true;
	} else {
		if (_request.getMethod() == "GET" || _request.getMethod() == "HEAD") {
			FileHandler::handleGet(_request, routing.filePath, routing.server, routing.location, _response);
		} else if (_request.getMethod() == "POST") {
			FileHandler::handlePost(_request, routing.filePath, routing.server, routing.location, _response);
		} else if (_request.getMethod() == "DELETE") {
			FileHandler::handleDelete(_request, routing.filePath, routing.server, routing.location, _response);
		} else {
			// Método desconocido/no implementado - retornar 501 Not Implemented
			_response.setStatus(501, "Not Implemented");
			_response.setBody("501 Not Implemented");
		}
	}
	
	// Connection header
	std::string connection = _request.getHeader("connection");
	if (connection.empty() || toLowerCase(connection) == "keep-alive") {
		_response.setHeader("Connection", "keep-alive");
	} else {
		_response.setHeader("Connection", "close");
		_shouldClose = true;
	}
	
	_state = WRITING_RESPONSE;
	_responseBuffer = _response.buildResponse();
	_responseSent = 0;
	return true;
}

bool ClientConnection::writeResponse() {
	if (_responseSent >= _responseBuffer.size()) {
		if (_shouldClose) {
			_state = CLOSING;
		} else {
			_request.reset();
			_response.clear();
			_responseBuffer.clear();
			_responseSent = 0;
			_state = READING_REQUEST;
		}
		return true;
	}
	
	ssize_t bytes = send(_fd, _responseBuffer.c_str() + _responseSent, 
						_responseBuffer.size() - _responseSent, 0);
	
    if (bytes == -1) {
        // Error real
        _shouldClose = true;
        return false;
    }
    if (bytes == 0) {
        // EOF/cierre de conexión
        _shouldClose = true;
        return false;
    }
	
	_responseSent += bytes;
	return _responseSent >= _responseBuffer.size();
}

void ClientConnection::updateLastActivity() {
	_lastActivity = time(NULL);
}

time_t ClientConnection::getLastActivity() const {
	return _lastActivity;
}

bool ClientConnection::isTimeout(time_t timeoutSeconds) const {
	return (time(NULL) - _lastActivity) > timeoutSeconds;
}

bool ClientConnection::shouldClose() const {
	return _shouldClose || _state == CLOSING;
}

void ClientConnection::close() {
	if (_fd >= 0) {
		::close(_fd);
		_fd = -1;
		_shouldClose = true; // Marcar para eliminación en cleanupConnections
	}
}

std::string ClientConnection::toLowerCase(const std::string& str) const {
	std::string result = str;
	for (size_t i = 0; i < result.size(); ++i) {
		result[i] = std::tolower(result[i]);
	}
	return result;
}

// CGI async methods
bool ClientConnection::initCGI(const std::string& scriptPath, const Request& request,
								const ServerConfig& /*server*/, const LocationConfig* location) {
	cleanupCGI(); // Limpiar cualquier CGI previo
	
	// Crear pipes
	if (pipe(_cgiPipeIn) < 0 || pipe(_cgiPipeOut) < 0) {
		return false;
	}
	
	// Hacer pipes no bloqueantes
	int flags = fcntl(_cgiPipeIn[1], F_GETFL, 0);
	fcntl(_cgiPipeIn[1], F_SETFL, flags | O_NONBLOCK);
	flags = fcntl(_cgiPipeOut[0], F_GETFL, 0);
	fcntl(_cgiPipeOut[0], F_SETFL, flags | O_NONBLOCK);
	
	// Guardar body para enviar después
	_cgiRequestBody = request.getBody();
	_cgiBodySent = 0;
	_cgiOutput.clear();
	_cgiContentType = "text/html";
	
	// Fork
	_cgiPid = fork();
	if (_cgiPid < 0) {
		cleanupCGI();
		return false;
	}
	
	if (_cgiPid == 0) {
		// Child process
		dup2(_cgiPipeIn[0], STDIN_FILENO);
		dup2(_cgiPipeOut[1], STDOUT_FILENO);
		dup2(_cgiPipeOut[1], STDERR_FILENO);
		
		::close(_cgiPipeIn[0]);
		::close(_cgiPipeIn[1]);
		::close(_cgiPipeOut[0]);
		::close(_cgiPipeOut[1]);
		
		// Build environment
		std::vector<std::string> envVars;
		envVars.push_back("REQUEST_METHOD=" + request.getMethod());
		envVars.push_back("SCRIPT_FILENAME=" + scriptPath);
		envVars.push_back("SCRIPT_NAME=" + request.getPath());
		envVars.push_back("QUERY_STRING=" + request.getQuery());
		envVars.push_back("PATH_INFO=" + request.getPath());
		envVars.push_back("SERVER_SOFTWARE=webserv/1.0");
		
		std::string host = request.getHeader("host");
		if (!host.empty()) {
			envVars.push_back("HTTP_HOST=" + host);
		}
		
		std::string contentType = request.getHeader("content-type");
		if (!contentType.empty()) {
			envVars.push_back("CONTENT_TYPE=" + contentType);
			std::ostringstream oss;
			oss << request.getContentLength();
			envVars.push_back("CONTENT_LENGTH=" + oss.str());
		}
		
		char** env = new char*[envVars.size() + 1];
		for (size_t i = 0; i < envVars.size(); ++i) {
			env[i] = new char[envVars[i].size() + 1];
			std::strcpy(env[i], envVars[i].c_str());
		}
		env[envVars.size()] = NULL;
		
		// Get executor
		std::string executor;
		if (location) {
			size_t dot = scriptPath.find_last_of('.');
			if (dot != std::string::npos) {
				std::string ext = scriptPath.substr(dot);
				std::map<std::string, std::string>::const_iterator it = location->cgiPass.find(ext);
				if (it != location->cgiPass.end()) {
					executor = it->second;
				}
			}
		}
		
		if (executor.empty()) {
			for (size_t i = 0; env[i]; ++i) {
				delete[] env[i];
			}
			delete[] env;
			exit(1);
		}
		
		const char* args[] = { executor.c_str(), scriptPath.c_str(), NULL };
		execve(executor.c_str(), (char* const*)args, env);
		
		for (size_t i = 0; env[i]; ++i) {
			delete[] env[i];
		}
		delete[] env;
		exit(1);
	} else {
		// Parent process
		::close(_cgiPipeIn[0]);
		::close(_cgiPipeOut[1]);
		_cgiActive = true;
		updateLastActivity();
		return true;
	}
}

bool ClientConnection::writeToCGI() {
	if (!_cgiActive || _cgiPipeIn[1] < 0) {
		return false;
	}
	
	if (_cgiBodySent >= _cgiRequestBody.size()) {
		// Body completo enviado, cerrar pipe y cambiar a lectura
		::close(_cgiPipeIn[1]);
		_cgiPipeIn[1] = -1;
		_state = READING_FROM_CGI;
		updateLastActivity();
		return true;
	}
	
	ssize_t bytes = write(_cgiPipeIn[1], 
						  _cgiRequestBody.c_str() + _cgiBodySent,
						  _cgiRequestBody.size() - _cgiBodySent);
	
	if (bytes == -1) {
		// Error real - cerrar y pasar a lectura
		::close(_cgiPipeIn[1]);
		_cgiPipeIn[1] = -1;
		_state = READING_FROM_CGI;
		updateLastActivity();
		return true;
	}
	if (bytes == 0) {
		// EOF - cerrar y pasar a lectura
		::close(_cgiPipeIn[1]);
		_cgiPipeIn[1] = -1;
		_state = READING_FROM_CGI;
		updateLastActivity();
		return true;
	}
	
	_cgiBodySent += bytes;
	updateLastActivity();
	return false; // Aún hay más que escribir
}

bool ClientConnection::readFromCGI() {
	if (!_cgiActive || _cgiPipeOut[0] < 0) {
		return false;
	}
	
	char buffer[8192];
	ssize_t bytes = read(_cgiPipeOut[0], buffer, sizeof(buffer) - 1);
	
	if (bytes == -1) {
		// Error real - procesar output y finalizar
		::close(_cgiPipeOut[0]);
		_cgiPipeOut[0] = -1;
		
		// Esperar al proceso hijo
		if (_cgiPid > 0) {
			waitpid(_cgiPid, NULL, 0);
			_cgiPid = -1;
		}
		
		// Parsear Content-Type del output
		size_t ctPos = _cgiOutput.find("Content-Type:");
		if (ctPos != std::string::npos) {
			size_t endLine = _cgiOutput.find("\r\n", ctPos);
			if (endLine != std::string::npos) {
				std::string ctLine = _cgiOutput.substr(ctPos + 13, endLine - ctPos - 13);
				size_t start = ctLine.find_first_not_of(" \t");
				if (start != std::string::npos) {
					size_t end = ctLine.find_first_of(" \r\n", start);
					_cgiContentType = ctLine.substr(start, end - start);
				}
			}
			// Remover headers HTTP del output
			size_t bodyStart = _cgiOutput.find("\r\n\r\n");
			if (bodyStart != std::string::npos) {
				_cgiOutput = _cgiOutput.substr(bodyStart + 4);
			}
		}
		
		// Preparar respuesta
		_response.setStatus(200);
		_response.setBody(_cgiOutput);
		_response.setHeader("Content-Type", _cgiContentType);
		
		// Connection header
		std::string connection = _request.getHeader("connection");
		if (connection.empty() || toLowerCase(connection) == "keep-alive") {
			_response.setHeader("Connection", "keep-alive");
		} else {
			_response.setHeader("Connection", "close");
			_shouldClose = true;
		}
		
		_state = WRITING_RESPONSE;
		_responseBuffer = _response.buildResponse();
		_responseSent = 0;
		_cgiActive = false;
		updateLastActivity();
		return true;
	}
	if (bytes == 0) {
		// EOF - procesar output y finalizar
		::close(_cgiPipeOut[0]);
		_cgiPipeOut[0] = -1;
		
		// Esperar al proceso hijo
		if (_cgiPid > 0) {
			waitpid(_cgiPid, NULL, 0);
			_cgiPid = -1;
		}
		
		// Parsear Content-Type del output
		size_t ctPos = _cgiOutput.find("Content-Type:");
		if (ctPos != std::string::npos) {
			size_t endLine = _cgiOutput.find("\r\n", ctPos);
			if (endLine != std::string::npos) {
				std::string ctLine = _cgiOutput.substr(ctPos + 13, endLine - ctPos - 13);
				size_t start = ctLine.find_first_not_of(" \t");
				if (start != std::string::npos) {
					size_t end = ctLine.find_first_of(" \r\n", start);
					_cgiContentType = ctLine.substr(start, end - start);
				}
			}
			// Remover headers HTTP del output
			size_t bodyStart = _cgiOutput.find("\r\n\r\n");
			if (bodyStart != std::string::npos) {
				_cgiOutput = _cgiOutput.substr(bodyStart + 4);
			}
		}
		
		// Preparar respuesta
		_response.setStatus(200);
		_response.setBody(_cgiOutput);
		_response.setHeader("Content-Type", _cgiContentType);
		
		// Connection header
		std::string connection = _request.getHeader("connection");
		if (connection.empty() || toLowerCase(connection) == "keep-alive") {
			_response.setHeader("Connection", "keep-alive");
		} else {
			_response.setHeader("Connection", "close");
			_shouldClose = true;
		}
		
		_state = WRITING_RESPONSE;
		_responseBuffer = _response.buildResponse();
		_responseSent = 0;
		_cgiActive = false;
		updateLastActivity();
		return true;
	}
	
	buffer[bytes] = '\0';
	_cgiOutput += std::string(buffer, bytes);
	updateLastActivity();
	return false; // Aún hay más que leer
}

bool ClientConnection::isCGIActive() const {
	return _cgiActive;
}

int ClientConnection::getCGIWriteFd() const {
	return _cgiPipeIn[1];
}

int ClientConnection::getCGIReadFd() const {
	return _cgiPipeOut[0];
}

pid_t ClientConnection::getCGIPid() const {
	return _cgiPid;
}

void ClientConnection::cleanupCGI() {
	if (_cgiPipeIn[1] >= 0) {
		::close(_cgiPipeIn[1]);
		_cgiPipeIn[1] = -1;
	}
	if (_cgiPipeIn[0] >= 0) {
		::close(_cgiPipeIn[0]);
		_cgiPipeIn[0] = -1;
	}
	if (_cgiPipeOut[0] >= 0) {
		::close(_cgiPipeOut[0]);
		_cgiPipeOut[0] = -1;
	}
	if (_cgiPipeOut[1] >= 0) {
		::close(_cgiPipeOut[1]);
		_cgiPipeOut[1] = -1;
	}
	if (_cgiPid > 0) {
		waitpid(_cgiPid, NULL, 0);
		_cgiPid = -1;
	}
	_cgiActive = false;
}

