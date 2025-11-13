/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: joscastr <joscastr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/01 15:33:11 by joscastr          #+#    #+#             */
/*   Updated: 2025/08/01 22:23:49 by joscastr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "ServerConfig.hpp"
#include <string>
#include <vector>

class ConfigParser{
	public:
		ConfigParser(const std::string& filepath);
		
		void parse();
		const std::vector<ServerConfig>& getServers() const;

	private:
		std::string _filepath;
		std::string _fileContent;
		std::vector<ServerConfig> _servers;

		void readFile();
		void removeComments();
		void splitServerBlocks();
};


#endif