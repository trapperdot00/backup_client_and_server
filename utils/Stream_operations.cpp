#include "Stream_operations.h"

#include <cctype>

void skip_whitespace(std::istream& is) {
	for (char ch = 0; is.get(ch) && std::isspace(ch); )
		;
	if (is)
		is.unget();
}
