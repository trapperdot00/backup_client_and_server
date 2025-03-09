#include "Strip_impl.h"

Strip_impl::Strip_impl() : s{" \f\n\r\t\v"} {}

std::string Strip_impl::lstrip(const std::string& s) const {
	std::string::size_type pos = s.find_first_not_of(whitespace());
	if (pos > s.size())
		return std::string{};
	return s.substr(pos);
}

std::string Strip_impl::rstrip(const std::string& s) const {
	std::string::size_type pos = s.find_last_not_of(whitespace());
	return s.substr(0, pos + 1);
}

std::string Strip_impl::strip(const std::string& s) const {
	return rstrip(lstrip(s));
}
