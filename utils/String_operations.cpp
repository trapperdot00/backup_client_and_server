#include "String_operations.h"
#include "Strip_impl.h"

#include <memory>

static std::unique_ptr<Strip_impl>& get_impl() {
	static std::unique_ptr<Strip_impl> p =
		std::make_unique<Strip_impl>();
	return p;
}

std::string lstrip(const std::string& s) {
	return get_impl()->lstrip(s);
}

std::string rstrip(const std::string& s) {
	return get_impl()->rstrip(s);
}

std::string strip(const std::string& s) {
	return get_impl()->strip(s);
}
