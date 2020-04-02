#include "parameter.h"

#include <cxxopts.hpp>

#include <map>

namespace dvr {
	cxxopts::Options createOptions(Parameter& params){
		cxxopts::Options options("devoured", " - a wrapper around badly behaving binaries");

		options.show_positional_help();

		options.show_positional_help()
			.add_options()
			("i,interactive", "open interactive", cxxopts::value<bool>(params.interactive))
			("s,status", "shows status", cxxopts::value<bool>(params.status))
			("c,command", "sends a command", cxxopts::value<std::optional<std::string>>(params.command))
			("a,alias", "an aliased command", cxxopts::value<std::optional<std::string>>(params.alias))
			("d,devour", "starts the daemon and execute it", cxxopts::value<bool>(params.devour))
			("t,target", "name of the session", cxxopts::value<std::optional<std::string>>(params.target))
			("m,manage", "manage devoured", cxxopts::value<bool>(params.spawn))
			("control", "Control keywords", cxxopts::value<std::optional<std::string>>(params.control))
			("h,help", "Display help")
		;

		options.parse_positional({"control"});

		return options;
	}

	const std::map<std::string, Devoured::Mode> control_keywords = {
		{"start",Devoured::Mode::START},
		{"stop",Devoured::Mode::STOP},
		{"enable",Devoured::Mode::ENABLE},
		{"disable",Devoured::Mode::DISABLE}
	};

	void checkDependencies(Parameter& config){
		uint8_t counter = 0;

		if(config.interactive){
			++counter;
			config.mode = Devoured::Mode::INTERACTIVE;
		}
		if(config.status){
			++counter;
			config.mode = Devoured::Mode::STATUS;
		}
		if(config.command.has_value()){
			++counter;
			config.mode = Devoured::Mode::COMMAND;
		}
		if(config.alias.has_value()){
			++counter;
			config.mode = Devoured::Mode::ALIAS;
		}
		if(config.devour){
			++counter;
			config.mode = Devoured::Mode::DAEMON;
		}
		if(config.control.has_value()){
			auto find = control_keywords.find(config.control.value());
			if( find != control_keywords.end() ){
				++counter;
				config.mode = find->second;
			}
		}
		
		if(counter == 0){
			config.mode = Devoured::Mode::INTERACTIVE;
		}else if (counter > 1){
			config.mode = Devoured::Mode::INVALID;
			std::cerr<<"Specified more than one top level option. Choose only one"<<std::endl;
		}
	}

	const Parameter parseParams(int argc, char** argv){
		Parameter params;
		cxxopts::Options options = createOptions(params);
		try {
			auto result = options.parse(argc,argv);
			checkDependencies(params);
			if(result.count("help")){
				std::cout<<options.help({""})<<std::endl;
				exit(0);
			}
		}catch(const cxxopts::OptionException& e){
			params.mode = Devoured::Mode::INVALID;
			std::cerr<<"Error while parsing options: "<<e.what()<<std::endl;
		}

		return params;
	}
}
