#pragma once

#include <string>

namespace dvr {
	struct Config {
		//Flag to set/check invalid config
		int valid = 0;

		std::string control_iloc = "/tmp/devoured/";
		std::string control_name = "default";
	};

	const Config parseConfig(const std::string& path);
}
