#include "Path_handler.h"
#include <filesystem>

namespace fs = std::filesystem;

void add_recursively(Path_handler& ph, const fs::path& path) {
	for (fs::recursive_directory_iterator it{path};
			it != fs::recursive_directory_iterator{};
			++it) {
		if (fs::is_regular_file(*it))
			ph.add_file(*it);
	}
}
