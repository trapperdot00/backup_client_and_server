#include "Option_parser.h"
#include "String_operations.h"
#include "Stream_operations.h"

#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

bool comment_line(const std::string& line) {
	std::istringstream is{line};
	char ch = 0;
	is >> ch;
	return ch == '#';
}

std::pair<std::string, std::string> parse_line(const std::string& line) {
	if (line.empty())
		throw std::runtime_error{"empty line"};
	std::istringstream is{line};
	skip_whitespace(is);
	std::string option;
	for (char ch = 0; is.get(ch) && ch != '='; )
		option += ch;
	if (!is)
		throw std::runtime_error{"option \"" + option + "\" has no value"};
	option = rstrip(option);
	if (option.empty())
		throw std::runtime_error{"empty option"};
	std::string value;
	for (char ch = 0; is.get(ch); )
		value += ch;
	value = strip(value);
	if (value.empty())
		throw std::runtime_error{"option \"" + option + "\" has no value"};

	return std::make_pair(option, value);
}

Options parse_options(std::istream& is) {
	Options options;
	size_t line_no = 0;
	for (std::string line; std::getline(is, line); ++line_no) try {
		if (strip(line).empty() || comment_line(line))
			continue;
		std::pair<std::string, std::string> parsed = parse_line(line);
		if (!options.data.insert(parse_line(line)).second)
			throw std::runtime_error{"multiple values for option " + parsed.first};
	} catch (std::runtime_error& e) {	// Attach context to error
		throw std::runtime_error{
			"line " + std::to_string(line_no+1) + ": " + e.what()
		};
	}
	return options;
}

Options parse_options(const fs::path& path) {
	std::ifstream is{path};
	if (!is)
		throw std::runtime_error{
			"can't open config file ("
			+ path.string()
			+ ") for reading"
		};
	return parse_options(is);
}
