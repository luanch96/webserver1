/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: luis <luis@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/29 22:54:48 by luis              #+#    #+#             */
/*   Updated: 2025/10/29 22:54:48 by luis             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"
#include <sstream>
#include <ctime>
#include <iomanip>
#include <cstring>

Response::Response() : _statusCode(200), _statusMessage("OK") {
	setHeader("Server", "webserv/1.0");
	setHeader("Date", getDateHeader());
}

Response::~Response() {}

void Response::setStatus(int code, const std::string& message) {
	_statusCode = code;
	if (message.empty()) {
		_statusMessage = getStatusMessageForCode(code);
	} else {
		_statusMessage = message;
	}
}

void Response::setHeader(const std::string& key, const std::string& value) {
	_headers[key] = value;
}

void Response::setBody(const std::string& body) {
	_body = body;
	setHeader("Content-Length", toString(_body.size()));
}

void Response::setBody(const char* data, size_t size) {
	_body.assign(data, size);
	setHeader("Content-Length", toString(_body.size()));
}

std::string Response::toString(size_t value) const {
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

std::string Response::getStatusMessageForCode(int code) const {
	switch (code) {
		case 200: return "OK";
		case 201: return "Created";
		case 204: return "No Content";
		case 301: return "Moved Permanently";
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 408: return "Request Timeout";
		case 413: return "Payload Too Large";
		case 414: return "URI Too Long";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 502: return "Bad Gateway";
		case 503: return "Service Unavailable";
		case 504: return "Gateway Timeout";
		default: return "Unknown";
	}
}

int Response::getStatus() const {
	return _statusCode;
}

const std::string& Response::getStatusMessage() const {
	return _statusMessage;
}

const std::string& Response::getHeader(const std::string& key) const {
	static const std::string empty;
	std::map<std::string, std::string>::const_iterator it = _headers.find(key);
	if (it != _headers.end()) {
		return it->second;
	}
	return empty;
}

bool Response::hasHeader(const std::string& key) const {
	return _headers.find(key) != _headers.end();
}

const std::string& Response::getBody() const {
	return _body;
}

size_t Response::getBodySize() const {
	return _body.size();
}

std::string Response::buildResponse() const {
	std::ostringstream oss;
	
	// Status line
	oss << "HTTP/1.1 " << _statusCode << " " << _statusMessage << "\r\n";
	
	// Headers
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
		 it != _headers.end(); ++it) {
		oss << it->first << ": " << it->second << "\r\n";
	}
	
	// Empty line
	oss << "\r\n";
	
	// Body
	oss << _body;
	
	return oss.str();
}

void Response::clear() {
	_statusCode = 200;
	_statusMessage = "OK";
	_headers.clear();
	_body.clear();
	setHeader("Server", "webserv/1.0");
	setHeader("Date", getDateHeader());
}

std::string Response::getDateHeader() const {
	char buffer[128];
	time_t now = time(NULL);
	struct tm* gmt = gmtime(&now);
	strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);
	return std::string(buffer);
}

