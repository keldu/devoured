#include "Config.h"

#include <cxxopts.hpp>

namespace dvr {
	cxxopts::Options createOptions(Config& config){
		cxxopts::Options options("devoured", " - a wrapper around badly behaving binaries");

		options.add_options()
			("i,interactive","open interactive", cxxopts::value<bool>(config.interactive))
			("s,status", "shows status", cxxopts::value<bool>(config.status))
			("c,command", "sends a command", cxxopts::value<std::optional<std::string>>(config.command))
			("a,alias", "an aliased command", cxxopts::value<std::optional<std::string>>(config.alias))
			("d,devour", "wrap a binary and execute it", cxxopts::value<std::optional<std::string>>(config.devour))
			("t,target", "name of the session", cxxopts::value<std::string>(config.target))
		;

		return options;
	}

	void checkDependencies(Config& config){
		uint8_t counter = 0;

		if(config.interactive){
			++counter;
			config.mode = Config::Mode::INTERACTIVE;
		}
		if(config.status){
			++counter;
			config.mode = Config::Mode::STATUS;
		}
		if(config.command.has_value()){
			++counter;
			config.mode = Config::Mode::COMMAND;
		}
		if(config.alias.has_value()){
			++counter;
			config.mode = Config::Mode::ALIAS;
		}
		if(config.devour.has_value()){
			++counter;
			config.mode = Config::Mode::SERVICE;
		}

		if(counter == 0){
			config.mode = Config::Mode::INTERACTIVE;
		}else if (counter > 1){
			config.mode = Config::Mode::INVALID;

			std::cerr<<"Specified more than one top level option. Choose only one"<<std::endl;
		}
	}

	const Config parseConfig(int argc, char** argv){
		Config config;

		cxxopts::Options options = createOptions(config);
		try {
			auto result = options.parse(argc,argv);
			checkDependencies(config);
		}catch(const cxxopts::OptionException& e){
			config.mode = Config::Mode::INVALID;
			std::cerr<<"Error while parsing options: "<<e.what()<<std::endl;
		}

		return config;
	}
}