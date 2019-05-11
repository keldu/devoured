#pragma once

#include <string>
#include <optional>

namespace dvr {

	struct Parameter{
	public:

		enum class Mode{
			INVALID,
			SERVICE,
			INTERACTIVE,
			COMMAND,
			ALIAS,
			STATUS
		};

		//CONFIG VALUES
		Mode mode;

		std::string path;

		bool interactive;
		bool status;
		std::optional<std::string> alias;
		std::optional<std::string> command;
		std::optional<std::string> devour;

		std::string target;
	};

	const Parameter parseParams(int argc, char** argv);

}