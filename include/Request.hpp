/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: luis <luis@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/29 22:54:48 by luis              #+#    #+#             */
/*   Updated: 2025/10/29 22:54:48 by luis             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>
#include <vector>

enum RequestState {
	REQUEST_LINE,
	HEADERS,
	BODY,
	COMPLETE,
	ERROR
};

class Request {
public:
	Request();
	~Request();
	
	// Parsing
	bool parseChunk(const std::string& chunk);
	void reset();
	
	// Getters
	const std::string& getMethod() const;
	const std::string& getUri() const;
	const std::string& getPath() const;
	const std::string& getQuery() const;
	const std::string& getVersion() const;
	const std::string& getHeader(const std::string& key) const;
	const std::map<std::string, std::string>& getHeaders() const;
	const std::string& getBody() const;
	RequestState getState() const;
	size_t getContentLength() const;
	bool isComplete() const;
	bool hasHeader(const std::string& key) const;
	
	// Content type helpers
	bool isChunked() const;
	bool isMultipart() const;

private:
	RequestState _state;
	std::string _method;
	std::string _uri;
	std::string _path;
	std::string _query;
	std::string _version;
	std::map<std::string, std::string> _headers;
	std::string _body;
	std::string _buffer;
	size_t _contentLength;
	bool _chunked;
	
	// Parsing helpers
	bool parseRequestLine(const std::string& line);
	bool parseHeaders();
	bool parseBody();
	void parseUri();
	std::string toLowerCase(const std::string& str) const;
	std::string trim(const std::string& str) const;
	void extractContentLength();
};

#endif

