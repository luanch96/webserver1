/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: luis <luis@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/29 22:54:48 by luis              #+#    #+#             */
/*   Updated: 2025/10/29 22:54:48 by luis             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>

class Response {
public:
	Response();
	~Response();
	
	void setStatus(int code, const std::string& message = "");
	void setHeader(const std::string& key, const std::string& value);
	void setBody(const std::string& body);
	void setBody(const char* data, size_t size);
	
	int getStatus() const;
	const std::string& getStatusMessage() const;
	const std::string& getHeader(const std::string& key) const;
	bool hasHeader(const std::string& key) const;
	const std::string& getBody() const;
	
	std::string buildResponse() const;
	size_t getBodySize() const;
	
	void clear();

private:
	int _statusCode;
	std::string _statusMessage;
	std::map<std::string, std::string> _headers;
	std::string _body;
	
	std::string getDateHeader() const;
	std::string getStatusMessageForCode(int code) const;
	std::string toString(size_t value) const;
};

#endif

