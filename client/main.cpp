#include <limits>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <unistd.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <sstream>
#include <boost/crc.hpp>

namespace fs = std::filesystem;

std::vector<fs::path>
recursive_directory_search(const fs::path& p, size_t depth) {
	std::vector<fs::path> entries;
	for (fs::directory_iterator it{p};
		it != fs::directory_iterator{};
		++it) {
		// Only return files, not directories, as directories don't
		// have checksum values, and it would mess with the parsing
		if (!fs::is_directory(*it))
			entries.push_back(*it);
		if (depth > 1 && fs::is_directory(*it)) {
			std::vector<fs::path> dir_entries =
				recursive_directory_search(*it, depth - 1);
			std::move(
				dir_entries.begin(),
				dir_entries.end(),
				std::back_inserter(entries)
			);
		}
	}
	return entries;
}

std::vector<fs::path> get_directory_entries
(const fs::path& p, size_t depth) {
	if (!depth)
		throw std::runtime_error{"depth of 0"};
	return recursive_directory_search(p, depth);
}

uint32_t get_crc32(const std::string& s) {
	boost::crc_32_type value;
	value.process_bytes(s.data(), s.size());
	return value.checksum();
}

uint32_t get_crc32_from_file(const fs::path& path) {
	constexpr size_t bufsize = 4096;
	std::ifstream is{path, std::ios_base::binary};
	if (!is)
		throw std::runtime_error{
			"can't open "
			+ path.string()
			+ " for reading"
		};
	boost::crc_32_type value;
	char buffer[bufsize];
	while (is.read(buffer, bufsize))
		value.process_bytes(buffer, is.gcount());
	if (is.gcount() > 0)
		value.process_bytes(buffer, is.gcount());
	return value.checksum();
}

void send_message(int fd, const std::string& msg) {
	send(fd, msg.c_str(), msg.size(), MSG_CONFIRM);
	send(fd, "\0", 1, MSG_CONFIRM);	// END TRANSMISSION
}

struct Entry {
	fs::path path;
	uint32_t checksum;
};

std::vector<Entry> checksums_for_directory_entries
(const fs::path& p, size_t depth) {
	std::vector<Entry> ret;
	std::vector<fs::path> paths = get_directory_entries(p, depth);
	for (fs::path& p : paths) {
		uint32_t checksum = get_crc32_from_file(p);
		ret.push_back(Entry{std::move(p), checksum});
	}
	return ret;
}

std::string format(const std::vector<Entry>& vec, const fs::path& root) {
	std::ostringstream os;
	for (const Entry& e : vec) {
		os << fs::relative(e.path, root)
			<< '\t'
			<< e.checksum
			<< '\n';
	}
	return os.str();
}

std::set<fs::path> parse_message(const std::string& msg) {
	std::istringstream is{msg};
	std::set<fs::path> paths;
	for (std::string s; std::getline(is, s); ) {
		paths.insert(fs::path{s});
	}
	return paths;
}

std::string receive_message(int fd) {
	std::string message;
	constexpr size_t bufsize = 1024;
	char temp_buf[bufsize]{};
	int status = 0;
	while ((status = read(fd, temp_buf, bufsize)) > 0) {
		message.append(temp_buf, status);
		if (message.find('\0') != std::string::npos) {
			message.erase(message.size()-1);
			break;
		}
	}
	return message;
}

void send_files
(int fd, const std::set<fs::path>& paths, const fs::path& sync_path) {
	for (const fs::path& path : paths) {
		fs::path localpath = sync_path / path;
		std::ifstream is{localpath, std::ios_base::binary};
		if (!is) {
			std::cerr << path << " doesn't exist!\n";
			continue;
		}
		size_t file_size = fs::file_size(localpath);
		std::ostringstream metadata;
		metadata << path << ' ' << file_size << '\n';
		std::cout << "Sending " << path << " (" << file_size << " bytes)\n";
		send(fd, metadata.str().c_str(), metadata.str().size(), MSG_CONFIRM);
		constexpr size_t bufsize = 1024;
		char buf[bufsize]{};
		for (size_t i = 0; i < file_size; i += bufsize) {
			is.read(buf, bufsize);
			send(fd, buf, is.gcount(), MSG_CONFIRM);
		}
	}
}

struct Options {
	enum setting {
		ServerIP, Port, SyncPath
	};
	Options(const std::map<setting, std::string>& data)
		: server_ip{data.at(ServerIP)},
		port{std::stoi(data.at(Port))},
		sync_path{data.at(SyncPath)}
	{}

	std::string server_ip;
	int port;
	fs::path sync_path;	// Directory to sync with server
};
Options::setting get_setting(const std::string& s) {
	static const std::map<std::string, Options::setting> trans = {
		{"ServerIP", Options::ServerIP},
		{"Port", Options::Port},
		{"SyncPath", Options::SyncPath}
	};
	std::map<std::string, Options::setting>::const_iterator it =
		trans.find(s);
	if (it == trans.cend())
		throw std::runtime_error{"invalid setting: \"" + s + '"'};
	return it->second;
}

void rstrip(std::string& s) {
	size_t pos = s.find_last_not_of(" \t\n");
	s = s.substr(0, pos + 1);
}

void lstrip(std::string& s) {
	size_t pos = s.find_first_not_of(" \t\n");
	s = s.substr(pos);
}

Options parse_config_file(const fs::path& filepath) {
	std::ifstream is{filepath};
	if (!is)
		throw std::runtime_error{"can't open config file "
			+ filepath.string()
			+ " for reading"
		};
	std::map<Options::setting, std::string> values;
	while (is) {
		std::string setting;
		std::string value;
		char ch = 0;
		// Skip whitespace from the front
		while (is.get(ch) && std::isspace(ch))
			;
		if (!is)	// No more lines to read
			break;
		// Comment line starts with a # after any whitespace chars
		if (ch == '#') {
			// Skip to the next line
			while (is.get(ch) && ch != '\n')
				;
			continue;
		}
		is.unget();	// Put back if not #
		// Read setting, drop =
		while (is.get(ch) && ch != '=') {
			setting += ch;
		}
		rstrip(setting);
		if (!is)
			throw std::runtime_error{"setting missing in config file"};
		// Read value
		while (is.get(ch) && ch != '\n') {
			value += ch;
		}
		lstrip(value);
		if (!is || value.empty())
			throw std::runtime_error{
				"value missing for " + setting + " in config file"
			};
		std::cout << setting << " : " << value << '\n';
		Options::setting setting_e = get_setting(setting);
		// Check for insert success
		bool result = values.insert(
			std::make_pair(setting_e, value)
			).second;
		if (!result)
			throw std::runtime_error{
				"multiple values for setting " + setting
			};
	}
	Options options{values};
	return options;
}

int main(int argc, char* argv[]) try {
	// Configuration file path
	const fs::path config_path = "./config.txt";
	const Options options = parse_config_file(config_path);

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		throw std::runtime_error{"socket error"};
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(options.port);
	addr.sin_addr.s_addr = inet_addr(options.server_ip.c_str());

	size_t depth = (argc > 1 ? std::stoull(argv[1])
			: std::numeric_limits<size_t>::max());
	std::vector<Entry> entries =
		checksums_for_directory_entries(options.sync_path, depth);
	std::string send_text = format(entries, options.sync_path);
	if (send_text.empty()) {
		std::cout << "No files to backup!\n";
		return 0;
	}
	if (connect(fd, reinterpret_cast<sockaddr*>(&addr),
			sizeof(addr)) == -1) {
		int err = errno;
		throw std::runtime_error{
			"failed connect() " + std::to_string(err)
		};
	}

	send_message(fd, send_text);
	
	std::string received = receive_message(fd);
	std::set<fs::path> outdated = parse_message(received);
	if (outdated.empty()) {
		std::cout << "Local files up to date with backup.\n";
	} else {
		std::cout << outdated.size() << " file(s) to backup.\n";
		send_files(fd, outdated, options.sync_path);
	}

	shutdown(fd, SHUT_WR);
	close(fd);
	std::cout << "Backup complete!\n";
} catch (const std::exception& e) {
	std::cerr << "error: " << e.what() << '\n';
} catch (...) {
	std::cerr << "unknown error!\n";
}
