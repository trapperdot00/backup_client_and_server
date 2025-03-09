#ifndef CLIENT_OPTIONS_H
#define CLIENT_OPTIONS_H

#include "../utils/Option_parser.h"

class Server_options : private Options {
public:
	Server_options(const Options& o) : Options(o) {}

	int port() const;
	std::filesystem::path backup_path() const;
private:
	template <typename T = std::string>
	T lookup(const std::string& key) const {
		auto it = data.find(key);
		if (it == data.cend())
			throw std::runtime_error{"no value for option \"" + key + '"'};
		return static_cast<T>(it->second);
	}
};

#endif
