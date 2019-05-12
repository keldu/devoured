#include "parameter.h"

#include <cxxopts/cxxopts.hpp>

namespace dvr {
	cxxopts::Options createOptions(Parameter& params){
		cxxopts::Options options("devoured", " - a wrapper around badly behaving binaries");

		options.add_options()
			("i,interactive","open interactive", cxxopts::value<bool>(params.interactive))
			("s,status", "shows status", cxxopts::value<bool>(params.status))
			("c,command", "sends a command", cxxopts::value<std::optional<std::string>>(params.command))
			("a,alias", "an aliased command", cxxopts::value<std::optional<std::string>>(params.alias))
			("d,devour", "wrap a binary and execute it", cxxopts::value<std::optional<std::string>>(params.devour))
			("t,target", "name of the session", cxxopts::value<std::string>(params.target))
		;

		return options;
	}

	void checkDependencies(Parameter& config){
		uint8_t counter = 0;

		if(config.interactive){
			++counter;
			config.mode = Parameter::Mode::INTERACTIVE;
		}
		if(config.status){
			++counter;
			config.mode = Parameter::Mode::STATUS;
		}
		if(config.command.has_value()){
			++counter;
			config.mode = Parameter::Mode::COMMAND;
		}
		if(config.alias.has_value()){
			++counter;
			config.mode = Parameter::Mode::ALIAS;
		}
		if(config.devour.has_value()){
			++counter;
			config.mode = Parameter::Mode::SERVICE;
		}

		if(counter == 0){
			config.mode = Parameter::Mode::INTERACTIVE;
		}else if (counter > 1){
			config.mode = Parameter::Mode::INVALID;

			std::cerr<<"Specified more than one top level option. Choose only one"<<std::endl;
		}
	}

	const Parameter parseParams(int argc, char** argv){
		Parameter params;

		cxxopts::Options options = createOptions(params);
		try {
			auto result = options.parse(argc,argv);
			checkDependencies(params);
		}catch(const cxxopts::OptionException& e){
			params.mode = Parameter::Mode::INVALID;
			std::cerr<<"Error while parsing options: "<<e.what()<<std::endl;
		}

		return params;
	}
}