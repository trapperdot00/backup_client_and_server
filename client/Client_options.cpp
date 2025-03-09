#include "Client_options.h"

namespace fs = std::filesystem;

template <>
int Client_options::lookup<int>(const std::string& key) const {
	return std::stoi(lookup(key));
}

std::string Client_options::server_ip() const {
	return lookup("ServerIP");
}

int Client_options::port() const {
	return lookup<int>("Port");
}

fs::path Client_options::sync_path() const {
	return lookup<fs::path>("SyncPath");
}

