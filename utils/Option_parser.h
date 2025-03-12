#ifndef OPTION_PARSER_H
#define OPTION_PARSER_H

#include <unordered_map>
#include <string>
#include <iostream>
#include <filesystem>
#include <utility>
#include <vector>
#include <stdexcept>
#include <algorithm>

// Holds a configuration file's data
struct Options {
	std::unordered_multimap<std::string, std::string> data;
	std::vector<std::string> lookup(const std::string& key) const {
		auto [beg, end] = data.equal_range(key);
		if (beg == end)
			throw std::runtime_error{"no value for option \"" + key + '"'};
		std::vector<std::string> ret;
		std::transform(beg, end,
				std::back_inserter(ret),
				[](const auto& p){ return p.second; }
		);
		return ret;
	}
	std::string lookup_single(const std::string& key) const {
		auto [beg, end] = data.equal_range(key);
		if (beg == end)
			throw std::runtime_error{"no value for option \"" + key + '"'};
		if (std::distance(beg, end) > 1)
			throw std::runtime_error{"multiple values for option \"" + key + '"'};
		return beg->second;
	}
	template <typename T>
	std::vector<T> lookup_as(const std::string& key) const {
		std::vector<std::string> vec = lookup(key);
		std::vector<T> ret;
		std::transform(vec.begin(), vec.end(),
				std::back_inserter(ret), [](const std::string& s){ return T{s}; }
		);
		return ret;
	}
	template <typename T>
	T lookup_single_as(const std::string& key) const {
		return lookup_single(key);
	}
};

template <> std::vector<int> Options::lookup_as<int>(const std::string&) const;
template <> int Options::lookup_single_as<int>(const std::string&) const;


// Check whether line is commented out
bool comment_line(const std::string&);
// Parses non-comment lines
std::pair<std::string, std::string> parse_line(const std::string&);
// Parses the configuration file
Options parse_options(std::istream&);
// Opens an ifstream and delegates work to parse_options(std::istream&)
Options parse_options(const std::filesystem::path&);

#endif
