/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: joserra <joserra@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/29 22:00:00 by joserra           #+#    #+#             */
/*   Updated: 2025/10/29 22:00:00 by joserra          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

namespace Utils {
	std::string urlDecode(const std::string& str);
	std::string getMimeType(const std::string& filePath);
	std::string generateAutoindex(const std::string& dirPath, const std::string& requestPath);
	bool isDirectory(const std::string& path);
	bool fileExists(const std::string& path);
	std::string readFile(const std::string& path);
	size_t parseSize(const std::string& sizeStr);
}

#endif

