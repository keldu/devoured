#include "config.h"

#include <cpptoml.h>

namespace dvr{
	const Config parseConfig(const std::string& path){
		Config config;
		config.control_iloc = "/tmp/";
		config.control_name = "terraria";

		return config;
	}
}