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

	bool parseTomlConfig(Config& config, const std::string& path){
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

	bool parseTomlService(ServiceConfig& config, const std::string& path){
		std::shared_ptr<cpptoml::table> toml_table = nullptr;
		try {
			toml_table = cpptoml::parse_file(path);
		}
		catch(std::exception& e){
			std::cerr<<"No service config file found at ( "<<path<<" ) Consider creating one."<<e.what()<<std::endl;
			return false;
		}
		{
			auto table = toml_table->get_table("Service");
			if(!table){
				table = cpptoml::make_table();
				toml_table->insert("Service", table);
			}
			config.working_directory = table->get_as<std::string>("WorkingDirectory").value_or(config.working_directory);
			/*
			* Split the start command into the proper format
			*/
			{
				std::string command = table->get_as<std::string>("Start").value_or("");
				std::stringstream ss{command};
				std::string token;
				char delimiter = ' ';
				if(std::getline(ss, token, delimiter)){
					config.start.path = token;
					while(std::getline(ss, token, delimiter)){
						config.start.arguments.push_back(token);
					}
				}else{
					std::cerr<<"No command was set"<<std::endl;
					return false;
				}
			}
			// TODO create multiple stop alternatives. E.g. kill() / command with execve / command through the minecraft pipe
			/*
			* Split stop command if it exists into a proper format
			*/
			{
				auto stop_opt = table->get_as<std::string>("Stop");
				if(stop_opt){
					Command stop_cmd;
					std:: string command = *stop_opt;
					char delimiter = ' ';
					std::stringstream ss{command};
					std::string token;
					if(std::getline(ss, token, delimiter)){
						stop_cmd.path = token;
						while(std::getline(ss, token, delimiter)){
							stop_cmd.arguments.push_back(token);
						}
					}else{
						std::cerr<<"No command was set"<<std::endl;
						return false;
					}
					config.stop = stop_cmd;
				}else{
					config.stop = std::nullopt;
				}
			}
		}
		return true;
	}

	bool Environment::isRoot() const {
		return user_id == 0;
	}

	const Config Environment::parseConfig(){
		Config config;
		parseTomlConfig(config, configPath().native());
		return config;
	}

	std::optional<ServiceConfig> Environment::parseService(const std::string& path){
		fs::path service_config_path = service_directory / (path+std::string{".service"});
		if(fs::exists(service_config_path)){
			ServiceConfig config;
			if(parseTomlService(config, service_config_path.native())){
				return config;
			}else{
				return std::nullopt;
			}
		}else{
			return std::nullopt;
		}
	}

	static Environment* global_environment = nullptr;

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
		temp_devoured_dir{fs::temp_directory_path()/std::string_view{"devoured"}}
	{
		// TODO move to setup with check
		fs::create_directories(temp_devoured_dir);
		assert(!global_environment);
		if(!global_environment){
			global_environment = this;
		}
	}

	Environment::Environment(){
	}

	Environment::Environment(Environment&& e):
		user_id{e.user_id},
		user_id_string{e.user_id_string},
		home_path{e.home_path},
		config_directory{e.config_directory},
		config_path{e.config_path},
		service_directory{e.service_directory},
		lock_file_path{e.lock_file_path},
		temp_devoured_dir{e.temp_devoured_dir}
	{
		if(global_environment == &e){
			global_environment = this;
		}
	}

	Environment::~Environment(){
		assert(global_environment);
		if(global_environment == this){
			global_environment = nullptr;
		}
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
				config_dir = usr_home/std::string_view{USER_CONFIG_DIR};
				// TODO remove this
				std::cout<<"Home "<<config_dir.native()<<std::endl;
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

		fs::path service_dir = config_dir / std::string{"service"};

		if(fs::exists(service_dir)){
			if(!fs::is_directory(service_dir)){
				std::cerr<<"Path ( "<<service_dir.native()<<" ) is not a directory!"<<std::endl;
				return std::nullopt;
			}
		}else{
			if(!fs::create_directories(service_dir)){
				std::cerr<<"Service directory can't be created at path ( "<<service_dir.native()<<" )"<<std::endl;
				return std::nullopt;
			}
		}

		fs::path config_path = config_dir / config_file;
		std::string config_path_str = config_path.native();
		std::string service_dir_str = service_dir.native();

		Environment environment{
			uid,
			"",
			isRoot?ROOT_CONFIG_DIR:USER_CONFIG_DIR,
			config_path_str,
			service_dir_str,
			""
		};

		return std::make_optional(std::move(environment));
	}

	Environment& globalEnvironment(){
		assert(global_environment);
		return *global_environment;
	}
}
