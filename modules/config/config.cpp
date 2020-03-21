#include "config.h"

#include <cpptoml.h>

#include <filesystem>
#include <iostream>

#include <unistd.h>

#ifndef ROOT_CONFIG_DIR
#define ROOT_CONFIG_DIR "/etc/devoured/"
#endif

#ifndef USER_CONFIG_DIR
#define USER_CONFIG_DIR ".config/devoured/"
#endif

const std::string config_file = "devoured.cfg";

namespace fs = std::filesystem;

namespace dvr{

	void checkParsedConfig(Config& config){
		fs::path c_name{config.control_name};
		fs::path c_iloc{config.control_iloc};

		fs::path full_path{c_iloc.string() + c_name.string()};
	}

	bool parseConfig(Config& config, const std::string& path){
		fs::path config_path{path};

		std::shared_ptr<cpptoml::table> toml_table;
		try {
			toml_table =  cpptoml::parse_file(fs::absolute(config_path).string());
		}catch(std::exception& e){
			std::cerr<<"No config file found at "<<path<<". Consider creating one."<<std::endl;
			return false;
		}
		{
			auto table = toml_table->get_table("Socket");
			if(!table){
				table = cpptoml::make_table();
				toml_table->insert("Socket", table);
			}
			config.control_name = table->get_as<std::string>("Name").value_or(config.control_name);
			config.control_iloc = table->get_as<std::string>("Path").value_or(config.control_iloc);
		}
		return true;
	}

	const ServiceConfig parseServiceConfig(const std::string& path){
		ServiceConfig config;
		fs::path config_path{path};
		
		std::shared_ptr<cpptoml::table> toml_table =  cpptoml::parse_file(fs::absolute(config_path).string());
		{
			auto table = toml_table->get_table("Service");
			if(!table){
				table = cpptoml::make_table();
				toml_table->insert("Service", table);
			}
		}
		return config;
	}

	bool Environment::isRoot() const {
		return user_id == 0;
	}

	const Config Environment::parseConfig(){
		return Config{};
	}

	const std::vector<ServiceConfig> Environment::parseServices() {
		return std::vector<ServiceConfig>{};
	}

	Environment::Environment(uid_t user_id, std::string_view home_path, std::string_view config_directory, std::string_view config_path, std::string_view service_directory, std::string_view lock_file_path):
		user_id{user_id},
		home_path{home_path},
		config_directory{config_directory},
		config_path{config_path},
		service_directory{service_directory},
		lock_file_path{lock_file_path},
		temp_devoured_dir{fs::temp_directory_path().concat("/devoured")}
	{
		// TODO move to setup with check
		fs::create_directories(temp_devoured_dir);
	}

	Environment::Environment(){
	}

	std::optional<Environment> setupEnvironment(){
		uid_t uid = ::getuid();
		bool isRoot = uid == 0;

		fs::path config_dir;
		if(isRoot){
			config_dir = ROOT_CONFIG_DIR;
		}else{
			auto home_env = ::getenv("HOME");
			if(home_env){
				fs::path usr_home{home_env};
				config_dir = usr_home.concat(std::string{"/"}+USER_CONFIG_DIR);
				std::cerr<<"Home "<<config_dir.native()<<std::endl;
			}else{
				std::cerr<<"HOME env variable not set"<<std::endl;
				return std::nullopt;
			}
		}

		if(fs::exists(config_dir)){
			if(!fs::is_directory(config_dir)){
				std::cerr<<"Path ( "<<config_dir.native()<<" ) is not a directory!"<<std::endl;
				return std::nullopt;
			}
		}else{
			if(!fs::create_directories(config_dir)){
				std::cerr<<"Config directory can't be created at path ( "<<config_dir.native()<<" )"<<std::endl;
				return std::nullopt;
			}
		}

		fs::path config_path{};

		Environment environment{
			uid,
			"",
			isRoot?ROOT_CONFIG_DIR:USER_CONFIG_DIR,
			"",
			"",
			""
		};

		return std::make_optional(std::move(environment));
	}
}
