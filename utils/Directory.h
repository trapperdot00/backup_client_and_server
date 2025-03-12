#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <utility>
#include <stdexcept>

// Returns a unique sorted list of only directory names
// under a given path, searched recursively
std::vector<std::string> directories(const std::vector<std::filesystem::path>& root);

// Returns a list of indices of each directory in a path
// found in the passed list of directories
std::vector<size_t> tokenize_path(const std::vector<std::string>& dir, const std::filesystem::path& path);

// Write and read file that contains the directories' names
void write_directories_file(const std::filesystem::path& filepath, const std::vector<std::filesystem::path>& tokens);

template <typename T = std::string>
std::vector<T> read_file_rows(const std::filesystem::path& filepath) {
	std::vector<T> vec;
	std::ifstream is{filepath};
	if (!is)
		throw std::runtime_error{"can't open " + filepath.string() + " for reading"};
	for (std::string line; std::getline(is, line); )
		vec.push_back(T{std::move(line)});
	return vec;
}

// Collect unique elements that are common to each sorted vector
template <typename T>
std::vector<T> collect_common_elements(const std::vector<std::vector<T>>& v) {
	if (v.empty())
		return {};
	std::vector<T> vec = v[0];
	for (size_t i = 1; !vec.empty() && i < v.size(); ++i) {
		std::vector<T> temp;
		std::set_union(
			vec.cbegin(), vec.cend(),
			v[i].cbegin(), v[i].cend(),
			std::back_inserter(temp)
		);
		vec = std::move(temp);
	}
	std::sort(vec.begin(), vec.end());
	auto last = std::unique(vec.begin(), vec.end());
	vec.erase(last, vec.end());
	return vec;
}

// Collect only used tokens as strings
std::vector<std::string> collect_used_directories
(const std::vector<std::string>& tokens, const std::vector<std::vector<size_t>>& token_nums);



#endif
