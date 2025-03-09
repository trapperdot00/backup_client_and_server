#include <ostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cstring>
#include <string>
#include <stdexcept>
#include <cerrno>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <utility>
#include <fstream>
#include <algorithm>
#include <set>

#include "Server_options.h"

namespace fs = std::filesystem;

struct Entry {
	fs::path path;
	uint32_t checksum;
};

struct Compare {
	bool operator()(const Entry& a, const Entry& b) const {
		return a.path < b.path;
	}
};

std::set<Entry, Compare> parse(std::istream& is) {
	std::set<Entry, Compare> entries;
	fs::path path;
	uint32_t checksum = 0;
	while (is >> path >> checksum) {
		entries.insert(Entry{std::move(path), checksum});
	}
	return entries;
}

std::set<Entry, Compare> parse(const std::string& s) {
	std::istringstream is{s};
	return parse(is);
}

std::set<Entry, Compare> parse_file(const fs::path& filepath) {
	std::ifstream is{filepath};
	return parse(is);
}

std::set<fs::path> get_outdated
(const std::set<Entry, Compare>& rec, const std::set<Entry, Compare>& exi) {
	std::set<fs::path> outdated;
	for (const Entry& e : rec) {
		std::set<Entry>::const_iterator it = exi.find(e);
		if (it == exi.cend() || e.checksum != it->checksum)
			outdated.insert(e.path);
	}
	return outdated;
}

struct FileHeader {
	fs::path path;
	size_t byte_count;
};

FileHeader parse_header(const std::string& s) {
	std::istringstream is{s};
	std::string path_s;
	size_t byte_count = 0;
	if (!(is >> std::quoted(path_s) >> byte_count))
		throw std::runtime_error{"invalid file header"};
	return FileHeader{fs::path{path_s}, byte_count};
}

bool read_file_header(int fd, FileHeader& h) {
	std::string header;	// "path" bytecount
	char ch = 0;
	ssize_t status = 0;
	while ((status = read(fd, &ch, 1)) > 0 && ch != '\n') {
		header += ch;
	}
	if (status > 0) {
		h = parse_header(header);
		return true;
	}
	return false;
}

// Wrapper to RAII delete
struct HeapAlloc {
	HeapAlloc(size_t n) : p(new char[n]{}) {}
	~HeapAlloc() {
		delete[] p;
	}
	char* p = nullptr;
};

void create_files
(int fd, std::size_t bufsize, const fs::path& backup_path) {
	FileHeader header;
	while (read_file_header(fd, header)) {
		const fs::path path = backup_path / header.path;
		/*
		if (fs::exists(path))
			std::clog << "Overwritten " << path.string() << '\n';
		else {
			fs::create_directories(path.parent_path());
			std::clog << "Created " << path.string() << '\n';
		}
		*/
		std::ofstream os{path, std::ios_base::binary};
		if (!os)
			throw std::runtime_error{
				"can't open "
				+ path.string() 
				+ " for writing"
			};
		HeapAlloc buf{bufsize};
		size_t remaining = header.byte_count;
		size_t read_amount = std::min(bufsize, remaining);
		ssize_t bytes_read = 0;
		while (remaining > 0 && (bytes_read = read(fd, buf.p, read_amount)) > 0) {
			os.write(buf.p, bytes_read);
			remaining -= bytes_read;
			read_amount = std::min(bufsize, remaining);
		}
	}
}

// TODO clean this mess
int main(int argc, char* argv[]) {
	constexpr size_t default_bufsize = 1024;
	const size_t bufsize = (argc < 2) ? default_bufsize : std::stoull(argv[1]);	// Read in chunks
	const fs::path config_path = "./config.txt";
	const fs::path checksums_path = "./checksums.txt";
	const Server_options options = parse_options(config_path);
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		throw std::runtime_error{"failed socket()"};
	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(options.port());
	addr.sin_addr.s_addr = INADDR_ANY;	// Accept connections from any IP address

	if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
		int err = errno;
		throw std::runtime_error{"failed bind() " + std::to_string(err)};
	}
	if (listen(fd, 1) == -1) {
		int err = errno;
		throw std::runtime_error{"failed listen() " + std::to_string(err)};
	}
	socklen_t size = sizeof(addr);
	for (;;) {
		int clientfd = accept(fd, reinterpret_cast<sockaddr*>(&addr), &size);
		if (clientfd == -1) {
			std::cerr << "accept() failed\n";
			continue;
		}
		std::cout << "--Connected from " << inet_ntoa(addr.sin_addr) << "--\n";
		HeapAlloc buf{bufsize};	// Store currently received characters
		int status = 0;		// How many bytes we have read
		std::string fulldata;	// Store all of the received characters
		while ((status = read(clientfd, buf.p, bufsize)) > 0) {
			fulldata.append(buf.p, status);
			if (fulldata.find('\0') != std::string::npos)
				break;
		}
		std::set<Entry, Compare> received_entries = parse(fulldata);
		std::cout << "Received: " << received_entries.size() << " file(s)\n";
		std::set<Entry, Compare> existing_entries = parse_file(checksums_path);
		std::cout << "Existing file(s): " << existing_entries.size() << '\n';
		std::set<fs::path> outdated =
			get_outdated(received_entries, existing_entries);	// Files to be sent by the client
		if (outdated.empty())
			std::cout << "Backup up to date\n";
		else
			std::cout << outdated.size() << " file(s) to update\n";
		std::ofstream os{checksums_path};
		for (const Entry& e : received_entries)
			os << e.path << '\t' << e.checksum << '\n';
		for (const fs::path& p : outdated) {
			std::string msg = p.string() + '\n';
			send(clientfd, msg.c_str(), msg.size(), MSG_CONFIRM);
		}
		send(clientfd, "\0", 1, MSG_CONFIRM);	// Terminator
		
		create_files(clientfd, bufsize, options.backup_path());

		close(clientfd);
		std::cout << "--Disconnected from " << inet_ntoa(addr.sin_addr) << "--\n";
	}
}
