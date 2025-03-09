#ifndef OPTION_PARSER_H
#define OPTION_PARSER_H

#include <map>
#include <string>
#include <iostream>
#include <filesystem>
#include <utility>

// Holds a configuration file's data
struct Options {
	std::map<std::string, std::string> data;
};

// Check whether line is commented out
bool comment_line(const std::string&);
// Parses non-comment lines
std::pair<std::string, std::string> parse_line(const std::string&);
// Parses the configuration file
Options parse_options(std::istream&);
// Opens an ifstream and delegates work to parse_options(std::istream&)
Options parse_options(const std::filesystem::path&);

#endif
