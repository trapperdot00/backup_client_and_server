#include "Server_options.h"

namespace fs = std::filesystem;

template <>
int Server_options::lookup<int>(const std::string& key) const {
	return std::stoi(lookup(key));
}

int Server_options::port() const {
	return lookup<int>("Port");
}

fs::path Server_options::backup_path() const {
	return lookup<fs::path>("BackupPath");
}

