#pragma once

#include <string>
#include <optional>

#include "devoured/devoured.h"

namespace dvr {
	struct Parameter{
	public:
		//CONFIG VALUES
		Devoured::Mode mode;

		std::string path;

		bool interactive;
		bool status;
		std::optional<std::string> alias;
		std::optional<std::string> command;
		bool devour;
		std::optional<std::string> manage;

		std::optional<std::string> target;
	};

	const Parameter parseParams(int argc, char** argv);
}
