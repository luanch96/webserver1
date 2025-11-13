/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   LocationConfig.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: joscastr <joscastr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/31 11:19:28 by joscastr          #+#    #+#             */
/*   Updated: 2025/08/02 00:01:36 by joscastr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include <string>
#include <vector>
#include <map>

class LocationConfig {

	public:
		std::string path;
		std::string root;
		bool autoindex;
		std::vector<std::string> allowedMethods;
		std::map<std::string, std::string> cgiPass; // ext → path
		std::string redirect;
		size_t clientMaxBodySize;
	
		LocationConfig();
		
		void setAutoindex(const std::string& value);
		void addAllowedMethod(const std::string& method);
		void addCgiPass(const std::string& ext, const std::string& path);
};
#endif