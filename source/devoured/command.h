#pragma once

#include <string>
#include <vector>

namespace dvr {
	struct Command {
	public:
		std::string path;
		std::vector<std::string> arguments;
	};
}
