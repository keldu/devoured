#pragma once

#include <string>
#include <optional>

namespace dvr {
	struct Parameter{
	public:
		enum class Mode : uint8_t{
			INVALID,
			DAEMON,
			INTERACTIVE,
			COMMAND,
			ALIAS,
			STATUS,
			CREATE,
			DESTROY,
			MANAGE
		};

		//CONFIG VALUES
		Mode mode;

		std::string path;

		bool interactive;
		bool status;
		std::optional<std::string> alias;
		std::optional<std::string> command;
		bool devour;
		bool spawn;

		std::optional<std::string> target;
	};

	const Parameter parseParams(int argc, char** argv);

}
