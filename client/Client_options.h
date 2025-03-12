#ifndef CLIENT_OPTIONS_H
#define CLIENT_OPTIONS_H

#include "../utils/Option_parser.h"

struct Client_options : private Options {
public:
	Client_options(const Options& o) : Options(o) {}

	std::string server_ip() const;
	int port() const;
	std::vector<std::filesystem::path> sync_path() const;
	std::filesystem::path directory() const;
};

#endif
