#ifndef PATH_HANDLER_H
#define PATH_HANDLER_H

#include "Directory.h"

#include <sstream>
#include <iostream>
#include <unordered_map>

class Path_handler {
public:
	Path_handler(const std::filesystem::path& rootpath_file,
			const std::filesystem::path& directories)
	: paths_path{read_file_rows<std::filesystem::path>(rootpath_file)},
		directories_path{directories},
		directory_names{handle_directory_file()} {}

	Path_handler(const std::vector<std::filesystem::path>& roots,
			const std::filesystem::path& directories)
	: paths_path{roots},
		directories_path{directories},
		directory_names{handle_directory_file()} {}


	void add_file(const std::filesystem::path file) {
		auto normalized_path = file.lexically_normal();
		if (!path_inside_directories(normalized_path))
			throw std::runtime_error{"path \"" + file.string() + "\" not under given directories"};
		try {
			update_indices(normalized_path);
			update_used_directories();
		} catch (const std::exception& e) {
			handle_outdated_directory_file();
		}
		files.push_back(normalized_path);
	}
	void add_files(const std::vector<std::filesystem::path>& vec) {
		for (const auto& file : vec)
			add_file(file);
	}
	std::string translation_table() const {
		std::ostringstream os;
		for (size_t i = 0; i < used_directories.size(); ++i)
			os << std::hex << i << '\t' << used_directories.at(i) << '\n';
		return os.str();
	}
	std::string compressed_entry(size_t n) const {
		std::ostringstream os;
		const std::filesystem::path& file = files.at(n);
		for (size_t token : tokenize_path(used_directories, file))
			os << std::hex << token << ' ';
		os << file.filename();
		return os.str();
	}
	std::string compressed_entries() const {
		std::ostringstream os;
		for (size_t i = 0; i < files.size(); ++i)
			os << compressed_entry(i) << '\n';
		return os.str();
	}
	std::filesystem::path entry(size_t n) const {
		return files.at(n);
	}
	size_t size() const { return files.size(); }
private:
	std::vector<std::string> handle_directory_file() const {
		if (!std::filesystem::exists(directories_path)) {
			return generate_directory_file();
		}
		return read_file_rows(directories_path);
	}
	std::vector<std::string> generate_directory_file() const {
		write_directories_file(directories_path, paths_path);
		return read_file_rows(directories_path);
	}
	void migrate_directories(const std::vector<std::string>& new_dirs) {
		std::unordered_map<size_t, size_t> old_to_new;
		for (size_t i = 0; i < directory_names.size(); ++i) {
			const auto& directory = directory_names.at(i);
			auto it = std::lower_bound(new_dirs.begin(), new_dirs.end(), directory);
			if (it == new_dirs.end() || *it != directory)
				throw std::runtime_error{"Directory \"" + directory + "\" not found in new directory-file"};
			old_to_new[i] = std::distance(new_dirs.begin(), it);
		}
		directory_names = new_dirs;
		for (size_t& i : indices)
			i = old_to_new.at(i);
		used_directories.clear();
		for (size_t i : indices)
			used_directories.push_back(directory_names.at(i));
	}
	bool path_inside_directories(const std::filesystem::path& file) const {
		for (size_t i = 0; i < paths_path.size(); ++i) {
			const auto& path = paths_path.at(i);
			auto [beg, end] = std::mismatch(
					path.begin(), path.end(),
					file.begin(), file.end()
			);
			if (beg == path.end())
				return true;
		}
		return false;
	}
	void update_indices(const std::filesystem::path& file) {
		std::vector<size_t> current_indices = tokenize_path(directory_names, file);
		indices = collect_common_elements(std::vector<std::vector<size_t>>{current_indices, indices});
	}
	void update_used_directories() {
		std::vector<std::string> vec;
		for (size_t i : indices)
			vec.push_back(directory_names.at(i));
		used_directories = std::move(vec);
	}
	void handle_outdated_directory_file() {
		std::vector<std::string> new_dirs = generate_directory_file();
		migrate_directories(new_dirs);
	}

	// Recursive directory search starts from root
	std::vector<std::filesystem::path> paths_path;

	// File that holds the scanned directories' names
	std::filesystem::path directories_path;

	// Could be a separate class
	std::vector<std::string> directory_names;

	std::vector<size_t> indices;
	std::vector<std::string> used_directories;

	std::vector<std::filesystem::path> files;
};

void add_recursively(Path_handler&, const std::filesystem::path&);

#endif
