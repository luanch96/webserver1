/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   LocationConfig.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: joscastr <joscastr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/31 11:20:00 by joscastr          #+#    #+#             */
/*   Updated: 2025/10/29 22:00:00 by joscastr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "LocationConfig.hpp"

LocationConfig::LocationConfig()
	: autoindex(false), clientMaxBodySize(0) {
}

void LocationConfig::setAutoindex(const std::string& value) {
	autoindex = (value == "on");
}

void LocationConfig::addAllowedMethod(const std::string& method) {
	allowedMethods.push_back(method);
}

void LocationConfig::addCgiPass(const std::string& ext, const std::string& path) {
	cgiPass[ext] = path;
}

