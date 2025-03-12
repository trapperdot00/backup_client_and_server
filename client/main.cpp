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
#include <algorithm>
#include <sstream>
#include <boost/crc.hpp>

#include "Client_options.h"
#include "../utils/Path_handler.h"

namespace fs = std::filesystem;

std::vector<fs::path>
recursive_directory_search(const fs::path& p) {
	std::vector<fs::path> entries;
	for (fs::recursive_directory_iterator it{p};
		it != fs::recursive_directory_iterator{};
		++it) {
		// Only return files, not directories, as directories don't
		// have checksum values, and it would mess with the parsing
		if (!fs::is_directory(*it) && fs::is_regular_file(*it))
			entries.push_back(*it);
	}
	return entries;
}

std::vector<fs::path> get_directory_entries
(const fs::path& p) {
	return recursive_directory_search(p);
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

std::unordered_map<fs::path, uint32_t> checksums_for_directory_entries
(const std::vector<fs::path>& vec) {
	std::unordered_map<fs::path, uint32_t> ret;
	for (const fs::path& path : vec) {
		std::vector<fs::path> paths = get_directory_entries(path);
		for (fs::path& p : paths) {
			// VERBOSE
			std::cout << "Calculating " << p << '\n';
			
			uint32_t checksum = get_crc32_from_file(p);
			ret.insert({std::move(p), checksum});
		}
	}
	return ret;
}

std::string format(const std::unordered_map<fs::path, uint32_t>& entries, const fs::path& root) {
	std::ostringstream os;
	for (const auto& e : entries) {
		os << fs::relative(e.first, root)
			<< '\t'
			<< e.second
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

		// VERBOSE
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

int main(/*int argc, char* argv[]*/) try {
	// Configuration file path
	const fs::path config_path = "./config.txt";
	const Client_options options{parse_options(config_path)};
	Path_handler phandler(options.sync_path(), options.directory());
	for (const fs::path& p : options.sync_path())
		add_recursively(phandler, p);
	std::cout << "0000\n" 
		<< phandler.translation_table()
		<< "0000\n";
	for (size_t i = 0; i < phandler.size(); ++i)
		std::cout << phandler.entry(i) << '\t' << "cksum\n";
	std::cout << "0000\n";
/*
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		throw std::runtime_error{"socket error"};

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(options.port());
	addr.sin_addr.s_addr = inet_addr(options.server_ip().c_str());
	if (connect(fd, reinterpret_cast<sockaddr*>(&addr),
			sizeof(addr)) == -1) {
		int err = errno;
		throw std::runtime_error{
			"failed connect() " + std::to_string(err)
		};
	}
*/
//	std::cout << "Calculating checksums..." << std::endl;
//	std::unordered_map<fs::path, uint32_t> entries =
//		checksums_for_directory_entries(options.sync_path());

//	std::cout << "Sending checksums..." << std::endl;
	// TODO: multiple syncpath support
//	std::string send_text = format(entries, options.sync_path()[0]);
//	if (send_text.empty()) {
//		std::cout << "No files to backup!\n";
//		return 0;
//	}

//	std::cout << send_text << '\n';
/*	send_message(fd, send_text);
	
	std::string received = receive_message(fd);
	std::set<fs::path> outdated = parse_message(received);
	if (outdated.empty()) {
		std::cout << "Local files up to date with backup.\n";
	} else {
		std::cout << outdated.size() << " file(s) to backup.\n";
		send_files(fd, outdated, options.sync_path());
	}
	
	shutdown(fd, SHUT_WR);
	close(fd);
	std::cout << "Backup complete!\n";
*/
} catch (const std::exception& e) {
	std::cerr << "error: " << e.what() << '\n';
} catch (...) {
	std::cerr << "unknown error!\n";
}
