#pragma once

#include <string>
#include <optional>

#include "devoured/devoured.h"

namespace dvr {
	struct Parameter{
	public:
		// TODO Use the enum class Devoured::Mode and remove this later on
		enum class Mode : uint8_t{
			INVALID,
			DAEMON,
			STATUS,
			INTERACTIVE,
			COMMAND,
			ALIAS,
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
