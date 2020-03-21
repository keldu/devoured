#pragma once

#include <optional>
#include <string>
#include <vector>
#include <map>
#include <filesystem>

#include <sys/types.h>

namespace dvr {
	struct Config {
		//Flag to set/check invalid config
		int valid = 0;

		std::string control_iloc = "/tmp/devoured/";
		std::string control_name = "default";
	};

	bool parseConfig(Config& config, const std::string& path);

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
		std::optional<std::string> stop_command;
	
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
	
	class Environment {
	private:
		uid_t user_id;
		std::string user_id_string;

		std::filesystem::path home_path;
		std::filesystem::path config_directory;
		std::filesystem::path config_path;
		std::filesystem::path service_directory;
		std::filesystem::path lock_file_path;
		std::filesystem::path temp_devoured_dir;

		friend std::optional<Environment> setupEnvironment();
		Environment(uid_t uid, std::string_view home, std::string_view config_dir, std::string_view config_path, std::string_view service_dir, std::string_view lock_file);
	public:
		/*
		 * Default constructor for invalid states
		 */
		Environment();

		bool isRoot() const;

		const Config parseConfig();
		const std::vector<ServiceConfig> parseServices();

		uid_t userId() const{
			return user_id;
		}

		const std::filesystem::path& tmpPath() const {
			return temp_devoured_dir;
		}
	};

	/*
	 * Find out which user we are running as.
	 * Especially if we are root
	 * Does a service already run?
	 * Where should the default socket location be?
	 * Where is the users home, etc
	 *
	 * On fail, return std::nullopt_t ( or however it was called )
	 * Exceptions would be ok, but I dislike exceptions.
	 */
	std::optional<Environment> setupEnvironment();
}
