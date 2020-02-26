#pragma once

#include <string>
#include <vector>
#include <map>

namespace dvr {
	struct Config {
		//Flag to set/check invalid config
		int valid = 0;

		std::string control_iloc = "/tmp/devoured/";
		std::string control_name = "default";
	};

	const Config parseConfig(const std::string& path);

	struct ServiceConfig {
		/*
		 * Working directory of program to be run
		 */
		std::string working_directory;
		
		/*
		 * Command to start binary
		 */
		std::string start_command;
		
		/*
		 * Arguments to be used
		 */
		std::vector<std::string> arguments;
		
		/*
		 * Stop command
		 */
		std::string stop_command;
	
		/*
		 * Optionally map signals like SIGINT etc to some command
		 */
		std::map<std::string, std::string> signal_translation;
		/*
		 * Alias for commands for easier control
		 */
		std::map<std::string, std::string> alias_translation;
	};

	const ServiceConfig parseService(const std::string& path);
}
