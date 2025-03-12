#include <filesystem>
#include <iostream>
#include <fstream>

#include "Directory.h"

namespace fs = std::filesystem;

int main() {
	const fs::path path = fs::current_path().root_path();
	const fs::path directories_file = "./directories.txt";

	const fs::path rootpath_file = "./rootpath.txt";
	std::vector<fs::path> paths;
	std::ifstream is{rootpath_file};
	for (std::string line; std::getline(is, line); )
		paths.push_back(fs::path{line});
	if (!fs::exists(directories_file))
		write_directories_file(directories_file, paths);
	const auto directories = read_file_rows(directories_file);

	const fs::path paths_file = "./paths.txt";
	const std::vector<std::string> files = read_file_rows(paths_file);
	std::vector<std::vector<size_t>> tokens;
	for (auto file : files) {
		std::vector<size_t> curr_tokens = tokenize_path(directories, file);
		std::cout << file << '\t' << curr_tokens.size() << '\n';
		tokens.push_back(std::move(curr_tokens));
	}
	std::vector<std::string> used_directories = collect_used_directories(directories, tokens);
	size_t i = 0;
	for (const std::string& s : used_directories)
		std::cout << i++ << '\t' << s << '\n';
	for (const std::string& file : files) {
		fs::path path{file};
		std::vector<size_t> curr_tokens = tokenize_path(used_directories, path);
		for (size_t i : curr_tokens)
			std::cout << i << ' ';
		std::cout << path.filename() << '\n';
	}
}
