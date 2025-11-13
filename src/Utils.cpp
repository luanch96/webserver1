/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: joserra <joserra@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/13 00:00:00 by joserra           #+#    #+#             */
/*   Updated: 2025/08/13 00:00:00 by joserra          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Utils.hpp"
#include <sstream>
#include <cstdio>
#include <sys/stat.h>
#include <dirent.h>
#include <ctime>
#include <cstring>
#include <cctype>

std::string Utils::urlDecode(const std::string& str) {
	std::string result;
	for (size_t i = 0; i < str.length(); ++i) {
		if (str[i] == '%' && i + 2 < str.length()) {
			int value;
			std::istringstream iss(str.substr(i + 1, 2));
			if (iss >> std::hex >> value) {
				result += static_cast<char>(value);
				i += 2;
			} else {
				result += str[i];
			}
		} else if (str[i] == '+') {
			result += ' ';
		} else {
			result += str[i];
		}
	}
	return result;
}

std::string Utils::getMimeType(const std::string& filePath) {
	size_t dot = filePath.find_last_of('.');
	if (dot == std::string::npos) {
		return "application/octet-stream";
	}
	
	std::string ext = filePath.substr(dot);
	for (size_t i = 0; i < ext.length(); ++i) {
		ext[i] = std::tolower(ext[i]);
	}
	
	if (ext == ".html" || ext == ".htm") return "text/html; charset=utf-8";
	if (ext == ".css") return "text/css; charset=utf-8";
	if (ext == ".js") return "application/javascript; charset=utf-8";
	if (ext == ".json") return "application/json; charset=utf-8";
	if (ext == ".png") return "image/png";
	if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
	if (ext == ".gif") return "image/gif";
	if (ext == ".svg") return "image/svg+xml";
	if (ext == ".txt") return "text/plain; charset=utf-8";
	if (ext == ".pdf") return "application/pdf";
	if (ext == ".xml") return "application/xml; charset=utf-8";
	
	return "application/octet-stream";
}

std::string Utils::generateAutoindex(const std::string& dirPath, const std::string& requestPath) {
	std::ostringstream html;
	html << "<!DOCTYPE html>\n";
	html << "<html>\n<head>\n";
	html << "    <title>Index of " << requestPath << "</title>\n";
	html << "    <meta charset=\"utf-8\">\n";
	html << "    <style>\n";
	html << "        body { font-family: Arial, sans-serif; margin: 40px; }\n";
	html << "        h1 { color: #333; }\n";
	html << "        hr { border: 1px solid #ddd; margin: 20px 0; }\n";
	html << "        pre { font-family: monospace; font-size: 14px; }\n";
	html << "        a { text-decoration: none; color: #0066cc; }\n";
	html << "        a:hover { text-decoration: underline; }\n";
	html << "    </style>\n";
	html << "</head>\n<body>\n";
	html << "<h1>Index of " << requestPath << "</h1>\n<hr>\n<pre>\n";
	
	DIR* dir = opendir(dirPath.c_str());
	if (!dir) {
		html << "</pre><hr></body></html>";
		return html.str();
	}
	
	// Add parent directory link
	if (requestPath != "/") {
		std::string parentPath = requestPath;
		if (parentPath[parentPath.length() - 1] == '/') {
			parentPath = parentPath.substr(0, parentPath.length() - 1);
		}
		size_t lastSlash = parentPath.find_last_of('/');
		if (lastSlash != std::string::npos) {
			parentPath = parentPath.substr(0, lastSlash + 1);
		} else {
			parentPath = "/";
		}
		html << "<a href=\"" << parentPath << "\">../</a>\n";
	}
	
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;
		if (name == ".") continue;
		if (name == ".." && requestPath == "/") continue;
		
		std::string fullPath = dirPath + "/" + name;
		struct stat st;
		if (stat(fullPath.c_str(), &st) == 0) {
			std::string linkPath = requestPath;
			if (linkPath[linkPath.length() - 1] != '/') linkPath += "/";
			linkPath += name;
			
			if (S_ISDIR(st.st_mode)) {
				html << "<a href=\"" << linkPath << "/\">" << name << "/</a>\n";
			} else {
				html << "<a href=\"" << linkPath << "\">" << name << "</a>\n";
			}
		}
	}
	
	closedir(dir);
	html << "</pre>\n<hr>\n</body>\n</html>";
	return html.str();
}

bool Utils::isDirectory(const std::string& path) {
	struct stat st;
	if (stat(path.c_str(), &st) == 0) {
		return S_ISDIR(st.st_mode);
	}
	return false;
}

bool Utils::fileExists(const std::string& path) {
	struct stat st;
	return stat(path.c_str(), &st) == 0;
}

std::string Utils::readFile(const std::string& path) {
	FILE* file = fopen(path.c_str(), "rb");
	if (!file) {
		return "";
	}
	
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	
	std::string content;
	content.resize(size);
	if (size > 0) {
		fread(&content[0], 1, size, file);
	}
	
	fclose(file);
	return content;
}

size_t Utils::parseSize(const std::string& sizeStr) {
	if (sizeStr.empty()) return 0;
	
	size_t value = 0;
	std::istringstream iss(sizeStr);
	iss >> value;
	
	char unit = sizeStr[sizeStr.length() - 1];
	if (unit == 'k' || unit == 'K') {
		value *= 1024;
	} else if (unit == 'm' || unit == 'M') {
		value *= 1024 * 1024;
	} else if (unit == 'g' || unit == 'G') {
		value *= 1024 * 1024 * 1024;
	}
	
	return value;
}

