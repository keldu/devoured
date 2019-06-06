#include "config.h"

#include <cpptoml.h>

#include <filesystem>

namespace dvr{

	void checkParsedConfig(Config& config){
		std::filesystem::path c_name{config.control_name};
		std::filesystem::path c_iloc{config.control_iloc};

		std::filesystem::path full_path{c_iloc.string() + c_name.string()};
	}

	const Config parseConfig(const std::string& path){
		Config config;
		std::filesystem::path config_path{path};

		std::shared_ptr<cpptoml::table> toml_table =  cpptoml::parse_file(std::filesystem::absolute(config_path).string());

		{
			auto table = toml_table->get_table("Socket");
			config.control_name = table->get_as<std::string>("Name").value_or(config.control_name);
			config.control_iloc = table->get_as<std::string>("Path").value_or(config.control_iloc);
		}

		return config;
	}
}