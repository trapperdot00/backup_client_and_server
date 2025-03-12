#include "Directory.h"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <fstream>

namespace fs = std::filesystem;

std::vector<std::string> directories(const std::vector<fs::path>& paths) {
	// Holds directory names
	std::vector<std::string> dir_vec;
	for (const fs::path& path : paths) {
		for (const fs::directory_entry& e : fs::recursive_directory_iterator{
				path, fs::directory_options::skip_permission_denied
			}) try {
			if (fs::is_directory(e)) {
				// Insert name of current directory
				dir_vec.push_back(e.path().filename());
			}
		} catch (...) {
			// Skip unaccessible entries
		}
	}
	if (dir_vec.size()) {
		// Sort and remove non-unique elements
		std::sort(dir_vec.begin(), dir_vec.end());
		auto last = std::unique(dir_vec.begin(), dir_vec.end());
		dir_vec.erase(last, dir_vec.end());
	}
	return dir_vec;
}

std::vector<size_t> tokenize_path(const std::vector<std::string>& dir_vec, const fs::path& p) {
	std::vector<size_t> tokens;
	for (fs::path path{p.parent_path()}; path != path.root_path(); path = path.parent_path()) {
		const std::string dir = path.filename();
		auto it = std::lower_bound(dir_vec.cbegin(), dir_vec.cend(), dir);
		if (it != dir_vec.cend()) {
			size_t token = std::distance(dir_vec.cbegin(), it);
			tokens.push_back(token);
		} else {
			throw std::runtime_error{
				"Directory '" + dir + "' has no associated token"
			};
		}
	}
	std::reverse(tokens.begin(), tokens.end());
	return tokens;
}

void write_directories_file(const fs::path& file, const std::vector<fs::path>& paths) {
	std::ofstream os{file};
	if (!os)
		throw std::runtime_error{"can't open " + file.string() + " for writing"};
	const auto dir_vec = directories(paths);
	for (const auto& dir : dir_vec)
		os << dir << '\n';
}

std::vector<std::string> read_file_rows(const fs::path& file) {
	std::vector<std::string> tokens;
	std::ifstream is{file};
	if (!is)
		throw std::runtime_error{"can't open " + file.string() + " for reading"};
	for (std::string line; std::getline(is, line); )
		tokens.push_back(line);
	return tokens;
}

std::vector<std::string> collect_used_directories
(const std::vector<std::string>& tokens, const std::vector<std::vector<size_t>>& token_nums) {
	std::vector<std::string> vec;
	std::vector<size_t> indices = collect_common_elements(token_nums);
	for (size_t i : indices)
		vec.push_back(tokens[i]);
	return vec;
}
