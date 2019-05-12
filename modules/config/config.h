#pragma once

#include <string>

namespace dvr {
	struct Config {
		//TODO guarantee that location is a valid folder path with a single trailing slash
		std::string control_iloc;
		//TODO guarantee that name has no slashes
		std::string control_name;
	};

	const Config parseConfig(const std::string& path);
}