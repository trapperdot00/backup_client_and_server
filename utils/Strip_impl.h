#ifndef STRIP_IMPL_H
#define STRIP_IMPL_H

#include <string>

class Strip_impl {
public:
	Strip_impl();
	std::string lstrip(const std::string&) const;
	std::string rstrip(const std::string&) const;
	std::string strip(const std::string&) const;

	std::string whitespace() const { return s; }
private:
	std::string s;
};

#endif
