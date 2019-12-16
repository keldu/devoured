#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <cstdint>

#include "process_stream.h"

namespace dvr {
	class UserGroup{
	private:
		std::string user_str;
		std::string group_str;

		uid_t user_id;
		gid_t group_id;
	public:
		UserGroup(const std::string& u_p, const std::string& g_p, uid_t uid_p, gid_t gid_p);

		const std::string& user() const;
		const std::string& group() const;

		const uid_t& userId() const;
		const gid_t& groupId() const;
	};

	std::unique_ptr<UserGroup> createUserGroup(const std::string&, const std::string&);
	std::unique_ptr<UserGroup> createUserGroup();

	class Service {
	public:
		enum class State : uint8_t {
			Inactive,
			Active,
			Failed
		};
	private:
		State state;

		std::string working_directory;
		std::string command;
		std::vector<std::string> arguments;

		std::unique_ptr<UserGroup> user_group;

		std::map<std::string, std::string> alias_commands;
		
		std::optional<std::string> alternative_stop_command;
		std::unique_ptr<ProcessStream> process_stream;
	public:
		Service(const std::string& exec_p, const std::vector<std::string>& args_p);
		Service(const std::string& exec_p, const std::vector<std::string>& args_p, const std::string& alternative_stop_p);

		// Alias control functions
		bool addAlias(const std::string& alias, const std::string& command);
		bool removeAlias(const std::string& alias);
		const std::string getAlias(const std::string& alias) const;

		// Pid control
		void setPID();
	};
}
