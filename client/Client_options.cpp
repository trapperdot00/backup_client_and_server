#include "Client_options.h"

namespace fs = std::filesystem;

std::string Client_options::server_ip() const {
	return lookup_single("ServerIP");
}

int Client_options::port() const {
	return lookup_single_as<int>("Port");
}

std::vector<fs::path> Client_options::sync_path() const {
	return lookup_as<fs::path>("SyncPath");
}

fs::path Client_options::directory() const {
	return lookup_single_as<fs::path>("DirectoryFile");
}
