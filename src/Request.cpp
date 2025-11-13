/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: luis <luis@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/29 22:54:48 by luis              #+#    #+#             */
/*   Updated: 2025/10/29 22:54:48 by luis             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>

Request::Request() : _state(REQUEST_LINE), _contentLength(0), _chunked(false) {}

Request::~Request() {}

void Request::reset() {
	_state = REQUEST_LINE;
	_method.clear();
	_uri.clear();
	_path.clear();
	_query.clear();
	_version.clear();
	_headers.clear();
	_body.clear();
	_buffer.clear();
	_contentLength = 0;
	_chunked = false;
}

bool Request::parseChunk(const std::string& chunk) {
	if (_state == ERROR || _state == COMPLETE) {
		return false;
	}
	
	_buffer += chunk;
	
	while (_state != COMPLETE && _state != ERROR) {
		if (_state == REQUEST_LINE) {
			size_t pos = _buffer.find("\r\n");
			if (pos == std::string::npos)
				break;
			
			if (!parseRequestLine(_buffer.substr(0, pos))) {
				_state = ERROR;
				return false;
			}
			
			_buffer.erase(0, pos + 2);
			_state = HEADERS;
		}
		else if (_state == HEADERS) {
			if (!parseHeaders()) {
				break;
			}
			
			if (_method != "GET" && _method != "HEAD") {
				_state = BODY;
			} else {
				_state = COMPLETE;
			}
		}
		else if (_state == BODY) {
			if (!parseBody()) {
				break;
			}
			_state = COMPLETE;
		}
	}
	
	return _state == COMPLETE;
}

bool Request::parseRequestLine(const std::string& line) {
	std::istringstream iss(line);
	
	if (!(iss >> _method >> _uri >> _version)) {
		return false;
	}
	
	parseUri();
	
	// Validar que el método sea un token HTTP válido
	// Aceptamos cualquier método HTTP válido, no solo los implementados
	// Los métodos HTTP son tokens que consisten en letras mayúsculas, dígitos y algunos caracteres especiales
	if (_method.empty()) {
		return false;
	}
	// Verificar que el método sea un token válido (letras, dígitos, y algunos caracteres especiales permitidos)
	for (size_t i = 0; i < _method.length(); ++i) {
		char c = _method[i];
		if (!std::isalnum(c) && c != '-' && c != '_' && c != '.') {
			return false; // Método inválido
		}
	}
	
	if (_version != "HTTP/1.0" && _version != "HTTP/1.1") {
		return false;
	}
	
	return true;
}

bool Request::parseHeaders() {
	size_t pos = _buffer.find("\r\n\r\n");
	if (pos == std::string::npos) {
		return false;
	}
	
	std::string headersStr = _buffer.substr(0, pos);
	_buffer.erase(0, pos + 4);
	
	std::istringstream iss(headersStr);
	std::string line;
	
	while (std::getline(iss, line)) {
		if (line.empty() || line == "\r") continue;
		
		if (!line.empty() && line[line.size() - 1] == '\r') {
			line.erase(line.size() - 1);
		}
		
		size_t colon = line.find(':');
		if (colon == std::string::npos) {
			continue;
		}
		
		std::string key = trim(line.substr(0, colon));
		std::string value = trim(line.substr(colon + 1));
		
		_headers[toLowerCase(key)] = value;
	}
	
	extractContentLength();
	return true;
}

bool Request::parseBody() {
	if (_chunked) {
		return false;
	}
	
	if (_contentLength == 0) {
		_body.clear();
		return true;
	}
	
	if (_buffer.size() < _contentLength) {
		return false;
	}
	
	_body = _buffer.substr(0, _contentLength);
	_buffer.erase(0, _contentLength);
	
	return true;
}

void Request::parseUri() {
	size_t queryPos = _uri.find('?');
	if (queryPos != std::string::npos) {
		_path = _uri.substr(0, queryPos);
		_query = _uri.substr(queryPos + 1);
	} else {
		_path = _uri;
		_query.clear();
	}
}

void Request::extractContentLength() {
	std::map<std::string, std::string>::const_iterator it = _headers.find("content-length");
	if (it != _headers.end()) {
		std::istringstream iss(it->second);
		iss >> _contentLength;
	}
	
	std::map<std::string, std::string>::const_iterator it2 = _headers.find("transfer-encoding");
	if (it2 != _headers.end() && toLowerCase(it2->second).find("chunked") != std::string::npos) {
		_chunked = true;
	}
}

const std::string& Request::getMethod() const {
	return _method;
}

const std::string& Request::getUri() const {
	return _uri;
}

const std::string& Request::getPath() const {
	return _path;
}

const std::string& Request::getQuery() const {
	return _query;
}

const std::string& Request::getVersion() const {
	return _version;
}

const std::string& Request::getHeader(const std::string& key) const {
	static const std::string empty;
	std::map<std::string, std::string>::const_iterator it = _headers.find(toLowerCase(key));
	if (it != _headers.end()) {
		return it->second;
	}
	return empty;
}

const std::map<std::string, std::string>& Request::getHeaders() const {
	return _headers;
}

const std::string& Request::getBody() const {
	return _body;
}

RequestState Request::getState() const {
	return _state;
}

size_t Request::getContentLength() const {
	return _contentLength;
}

bool Request::isComplete() const {
	return _state == COMPLETE;
}

bool Request::hasHeader(const std::string& key) const {
	return _headers.find(toLowerCase(key)) != _headers.end();
}

bool Request::isChunked() const {
	return _chunked;
}

bool Request::isMultipart() const {
	std::map<std::string, std::string>::const_iterator it = _headers.find("content-type");
	if (it != _headers.end()) {
		return it->second.find("multipart/form-data") != std::string::npos;
	}
	return false;
}

std::string Request::toLowerCase(const std::string& str) const {
	std::string result = str;
	for (size_t i = 0; i < result.size(); ++i) {
		result[i] = std::tolower(result[i]);
	}
	return result;
}

std::string Request::trim(const std::string& str) const {
	size_t start = str.find_first_not_of(" \t\r\n");
	if (start == std::string::npos) {
		return "";
	}
	size_t end = str.find_last_not_of(" \t\r\n");
	return str.substr(start, end - start + 1);
}
